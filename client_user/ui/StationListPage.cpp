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

    // 设置整体主背景色
    setStyleSheet("QWidget { font-family: 'Microsoft YaHei', sans-serif; background-color: #f8fafc; }");

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(14);

    // 1. 顶部悬浮胶囊搜索 Container
    auto searchContainer = new QFrame();
    searchContainer->setStyleSheet(
        "QFrame {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e2e8f0;"
        "   border-radius: 16px;"
        "}"
    );
    auto locateLayout = new QHBoxLayout(searchContainer);
    locateLayout->setContentsMargins(8, 6, 8, 6);
    locateLayout->setSpacing(8);

    // 城市切换下拉框：去除标准外边框，精简为文字选择
    regionCombo = new QComboBox;
    regionCombo->addItem("📍上海", QVariant::fromValue(QPointF(31.2304, 121.4737)));
    regionCombo->addItem("📍北京", QVariant::fromValue(QPointF(39.9042, 116.4074)));
    regionCombo->addItem("📍深圳", QVariant::fromValue(QPointF(22.5431, 114.0579)));
    regionCombo->setStyleSheet(
        "QComboBox { border: none; font-weight: bold; color: #0f172a; font-size: 13px; background: transparent; padding-right: 4px; }"
        "QComboBox::drop-down { border: none; width: 20px; }"
    );

    addressEdit = new QLineEdit;
    addressEdit->setPlaceholderText("搜索充电站或目的地...");
    addressEdit->setStyleSheet("QLineEdit { border: none; background: transparent; font-size: 13px; color: #0f172a; }");

    completerModel = new QStringListModel(this);
    completer = new QCompleter(completerModel, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    addressEdit->setCompleter(completer);

    locateBtn = new QPushButton("搜索");
    locateBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #10b981;"
        "   color: #ffffff;"
        "   font-weight: bold;"
        "   border-radius: 10px;"
        "   padding: 6px 14px;"
        "   border: none;"
        "}"
        "QPushButton:hover { background-color: #059669; }"
    );

    locateLayout->addWidget(regionCombo);
    locateLayout->addWidget(addressEdit, 1);
    locateLayout->addWidget(locateBtn);
    mainLayout->addWidget(searchContainer);

    // 2. 列表标题栏（加入小徽章说明）
    auto sectionLabel = new QLabel("附近优质站点");
    sectionLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #0f172a; background: transparent;");
    mainLayout->addWidget(sectionLabel);

    // 3. 卡片滚动区域
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; }");

    cardContainerWidget = new QWidget();
    cardContainerWidget->setStyleSheet("background: transparent;");
    cardContainerLayout = new QVBoxLayout(cardContainerWidget);
    cardContainerLayout->setContentsMargins(0, 0, 0, 0);
    cardContainerLayout->setSpacing(12);
    cardContainerLayout->addStretch();

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
    // 1. 清空旧卡片[cite: 1]
    QLayoutItem *item;
    while ((item = cardContainerLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    auto list = PlatformService::stations(currentLat, currentLng);
    QList<QWidget*> cardList;

    for (const auto &var : list) {
        QVariantMap m = var.toMap();
        QWidget *card = createStationCard(m);
        cardContainerLayout->addWidget(card);
        cardList.append(card); // 收集新卡片用于动画[cite: 9]
    }

    cardContainerLayout->addStretch();

    // 2. 触发流式交错进场动画[cite: 9]
    auto seqGroup = new QSequentialAnimationGroup(this);
    for (int i = 0; i < cardList.size(); ++i) {
        auto card = cardList[i];
        auto opacityEffect = new QGraphicsOpacityEffect(card);
        card->setGraphicsEffect(opacityEffect);

        auto parallel = new QParallelAnimationGroup(seqGroup);

        // 淡入[cite: 9]
        auto fade = new QPropertyAnimation(opacityEffect, "opacity");
        fade->setDuration(300);
        fade->setStartValue(0.0);
        fade->setEndValue(1.0);
        fade->setEasingCurve(QEasingCurve::OutCubic);

        // 向上平滑上滑[cite: 9]
        auto move = new QPropertyAnimation(card, "pos");
        move->setDuration(300);
        QPoint finalPos = card->pos();
        move->setStartValue(QPoint(finalPos.x(), finalPos.y() + 25));
        move->setEndValue(finalPos);
        move->setEasingCurve(QEasingCurve::OutCubic);

        parallel->addAnimation(fade);
        parallel->addAnimation(move);
        seqGroup->addAnimation(parallel);
    }
    seqGroup->start(QAbstractAnimation::DeleteWhenStopped);
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

    auto cardWidget = new QWidget();
    cardWidget->setCursor(Qt::PointingHandCursor);
    cardWidget->setStyleSheet(
        "QWidget {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e2e8f0;"
        "   border-radius: 16px;"
        "}"
        "QWidget:hover {"
        "   border-color: #10b981;"
        "   background-color: #ffffff;"
        "}"
    );

    auto cardLayout = new QVBoxLayout(cardWidget);
    cardLayout->setContentsMargins(16, 14, 16, 14);
    cardLayout->setSpacing(8);

    // 第一行：站点名与距离 Pill 标签[cite: 9]
    auto topLayout = new QHBoxLayout();
    auto nameLabel = new QLabel(name);
    nameLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #0f172a; border: none; background: transparent;");

    auto distLabel = new QLabel(QString("📍 %1 km ").arg(distance, 0, 'f', 1));
    distLabel->setStyleSheet(
        "font-size: 11px; font-weight: bold; color: #d97706; "
        "background-color: #fef3c7; border-radius: 8px; padding: 2px 8px; border: none;"
    );

    topLayout->addWidget(nameLabel, 1);
    topLayout->addWidget(distLabel);
    cardLayout->addLayout(topLayout);

    // 第二行：详细地址[cite: 9]
    auto addrLabel = new QLabel(address.isEmpty() ? "暂无详细地址信息" : address);
    addrLabel->setStyleSheet("font-size: 12px; color: #64748b; border: none; background: transparent;");
    cardLayout->addWidget(addrLabel);

    // 第三行：单价与极速快充可视圆点[cite: 9]
    auto bottomLayout = new QHBoxLayout();
    auto priceLabel = new QLabel(QString("¥ <font size='4'><b>%1</b></font> /度").arg(price, 0, 'f', 2));
    priceLabel->setStyleSheet("font-size: 12px; color: #0f172a; border: none; background: transparent;");

    // 动态绘制微型状态胶囊[cite: 9]
    QString idleBg = idle > 0 ? "#ecfdf5" : "#fef2f2";
    QString idleText = idle > 0 ? "#10b981" : "#ef4444";
    auto idleLabel = new QLabel(QString("● 空闲 %1 / %2 桩 ").arg(idle).arg(total));
    idleLabel->setStyleSheet(QString(
        "font-size: 11px; font-weight: bold; color: %1; background-color: %2; "
        "border-radius: 8px; padding: 3px 8px; border: none;"
    ).arg(idleText, idleBg));

    bottomLayout->addWidget(priceLabel);
    bottomLayout->addStretch();
    bottomLayout->addWidget(idleLabel);
    cardLayout->addLayout(bottomLayout);

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
