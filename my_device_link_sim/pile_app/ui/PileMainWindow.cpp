#include "ui/PileMainWindow.h"
#include "MockPlatform.h"
#include "PileController.h"

#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QStyle>
#include <QTime>
#include <QVBoxLayout>
#include <QWidget>

namespace {

constexpr double kRatedPowerKw = 10.0; // 功率条满量程(按 10 kW 桩)

QString pileQss()
{
    return QStringLiteral(R"QSS(
QMainWindow { background:#EEF1F6; }
QLabel#title { color:#1F2733; font-size:15px; font-weight:700; }
QLabel#muted { color:#7A8699; font-size:12px; }
QLabel#value { color:#1F2733; font-size:14px; font-weight:700; }
QLabel#stateChip { padding:2px 10px; border-radius:9px; font-size:12px; font-weight:600; }
QLabel#stateChip[chipState="idle"] { color:#1E8E5A; background:#E6F7EF; }
QLabel#stateChip[chipState="charging"] { color:#1F6FD0; background:#E9F1FE; }
QLabel#stateChip[chipState="faulted"] { color:#C0392B; background:#FDEBEA; }
QLabel#stateChip[chipState="booting"] { color:#B26A00; background:#FFF3E0; }
QLabel#stateChip[chipState="unknown"] { color:#7A8699; background:#EEF1F6; }
QProgressBar { background:#E9EDF4; border:none; border-radius:5px; }
QProgressBar::chunk { border-radius:5px; }
QProgressBar#powerBar::chunk { background:#4C8DFF; }
QPushButton { background:#FFFFFF; border:1px solid #D4DCE8; border-radius:6px;
              padding:5px 12px; color:#26324B; font-size:12px; }
QPushButton:hover { border-color:#4C8DFF; color:#4C8DFF; }
QPushButton:disabled { color:#B9C2CF; background:#F2F4F8; border-color:#E5E9F0; }
QPushButton#accent { background:#4C8DFF; color:#FFFFFF; border:none; }
QPushButton#accent:hover { background:#3B7CE8; color:#FFFFFF; }
QPushButton#danger { color:#D64545; }
QPushButton#danger:hover { border-color:#D64545; color:#D64545; }
QPlainTextEdit { background:#222834; color:#D9E2F0; border:none; border-radius:8px;
                 padding:4px; font-family:'DejaVu Sans Mono','Courier New',monospace;
                 font-size:12px; }
)QSS");
}

QString stateColor(PileController::State state)
{
    switch (state) {
    case PileController::State::Booting:  return QStringLiteral("#F2994A");
    case PileController::State::Idle:     return QStringLiteral("#27AE60");
    case PileController::State::Charging: return QStringLiteral("#2F80ED");
    case PileController::State::Faulted:  return QStringLiteral("#EB5757");
    }
    return QStringLiteral("#8A94A6");
}

QString chipKey(PileController::State state)
{
    switch (state) {
    case PileController::State::Booting:  return QStringLiteral("booting");
    case PileController::State::Idle:     return QStringLiteral("idle");
    case PileController::State::Charging: return QStringLiteral("charging");
    case PileController::State::Faulted:  return QStringLiteral("faulted");
    }
    return QStringLiteral("unknown");
}

} // namespace

PileMainWindow::PileMainWindow(PileController *controller, MockPlatform *mock, QWidget *parent)
    : QMainWindow(parent)
    , m_controller(controller)
    , m_mock(mock)
{
    setWindowTitle(QStringLiteral("充电桩模拟器 - %1").arg(controller->pileCode()));
    resize(600, 560);
    setStyleSheet(pileQss());

    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(16, 14, 16, 12);
    root->setSpacing(10);

    auto *head = new QHBoxLayout;
    m_codeLabel = new QLabel(this);
    m_codeLabel->setObjectName(QStringLiteral("title"));
    head->addWidget(m_codeLabel);
    head->addStretch();
    m_connLabel = new QLabel(this);
    m_connLabel->setObjectName(QStringLiteral("muted"));
    head->addWidget(m_connLabel);
    root->addLayout(head);

    auto *statusRow = new QHBoxLayout;
    statusRow->addStretch();
    m_stateDot = new QLabel(this);
    m_stateDot->setFixedSize(14, 14);
    statusRow->addWidget(m_stateDot);
    m_stateLabel = new QLabel(this);
    m_stateLabel->setObjectName(QStringLiteral("stateChip"));
    statusRow->addWidget(m_stateLabel);
    statusRow->addStretch();
    root->addLayout(statusRow);

    auto *powerRow = new QHBoxLayout;
    auto *powerCaption = new QLabel(QStringLiteral("输出功率"), this);
    powerCaption->setObjectName(QStringLiteral("muted"));
    powerCaption->setFixedWidth(64);
    m_powerBar = new QProgressBar(this);
    m_powerBar->setObjectName(QStringLiteral("powerBar"));
    m_powerBar->setRange(0, 100);
    m_powerBar->setTextVisible(false);
    m_powerBar->setFixedHeight(10);
    m_powerLabel = new QLabel(this);
    m_powerLabel->setObjectName(QStringLiteral("value"));
    m_powerLabel->setFixedWidth(130);
    m_powerLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    powerRow->addWidget(powerCaption);
    powerRow->addWidget(m_powerBar, 1);
    powerRow->addWidget(m_powerLabel);
    root->addLayout(powerRow);

    m_energyLabel = new QLabel(this);
    m_energyLabel->setObjectName(QStringLiteral("muted"));
    root->addWidget(m_energyLabel);

    m_historyLabel = new QLabel(this);
    m_historyLabel->setObjectName(QStringLiteral("muted"));
    m_historyLabel->setWordWrap(true);
    root->addWidget(m_historyLabel);

    auto *ops = new QHBoxLayout;
    m_disconnectBtn = new QPushButton(QStringLiteral("手动断线"), this);
    m_faultBtn = new QPushButton(QStringLiteral("模拟故障"), this);
    m_recoverBtn = new QPushButton(QStringLiteral("恢复正常"), this);
    m_fullBtn = new QPushButton(QStringLiteral("模拟充满"), this);
    m_fullBtn->setObjectName(QStringLiteral("accent"));
    ops->addWidget(m_disconnectBtn);
    ops->addWidget(m_faultBtn);
    ops->addStretch();
    ops->addWidget(m_recoverBtn);
    ops->addWidget(m_fullBtn);
    root->addLayout(ops);

    if (m_mock) {
        auto *mockCaption = new QLabel(QStringLiteral("模拟平台下发指令(自测演示)"), this);
        mockCaption->setObjectName(QStringLiteral("muted"));
        root->addWidget(mockCaption);
        auto *mockOps = new QHBoxLayout;
        auto *startCmd = new QPushButton(QStringLiteral("开始充电"), this);
        startCmd->setObjectName(QStringLiteral("accent"));
        auto *stopCmd = new QPushButton(QStringLiteral("停止充电"), this);
        stopCmd->setObjectName(QStringLiteral("danger"));
        auto *resetCmd = new QPushButton(QStringLiteral("远程重启"), this);
        mockOps->addWidget(startCmd);
        mockOps->addWidget(stopCmd);
        mockOps->addWidget(resetCmd);
        mockOps->addStretch();
        root->addLayout(mockOps);

        connect(startCmd, &QPushButton::clicked, m_mock, [this]() {
            m_mock->sendToPile(QStringLiteral("RemoteStartTransaction"),
                               QJsonObject{{QStringLiteral("connectorId"), 1}});
        });
        connect(stopCmd, &QPushButton::clicked, m_mock, [this]() {
            m_mock->sendToPile(QStringLiteral("RemoteStopTransaction"), QJsonObject{});
        });
        connect(resetCmd, &QPushButton::clicked, m_mock, [this]() {
            m_mock->sendToPile(QStringLiteral("RemoteReset"),
                               QJsonObject{{QStringLiteral("type"), QStringLiteral("Hard")}});
        });
        connect(m_mock, &MockPlatform::logMessage, this, &PileMainWindow::appendLog);
    }

    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    root->addWidget(m_log, 1);

    connect(m_disconnectBtn, &QPushButton::clicked, m_controller,
            &PileController::injectDisconnect);
    connect(m_faultBtn, &QPushButton::clicked, m_controller, &PileController::injectFault);
    connect(m_recoverBtn, &QPushButton::clicked, m_controller,
            &PileController::recoverFromFault);
    connect(m_fullBtn, &QPushButton::clicked, m_controller,
            &PileController::simulateFullCharge);

    connect(m_controller, &PileController::connectionChanged, this,
            [this](bool) { refresh(); });
    connect(m_controller, &PileController::stateChanged, this,
            [this](PileController::State) { refresh(); });
    connect(m_controller, &PileController::meterUpdated, this,
            [this](double powerKw, double) {
                m_powerHistory.append(powerKw);
                while (m_powerHistory.size() > 12)
                    m_powerHistory.removeFirst();
                refresh();
            });
    connect(m_controller, &PileController::logMessage, this, &PileMainWindow::appendLog);

    refresh();
}

void PileMainWindow::refresh()
{
    const bool connected = m_controller->isConnected();
    const PileController::State state = m_controller->state();

    m_codeLabel->setText(QStringLiteral("充电桩 %1").arg(m_controller->pileCode()));
    m_connLabel->setText(connected ? QStringLiteral("● 已连接平台")
                                   : QStringLiteral("○ 未连接平台"));
    m_connLabel->setStyleSheet(connected ? QStringLiteral("color:#27AE60;")
                                         : QStringLiteral("color:#B0B7C3;"));

    m_stateDot->setStyleSheet(QStringLiteral("background:%1;border-radius:7px;")
                                  .arg(stateColor(state)));
    m_stateLabel->setText(m_controller->stateDisplayText());
    m_stateLabel->setProperty("chipState", chipKey(state));
    auto *style = m_stateLabel->style();
    style->unpolish(m_stateLabel);
    style->polish(m_stateLabel);

    const double powerKw = m_controller->powerKw();
    const double pct = qBound(0.0, powerKw / kRatedPowerKw * 100.0, 100.0);
    m_powerBar->setValue(qRound(pct));
    m_powerLabel->setText(QStringLiteral("%1 / %2 kW")
                              .arg(powerKw, 0, 'f', 2)
                              .arg(kRatedPowerKw, 0, 'f', 0));
    m_energyLabel->setText(QStringLiteral("累计电量 %1 kWh")
                               .arg(m_controller->energyKwh(), 0, 'f', 4));

    QString history;
    const int start = qMax(0, m_powerHistory.size() - 10);
    for (int i = start; i < m_powerHistory.size(); ++i) {
        if (!history.isEmpty())
            history += QStringLiteral(" → ");
        history += QString::number(m_powerHistory.at(i), 'f', 2);
    }
    m_historyLabel->setText(history.isEmpty()
                                ? QStringLiteral("功率序列: --")
                                : QStringLiteral("功率序列: %1").arg(history));

    m_disconnectBtn->setEnabled(connected);
    m_faultBtn->setEnabled(connected && state != PileController::State::Faulted);
    m_recoverBtn->setEnabled(connected && state == PileController::State::Faulted);
    m_fullBtn->setEnabled(connected && state == PileController::State::Charging);
}

void PileMainWindow::appendLog(const QString &text)
{
    m_log->appendPlainText(QStringLiteral("[%1] %2")
                               .arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss")),
                                    text));
}
