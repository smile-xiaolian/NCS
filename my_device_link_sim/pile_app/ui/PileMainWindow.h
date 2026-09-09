#pragma once

#include <QMainWindow>
#include <QVector>

class QLabel;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class MockPlatform;
class PileController;

// 桩端演示界面:状态/功率可视化 + 断线、故障注入与恢复操作。
class PileMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit PileMainWindow(PileController *controller, MockPlatform *mock = nullptr,
                            QWidget *parent = nullptr);

private:
    void refresh();
    void appendLog(const QString &text);

    PileController *m_controller = nullptr;
    MockPlatform *m_mock = nullptr;
    QLabel *m_codeLabel = nullptr;
    QLabel *m_connLabel = nullptr;
    QLabel *m_stateDot = nullptr;
    QLabel *m_stateLabel = nullptr;
    QLabel *m_powerLabel = nullptr;
    QLabel *m_energyLabel = nullptr;
    QLabel *m_historyLabel = nullptr;
    QProgressBar *m_powerBar = nullptr;
    QPlainTextEdit *m_log = nullptr;
    QPushButton *m_disconnectBtn = nullptr;
    QPushButton *m_faultBtn = nullptr;
    QPushButton *m_recoverBtn = nullptr;
    QPushButton *m_fullBtn = nullptr;
    QVector<double> m_powerHistory; // 最近若干次功率读数(滚动展示)
};
