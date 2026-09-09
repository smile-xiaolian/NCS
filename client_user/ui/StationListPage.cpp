#include "StationListPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPointF>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDebug>
#include <QMouseEvent>
#include <QAbstractItemView>

#include "core/service/PlatformService.h"

static const QString TENCENT_MAP_KEY = "VMNBZ-HQHE7-NH2XF-HGZCP-ZDC2T-FMFBU";

StationListPage::StationListPage(QWidget *parent) : QWidget(parent)
{
    networkManager = new QNetworkAccessManager(this);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(10);

    // 1. 顶部定位选择区
    auto locateLayout = new QHBoxLayout();
    regionCombo = new QComboBox;
    regionCombo->addItem("上海·人民广场", QVariant::fromValue(QPointF(31.2304, 121.4737)));
    regionCombo->addItem("上海·陆家嘴", QVariant::fromValue(QPointF(31.2393, 121.5000)));
    regionCombo->addItem("北京·天安门", QVariant::fromValue(QPointF(39.9042, 116.4074)));
    regionCombo->addItem("深圳·福田", QVariant::fromValue(QPointF(22.5431, 114.0579)));

    addressEdit = new QLineEdit;
    addressEdit->setPlaceholderText("输入地址或搜索关键字...");

    completerModel = new QStringListModel(this);
    completer = new QCompleter(completerModel, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    addressEdit->setCompleter(completer);

    locateBtn = new QPushButton("定位");

    locateLayout->addWidget(regionCombo, 2);
    locateLayout->addWidget(addressEdit, 2);
    locateLayout->addWidget(locateBtn, 1);
    mainLayout->addLayout(locateLayout);

    mainLayout->addWidget(new QLabel("<b>附近优质充电站（点击卡片选择电桩）</b>"));

    // 2. 替换为卡片滚动列表容器
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; }");

    cardContainerWidget = new QWidget();
    cardContainerWidget->setStyleSheet("background: transparent;");
    cardContainerLayout = new QVBoxLayout(cardContainerWidget);
    cardContainerLayout->setContentsMargins(0, 0, 0, 0);
    cardContainerLayout->setSpacing(12);
    cardContainerLayout->addStretch(); // 底部弹簧置底

    scrollArea->setWidget(cardContainerWidget);
    mainLayout->addWidget(scrollArea);

    // 3. 信号槽绑定
    connect(addressEdit, &QLineEdit::textEdited, this, &StationListPage::fetchAddressSuggestions);

    QAbstractItemView *popup = completer->popup();
    connect(popup, &QAbstractItemView::clicked, this, [this](const QModelIndex &index) {
        QString text = index.data(Qt::DisplayRole).toString();
        if (suggestionCoords.contains(text)) {
            auto pair = suggestionCoords[text];
            currentLat = pair.first;
            currentLng = pair.second;
            addressEdit->setText(text);
            refreshStations();
        }
    });

    connect(locateBtn, &QPushButton::clicked, this, &StationListPage::onLocate);
}

void StationListPage::refreshStations()
{
    // 清空旧的卡片（保留最后的弹簧）
    QLayoutItem *item;
    while ((item = cardContainerLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    // 从数据库获取充电站数据并自动按距离排序[cite: 6]
    auto list = PlatformService::stations(currentLat, currentLng);

    for (const auto &var : list) {
        QVariantMap m = var.toMap();
        QWidget *card = createStationCard(m);
        cardContainerLayout->addWidget(card);
    }

    // 在底部重新加回弹性垫片
    cardContainerLayout->addStretch();
}

QWidget* StationListPage::createStationCard(const QVariantMap &m)
{
    int stationId = m["id"].toInt();
    QString name = m["name"].toString();
    QString address = m["address"].toString();
    double price = m["price"].toDouble();
    int idle = m["idle"].toInt();
    int total = m["total"].toInt();
    double distance = m["distance"].toDouble();

    // 1. 卡片外层容器 Widget
    auto cardWidget = new QWidget();
    cardWidget->setCursor(Qt::PointingHandCursor);
    cardWidget->setStyleSheet(
        "QWidget {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e4e7ed;"
        "   border-radius: 12px;"
        "}"
        "QWidget:hover {"
        "   border-color: #409eff;"
        "   background-color: #f8fafc;"
        "}"
    );

    auto cardLayout = new QVBoxLayout(cardWidget);
    cardLayout->setContentsMargins(15, 12, 15, 12);
    cardLayout->setSpacing(6);

    // 2. 第一行：站点名称与距离
    auto topLayout = new QHBoxLayout();
    auto nameLabel = new QLabel(name);
    nameLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #2c3e50; border: none; background: transparent;");

    auto distLabel = new QLabel(QString("%1 km").arg(distance, 0, 'f', 1));
    distLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #e6a23c; border: none; background: transparent;");

    topLayout->addWidget(nameLabel, 1);
    topLayout->addWidget(distLabel);
    cardLayout->addLayout(topLayout);

    // 3. 第二行：详细地址
    auto addrLabel = new QLabel(address.isEmpty() ? "暂无详细地址信息" : address);
    addrLabel->setStyleSheet("font-size: 12px; color: #909399; border: none; background: transparent;");
    addrLabel->setWordWrap(true);
    cardLayout->addWidget(addrLabel);

    // 4. 第三行：单价与空闲/总数统计
    auto bottomLayout = new QHBoxLayout();
    
    auto priceLabel = new QLabel(QString("单价：<font color='#f56c6c'><b>¥ %1</b></font> 元/度").arg(price, 0, 'f', 2));
    priceLabel->setStyleSheet("font-size: 13px; color: #606266; border: none; background: transparent;");

    QString idleColor = idle > 0 ? "#67c23a" : "#f56c6c";
    auto idleLabel = new QLabel(QString("空闲 <font color='%1'><b>%2</b></font> / %3 桩").arg(idleColor).arg(idle).arg(total));
    idleLabel->setStyleSheet("font-size: 12px; color: #606266; border: none; background: transparent;");

    bottomLayout->addWidget(priceLabel);
    bottomLayout->addStretch();
    bottomLayout->addWidget(idleLabel);
    cardLayout->addLayout(bottomLayout);

    // 5. 使用事件过滤器监听整个卡片Widget的点击事件
    cardWidget->installEventFilter(this);
    cardWidget->setProperty("stationId", stationId);

    return cardWidget;
}

// 监听卡片点击事件
bool StationListPage::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        auto mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            QVariant prop = watched->property("stationId");
            if (prop.isValid()) {
                int stationId = prop.toInt();
                emit stationSelected(stationId); // 触发进入选桩详情页
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

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

