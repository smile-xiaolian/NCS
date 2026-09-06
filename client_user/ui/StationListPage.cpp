#include "StationListPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QPointF>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDebug>
#include "core/service/PlatformService.h"

static const QString TENCENT_MAP_KEY = "VMNBZ-HQHE7-NH2XF-HGZCP-ZDC2T-FMFBU";

StationListPage::StationListPage(QWidget *parent) : QWidget(parent)
{
    networkManager = new QNetworkAccessManager(this);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(10);

    auto locateLayout = new QHBoxLayout();
    regionCombo = new QComboBox;
    regionCombo->addItem("上海·人民广场", QVariant::fromValue(QPointF(31.2304, 121.4737)));
    regionCombo->addItem("上海·陆家嘴", QVariant::fromValue(QPointF(31.2393, 121.5000)));
    regionCombo->addItem("北京·天安门", QVariant::fromValue(QPointF(39.9042, 116.4074)));
    regionCombo->addItem("深圳·福田", QVariant::fromValue(QPointF(22.5431, 114.0579)));

    addressEdit = new QLineEdit;
    addressEdit->setPlaceholderText("输入地址或搜索关键字...");

    // 设置自动补全器 Completer
    completerModel = new QStringListModel(this);
    completer = new QCompleter(completerModel, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    addressEdit->setCompleter(completer);

    locateBtn = new QPushButton("定位");

    locateLayout->addWidget(regionCombo, 2);
    locateLayout->addWidget(addressEdit, 2);
    locateLayout->addWidget(locateBtn, 1);
    layout->addLayout(locateLayout);

    layout->addWidget(new QLabel("<b>附近优质充电站（点击卡片直接预约）</b>"));

    stationTable = new QTableWidget;
    setupTable();
    layout->addWidget(stationTable);

    auto enterDetailBtn = new QPushButton("进入选桩详情页");
    layout->addWidget(enterDetailBtn);

    // 监听输入框文本改变，触发联想建议接口
    connect(addressEdit, &QLineEdit::textEdited, this, &StationListPage::fetchAddressSuggestions);

    // 用户从联想下拉列表中选中某一项时
    connect(completer, QOverload<const QString &>::of(&QCompleter::activated), this, [this](const QString &text) {
        if (suggestionCoords.contains(text)) {
            auto pair = suggestionCoords[text];
            currentLat = pair.first;
            currentLng = pair.second;
            refreshStations();
        }
    });

    connect(locateBtn, &QPushButton::clicked, this, &StationListPage::onLocate);
    connect(enterDetailBtn, &QPushButton::clicked, this, &StationListPage::onStationClick);
    connect(stationTable, &QTableWidget::doubleClicked, this, &StationListPage::onStationClick);
}

void StationListPage::setupTable()
{
    QStringList headers = { "站点名称", "单价", "空闲/总数", "距离" };
    stationTable->setColumnCount(headers.size());
    stationTable->setHorizontalHeaderLabels(headers);
    stationTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    stationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    stationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    stationTable->verticalHeader()->setVisible(false);
    stationTable->setStyleSheet(
        "QTableWidget { background-color: #ffffff; alternate-background-color: #f9f9f9; border: 1px solid #e0e0e0; border-radius: 8px; gridline-color: #f0f0f0; }"
        "QHeaderView::section { background-color: #f5f7fa; padding: 8px; border: none; font-weight: bold; color: #333333; }"
    );
}

void StationListPage::refreshStations()
{
    auto list = PlatformService::stations(currentLat, currentLng);
    stationTable->setRowCount(list.size());

    for (int i = 0; i < list.size(); i++)
    {
        auto m = list[i].toMap();
        QStringList v = {
            m["name"].toString(),
            QString("%1 元/度").arg(m["price"].toDouble(), 0, 'f', 2),
            QString("%1 / %2 桩").arg(m["idle"].toInt()).arg(m["total"].toInt()),
            QString("%1 km").arg(m["distance"].toDouble(), 0, 'f', 1)
        };

        for (int j = 0; j < v.size(); j++)
        {
            auto item = new QTableWidgetItem(v[j]);
            if (j == 2) {
                item->setForeground(m["idle"].toInt() > 0 ? QColor("#52c41a") : QColor("#ff4d4f"));
            }
            stationTable->setItem(i, j, item);
        }
        stationTable->item(i, 0)->setData(Qt::UserRole, m["id"]);
    }
}

// 关键词输入提示 API（联想列表）
void StationListPage::fetchAddressSuggestions(const QString &keyword)
{
    if (keyword.trimmed().isEmpty()) {
        completerModel->setStringList(QStringList());
        return;
    }

    QUrl url("https://apis.map.qq.com/ws/place/v1/suggestion");
    QUrlQuery query;
    query.addQueryItem("keyword", keyword);
    query.addQueryItem("key", TENCENT_MAP_KEY);
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "联想搜索网络请求失败：" << reply->errorString();
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject rootObj = doc.object();

        int status = rootObj["status"].toInt();
        if (status == 0) {
            QJsonArray dataArray = rootObj["data"].toArray();
            QStringList suggestions;
            suggestionCoords.clear();

            for (const QJsonValue &val : dataArray) {
                QJsonObject item = val.toObject();
                QString title = item["title"].toString();
                QString address = item["address"].toString();
                QJsonObject loc = item["location"].toObject();

                QString displayStr = QString("%1 (%2)").arg(title, address);
                suggestions.append(displayStr);
                
                suggestionCoords[displayStr] = qMakePair(loc["lat"].toDouble(), loc["lng"].toDouble());
            }

            completerModel->setStringList(suggestions);
            completer->complete();
        } else {
            qWarning() << "腾讯地图 Suggestion API 异常，status:" << status << " message:" << rootObj["message"].toString();
        }
    });
}

void StationListPage::onLocate()
{
    QString inputAddr = addressEdit->text().trimmed();
    if (suggestionCoords.contains(inputAddr)) {
        auto pair = suggestionCoords[inputAddr];
        currentLat = pair.first;
        currentLng = pair.second;
        refreshStations();
    } else if (!inputAddr.isEmpty()) {
        geocodeAddress(inputAddr);
    } else {
        QPointF coords = regionCombo->currentData().toPointF();
        currentLat = coords.x();
        currentLng = coords.y();
        refreshStations();
    }
}

void StationListPage::geocodeAddress(const QString &address)
{
    QUrl url("https://apis.map.qq.com/ws/geocoder/v1/");
    QUrlQuery query;
    query.addQueryItem("address", address);
    query.addQueryItem("key", TENCENT_MAP_KEY);
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, address]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, "定位失败", "网络请求错误：" + reply->errorString());
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject rootObj = doc.object();

        if (rootObj["status"].toInt() == 0) {
            QJsonObject locationObj = rootObj["result"].toObject()["location"].toObject();
            currentLat = locationObj["lat"].toDouble();
            currentLng = locationObj["lng"].toDouble();

            QMessageBox::information(this, "定位成功", QString("已精确定位至：%1").arg(address));
            refreshStations();
        } else {
            QMessageBox::warning(this, "定位失败", "无法解析输入的地址，请重试");
        }
    });
}

void StationListPage::onStationClick()
{
    auto item = stationTable->currentItem();
    if (!item) {
        QMessageBox::information(this, "提示", "请先选择一个充电站");
        return;
    }
    int stationId = stationTable->item(item->row(), 0)->data(Qt::UserRole).toInt();
    emit stationSelected(stationId);
}