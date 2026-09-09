#include "ui/PlatformMainWindow.h"
#include "DemoPiles.h"
#include "PlatformController.h"
#include "ui/PileMainWindow.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QDir>
#include <QFrame>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QProcess>
#include <QScrollArea>
#include <QStringList>
#include <QStyle>
#include <QTime>
#include <QVBoxLayout>
#include <QWidget>

namespace {

// 演示参数:功率条满量程(按 10 kW 桩)、单次充电演示目标电量。
constexpr double kRatedPowerKw = 10.0;
constexpr double kDefaultSessionKwh = 0.2;
constexpr double kPowerEpsilon = 0.001;

QString demoQss()
{
    return QStringLiteral(R"QSS(
QMainWindow { background:#EEF1F6; }
QFrame#pileCard { background:#FFFFFF; border:1px solid #E1E6EF; border-radius:12px; }
QFrame#pileCard[offline="true"] { background:#F6F7FA; border-color:#E8EBF1; }
QLabel#title { color:#1F2733; font-size:15px; font-weight:700; }
QLabel#muted { color:#7A8699; font-size:12px; }
QLabel#value { color:#1F2733; font-size:13px; font-weight:600; }
QLabel#stateChip { padding:2px 10px; border-radius:9px; font-size:12px; font-weight:600; }
QLabel#stateChip[chipState="idle"] { color:#1E8E5A; background:#E6F7EF; }
QLabel#stateChip[chipState="charging"] { color:#1F6FD0; background:#E9F1FE; }
QLabel#stateChip[chipState="faulted"] { color:#C0392B; background:#FDEBEA; }
QLabel#stateChip[chipState="offline"] { color:#7A8699; background:#EEF1F6; }
QLabel#stateChip[chipState="booting"] { color:#B26A00; background:#FFF3E0; }
QLabel#stateChip[chipState="unknown"] { color:#7A8699; background:#EEF1F6; }
QProgressBar { background:#E9EDF4; border:none; border-radius:4px; }
QProgressBar::chunk { border-radius:4px; }
QProgressBar#powerBar::chunk { background:#4C8DFF; }
QProgressBar#sessionBar::chunk { background:#34C77B; }
QPushButton { background:#FFFFFF; border:1px solid #D4DCE8; border-radius:6px;
              padding:5px 12px; color:#26324B; font-size:12px; }
QPushButton:hover { border-color:#4C8DFF; color:#4C8DFF; }
QPushButton:disabled { color:#B9C2CF; background:#F2F4F8; border-color:#E5E9F0; }
QPushButton#accent { background:#4C8DFF; color:#FFFFFF; border:none; }
QPushButton#accent:hover { background:#3B7CE8; color:#FFFFFF; }
QPushButton#stopBtn { color:#D64545; }
QPushButton#stopBtn:hover { border-color:#D64545; color:#D64545; }
QPlainTextEdit { background:#222834; color:#D9E2F0; border:none; border-radius:8px;
                 padding:4px; font-family:'DejaVu Sans Mono','Courier New',monospace;
                 font-size:12px; }
QScrollArea { border:none; background:transparent; }
QScrollBar:vertical { background:transparent; width:8px; margin:0; }
QScrollBar::handle:vertical { background:#C7D0DE; border-radius:4px; min-height:24px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }
QCheckBox { color:#44506A; font-size:12px; }
QLineEdit { background:#FFFFFF; border:1px solid #D4DCE8; border-radius:6px;
            padding:4px 8px; color:#26324B; font-size:12px; }
QLineEdit:focus { border-color:#4C8DFF; }
)QSS");
}

// 查找外部 pile_app 可执行文件(平台与桩通常同目录平级部署)
QString pileAppExecutable()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList searchDirs = {QDir(appDir).filePath(QStringLiteral("..")), appDir};
    const QStringList names = {QStringLiteral("pile_app"), QStringLiteral("pile_app.exe")};
    for (const QString &dir : searchDirs) {
        for (const QString &name : names) {
            const QString candidate = QDir(dir).filePath(name);
            if (QFileInfo::exists(candidate))
                return QDir::cleanPath(candidate);
        }
    }
    return {};
}

QString stateColor(const QString &wire)
{
    if (wire == QLatin1String("Charging"))
        return QStringLiteral("#2F80ED");
    if (wire == QLatin1String("Idle"))
        return QStringLiteral("#27AE60");
    if (wire == QLatin1String("Faulted"))
        return QStringLiteral("#EB5757");
    if (wire == QLatin1String("Offline"))
        return QStringLiteral("#B0B7C3");
    if (wire == QLatin1String("Booting"))
        return QStringLiteral("#F2994A");
    return QStringLiteral("#8A94A6");
}

} // namespace

PlatformMainWindow::PlatformMainWindow(PlatformController *controller, quint16 httpPort,
                                       QWidget *parent)
    : QMainWindow(parent)
    , m_controller(controller)
    , m_httpPort(httpPort)
{
    setWindowTitle(QStringLiteral("充电桩平台监控台 - 演示测试"));
    resize(1080, 640);
    setStyleSheet(demoQss());

    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(14, 12, 14, 12);
    root->setSpacing(10);

    auto *topBar = new QHBoxLayout;
    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setObjectName(QStringLiteral("title"));
    topBar->addWidget(m_summaryLabel);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索:桩编号 / 电站 / 型号 / 状态"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFixedWidth(230);
    topBar->addWidget(m_searchEdit);
    topBar->addStretch();
    auto *targetCaption = new QLabel(QStringLiteral("本次目标电量"), this);
    targetCaption->setObjectName(QStringLiteral("muted"));
    topBar->addWidget(targetCaption);
    m_targetSpin = new QDoubleSpinBox(this);
    m_targetSpin->setDecimals(2);
    m_targetSpin->setRange(0.01, 1.00);
    m_targetSpin->setSingleStep(0.05);
    m_targetSpin->setValue(kDefaultSessionKwh);
    m_targetSpin->setSuffix(QStringLiteral(" kWh"));
    m_targetSpin->setToolTip(QStringLiteral("达到该电量后平台自动下发停止充电,"
                                             "调小可加快演示"));
    topBar->addWidget(m_targetSpin);
    m_autoStopBox = new QCheckBox(QStringLiteral("自动停止"), this);
    m_autoStopBox->setChecked(true);
    topBar->addWidget(m_autoStopBox);
    root->addLayout(topBar);

    auto *body = new QHBoxLayout;
    body->setSpacing(12);

    m_cardsHost = new QWidget(this);
    m_cardsLayout = new QVBoxLayout(m_cardsHost);
    m_cardsLayout->setContentsMargins(2, 2, 6, 2);
    m_cardsLayout->setSpacing(10);
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setWidget(m_cardsHost);
    scroll->setFrameShape(QFrame::NoFrame);
    body->addWidget(scroll, 1);

    auto *logHost = new QWidget(this);
    auto *logPanel = new QVBoxLayout(logHost);
    logPanel->setContentsMargins(0, 0, 0, 0);
    logPanel->setSpacing(6);
    auto *logTitle = new QLabel(QStringLiteral("运行日志"), logHost);
    logTitle->setObjectName(QStringLiteral("title"));
    logPanel->addWidget(logTitle);
    m_log = new QPlainTextEdit(logHost);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(2000);
    logPanel->addWidget(m_log, 1);
    logHost->setFixedWidth(350);
    body->addWidget(logHost);

    root->addLayout(body, 1);

    connect(m_controller, &PlatformController::pilesChanged, this,
            &PlatformMainWindow::refreshAll);
    connect(m_controller, &PlatformController::logMessage, this,
            &PlatformMainWindow::appendLog);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &PlatformMainWindow::refreshAll);

    refreshAll();
}

PlatformMainWindow::~PlatformMainWindow()
{
    const auto codes = m_pileWindows.keys();
    for (const QString &code : codes) {
        PileMainWindow *w = m_pileWindows.take(code);
        delete w;
    }
}

PlatformMainWindow::PileCard PlatformMainWindow::createCard(const PileRuntimeInfo &info)
{
    PileCard card;
    card.frame = new QFrame(m_cardsHost);
    card.frame->setObjectName(QStringLiteral("pileCard"));
    auto *lay = new QVBoxLayout(card.frame);
    lay->setContentsMargins(14, 10, 14, 10);
    lay->setSpacing(6);

    auto *head = new QHBoxLayout;
    card.stateDot = new QLabel(card.frame);
    card.stateDot->setFixedSize(12, 12);
    head->addWidget(card.stateDot);
    auto *codeLabel = new QLabel(info.pileCode, card.frame);
    codeLabel->setObjectName(QStringLiteral("title"));
    head->addWidget(codeLabel);
    auto *modelLabel = new QLabel(info.model, card.frame);
    modelLabel->setObjectName(QStringLiteral("muted"));
    head->addWidget(modelLabel);
    head->addStretch();
    card.stateText = new QLabel(card.frame);
    card.stateText->setObjectName(QStringLiteral("stateChip"));
    head->addWidget(card.stateText);
    lay->addLayout(head);

    auto *powerRow = new QHBoxLayout;
    auto *powerCaption = new QLabel(QStringLiteral("输出功率"), card.frame);
    powerCaption->setObjectName(QStringLiteral("muted"));
    powerCaption->setFixedWidth(64);
    card.powerBar = new QProgressBar(card.frame);
    card.powerBar->setObjectName(QStringLiteral("powerBar"));
    card.powerBar->setRange(0, 100);
    card.powerBar->setTextVisible(false);
    card.powerBar->setFixedHeight(8);
    card.powerText = new QLabel(card.frame);
    card.powerText->setObjectName(QStringLiteral("value"));
    card.powerText->setFixedWidth(118);
    card.powerText->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    powerRow->addWidget(powerCaption);
    powerRow->addWidget(card.powerBar, 1);
    powerRow->addWidget(card.powerText);
    lay->addLayout(powerRow);

    auto *sessionRow = new QHBoxLayout;
    auto *sessionCaption = new QLabel(QStringLiteral("本次进度"), card.frame);
    sessionCaption->setObjectName(QStringLiteral("muted"));
    sessionCaption->setFixedWidth(64);
    card.sessionBar = new QProgressBar(card.frame);
    card.sessionBar->setObjectName(QStringLiteral("sessionBar"));
    card.sessionBar->setRange(0, 100);
    card.sessionBar->setTextVisible(false);
    card.sessionBar->setFixedHeight(8);
    card.sessionText = new QLabel(card.frame);
    card.sessionText->setObjectName(QStringLiteral("value"));
    card.sessionText->setFixedWidth(200);
    card.sessionText->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sessionRow->addWidget(sessionCaption);
    sessionRow->addWidget(card.sessionBar, 1);
    sessionRow->addWidget(card.sessionText);
    lay->addLayout(sessionRow);

    card.metaText = new QLabel(card.frame);
    card.metaText->setObjectName(QStringLiteral("muted"));
    lay->addWidget(card.metaText);

    card.rollingText = new QLabel(card.frame);
    card.rollingText->setObjectName(QStringLiteral("muted"));
    lay->addWidget(card.rollingText);

    auto *btnRow = new QHBoxLayout;
    card.startBtn = new QPushButton(QStringLiteral("开始充电"), card.frame);
    card.startBtn->setObjectName(QStringLiteral("accent"));
    card.stopBtn = new QPushButton(QStringLiteral("停止充电"), card.frame);
    card.stopBtn->setObjectName(QStringLiteral("stopBtn"));
    card.resetBtn = new QPushButton(QStringLiteral("远程重启"), card.frame);
    btnRow->addWidget(card.startBtn);
    btnRow->addWidget(card.stopBtn);
    btnRow->addWidget(card.resetBtn);
    card.openBtn = new QPushButton(QStringLiteral("打开桩窗口"), card.frame);
    btnRow->addWidget(card.openBtn);
    btnRow->addStretch();
    lay->addLayout(btnRow);

    const QString code = info.pileCode;
    const QString model = info.model;
    connect(card.startBtn, &QPushButton::clicked, this,
            [this, code]() { m_controller->remoteStart(code); });
    connect(card.stopBtn, &QPushButton::clicked, this,
            [this, code]() { m_controller->remoteStop(code); });
    connect(card.resetBtn, &QPushButton::clicked, this,
            [this, code]() { m_controller->remoteReset(code); });
    connect(card.openBtn, &QPushButton::clicked, this,
            [this, code, model]() { openPileApp(code, model); });

    return card;
}

void PlatformMainWindow::attachDemoPiles(DemoPiles *demoPiles)
{
    m_demoPiles = demoPiles;
}

void PlatformMainWindow::updateCard(PileCard &card, const PileRuntimeInfo &info)
{
    const QString wireStatus = info.online ? info.status : QStringLiteral("Offline");
    const bool online = info.online;
    const bool charging = online && info.status == QLatin1String("Charging");
    const double targetKwh = m_targetSpin ? m_targetSpin->value() : kDefaultSessionKwh;

    // 识别"进入充电",记录本段起点,用于计算本次充电进度
    if (charging && card.lastStatus != QLatin1String("Charging")) {
        card.baseEnergyKwh = info.energyKwh;
        card.powerHistory.clear();
    }
    if (!charging)
        card.stopRequested = false;
    card.lastStatus = info.status;

    const double deltaKwh = charging ? qMax(0.0, info.energyKwh - card.baseEnergyKwh) : 0.0;
    const double progressPct = charging
                                   ? qBound(0.0, deltaKwh / qMax(0.001, targetKwh) * 100.0, 100.0)
                                   : 0.0;

    if (charging && progressPct >= 100.0 && m_autoStopBox->isChecked() && !card.stopRequested) {
        card.stopRequested = true;
        appendLog(QStringLiteral("桩 %1 本次充电已达演示目标,自动下发停止充电")
                      .arg(info.pileCode));
        m_controller->remoteStop(info.pileCode);
    }

    // 收集功率滚动序列(仅充电中的真实读数,去重心跳等重复事件)
    if (charging) {
        if (card.powerHistory.isEmpty()
            || qAbs(card.powerHistory.last() - info.powerKw) > kPowerEpsilon)
            card.powerHistory.append(info.powerKw);
        while (card.powerHistory.size() > 12)
            card.powerHistory.removeFirst();
    }

    const double powerPct = qBound(0.0, info.powerKw / kRatedPowerKw * 100.0, 100.0);
    card.powerBar->setValue(qRound(powerPct));
    card.powerText->setText(QStringLiteral("%1 / %2 kW")
                                .arg(info.powerKw, 0, 'f', 2)
                                .arg(kRatedPowerKw, 0, 'f', 0));

    card.sessionBar->setValue(qRound(progressPct));
    if (charging) {
        card.sessionText->setText(QStringLiteral("%1 / %2 kWh · %3%")
                                      .arg(deltaKwh, 0, 'f', 4)
                                      .arg(targetKwh, 0, 'f', 2)
                                      .arg(qRound(progressPct)));
    } else {
        card.sessionText->setText(QStringLiteral("--"));
    }

    card.stateDot->setStyleSheet(QStringLiteral("background:%1;border-radius:6px;")
                                     .arg(stateColor(wireStatus)));
    card.stateText->setText(PlatformController::statusDisplayText(wireStatus));
    card.stateText->setProperty("chipState", wireStatus.toLower());
    auto *chipStyle = card.stateText->style();
    chipStyle->unpolish(card.stateText);
    chipStyle->polish(card.stateText);

    card.frame->setProperty("offline", !online);
    auto *frameStyle = card.frame->style();
    frameStyle->unpolish(card.frame);
    frameStyle->polish(card.frame);

    const QString hb = info.lastHeartbeat.isValid()
                           ? info.lastHeartbeat.toString(QStringLiteral("HH:mm:ss"))
                           : QStringLiteral("--:--:--");
    QString meta;
    if (!info.stationName.isEmpty())
        meta = QStringLiteral("电站 %1  ·  ").arg(info.stationName);
    meta += QStringLiteral("累计电量 %1 kWh  ·  最后心跳 %2")
                .arg(info.energyKwh, 0, 'f', 4)
                .arg(hb);
    card.metaText->setText(meta);

    QString rolling;
    const int start = qMax(0, card.powerHistory.size() - 8);
    for (int i = start; i < card.powerHistory.size(); ++i) {
        if (!rolling.isEmpty())
            rolling += QStringLiteral(" → ");
        rolling += QString::number(card.powerHistory.at(i), 'f', 2);
    }
    card.rollingText->setText(rolling.isEmpty()
                                  ? QStringLiteral("功率序列: --")
                                  : QStringLiteral("功率序列: %1").arg(rolling));

    card.startBtn->setEnabled(online && info.status == QLatin1String("Idle"));
    card.stopBtn->setEnabled(charging);
    card.resetBtn->setEnabled(online);

    // 打开桩窗口:内置演示桩始终可开;真实离线桩拉起外部 pile_app
    const bool internalPile =
        m_demoPiles && m_demoPiles->pileController(info.pileCode) != nullptr;
    const bool showOpen = internalPile || !online;
    card.openBtn->setVisible(showOpen);
    card.openBtn->setEnabled(showOpen);
    card.openBtn->setToolTip(internalPile
                                 ? QStringLiteral("打开此内置桩对应的桩端窗口")
                                 : QStringLiteral("启动 pile_app 模拟器并连接本平台"));
}

void PlatformMainWindow::openPileApp(const QString &pileCode, const QString &model)
{
    if (m_pileWindows.contains(pileCode)) {
        PileMainWindow *existing = m_pileWindows.value(pileCode);
        if (existing) {
            existing->show();
            existing->raise();
            existing->activateWindow();
            return;
        }
        m_pileWindows.remove(pileCode);
    }

    // 内置演示桩:直接复用其控制器打开桩端窗口(同一连接,双向实时联动)
    PileController *pile =
        m_demoPiles ? m_demoPiles->pileController(pileCode) : nullptr;
    if (pile) {
        auto *w = new PileMainWindow(pile, nullptr);
        w->setAttribute(Qt::WA_DeleteOnClose);
        connect(w, &QObject::destroyed, this, [this, pileCode]() {
            m_pileWindows.remove(pileCode);
        });
        m_pileWindows.insert(pileCode, w);
        w->show();
        appendLog(QStringLiteral("已打开内置桩 %1 的桩端窗口").arg(pileCode));
        return;
    }

    // 真实桩(通常为离线状态):拉起外部 pile_app 重新连接本平台
    const QString exe = pileAppExecutable();
    if (exe.isEmpty()) {
        appendLog(QStringLiteral("未找到 pile_app 可执行文件,无法打开桩窗口"));
        return;
    }
    QStringList args;
    args << QStringLiteral("--code") << pileCode << QStringLiteral("--url")
         << QStringLiteral("ws://127.0.0.1:%1").arg(m_controller->wsPort());
    if (!model.isEmpty())
        args << QStringLiteral("--model") << model;
    if (QProcess::startDetached(exe, args)) {
        appendLog(QStringLiteral("已启动桩模拟器 %1").arg(exe));
    } else {
        appendLog(QStringLiteral("启动桩模拟器失败:%1").arg(exe));
    }
}

void PlatformMainWindow::refreshAll()
{
    // 关键字过滤:按 桩编号 / 型号 / 所属电站 / 界面状态 实时匹配
    const QString keyword = m_searchEdit ? m_searchEdit->text().trimmed() : QString();
    QList<PileRuntimeInfo> shown;
    for (const auto &info : m_controller->allPiles()) {
        if (keyword.isEmpty()) {
            shown.append(info);
        } else {
            const QString wire = info.online ? info.status : QStringLiteral("Offline");
            const bool hit = info.pileCode.contains(keyword, Qt::CaseInsensitive)
                             || info.model.contains(keyword, Qt::CaseInsensitive)
                             || info.stationName.contains(keyword, Qt::CaseInsensitive)
                             || PlatformController::statusDisplayText(wire)
                                    .contains(keyword, Qt::CaseInsensitive);
            if (hit)
                shown.append(info);
        }
    }

    QStringList shownCodes;
    for (const auto &info : shown)
        shownCodes.append(info.pileCode);

    // 移除不匹配(或已不存在)的卡片
    for (const QString &code : m_cards.keys()) {
        if (!shownCodes.contains(code)) {
            m_cards.value(code).frame->deleteLater();
            m_cards.remove(code);
        }
    }
    // 补建新卡片
    for (const auto &info : shown) {
        if (!m_cards.contains(info.pileCode))
            m_cards.insert(info.pileCode, createCard(info));
    }

    // 按桩编号排序后重新排列卡片,并刷新内容
    while (m_cardsLayout->count() > 0)
        delete m_cardsLayout->takeAt(0);

    int onlineCount = 0;
    for (const auto &info : shown) {
        if (info.online)
            ++onlineCount;
        updateCard(m_cards[info.pileCode], info);
        m_cardsLayout->addWidget(m_cards[info.pileCode].frame);
    }
    m_cardsLayout->addStretch(1);

    QString summary = QStringLiteral("监控台 ｜ WS %1 ｜ HTTP %2 ｜ 在线 %3/%4 台")
                          .arg(m_controller->wsPort())
                          .arg(m_httpPort)
                          .arg(onlineCount)
                          .arg(shown.size());
    if (!keyword.isEmpty())
        summary += QStringLiteral("  ｜ 搜索“%1”,匹配 %2 台")
                       .arg(keyword)
                       .arg(shown.size());
    m_summaryLabel->setText(summary);
}

void PlatformMainWindow::appendLog(const QString &text)
{
    m_log->appendPlainText(QStringLiteral("[%1] %2")
                               .arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss")),
                                    text));
}
