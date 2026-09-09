#pragma once

#include <QHash>
#include <QMainWindow>
#include <QString>
#include <QVector>

class QCheckBox;
class QDoubleSpinBox;
class QFrame;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QVBoxLayout;
class QWidget;
class DemoPiles;
class PileMainWindow;
class PlatformController;
struct PileRuntimeInfo;

// 平台端演示监控台:每台桩一张卡片,实时展示连接/状态/功率/充电进度与心跳,
// 卡片内可直接下发开始/停止/重启指令,右侧为运行日志。
class PlatformMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit PlatformMainWindow(PlatformController *controller, quint16 httpPort,
                                QWidget *parent = nullptr);
    ~PlatformMainWindow() override;

    // 注册演示模式的内置虚拟桩组,用于打开对应桩端窗口
    void attachDemoPiles(DemoPiles *demoPiles);

private:
    struct PileCard {
        QFrame *frame = nullptr;
        QLabel *stateDot = nullptr;
        QLabel *stateText = nullptr;
        QLabel *powerText = nullptr;
        QLabel *sessionText = nullptr;
        QLabel *metaText = nullptr;
        QLabel *rollingText = nullptr;
        QProgressBar *powerBar = nullptr;
        QProgressBar *sessionBar = nullptr;
        QPushButton *startBtn = nullptr;
        QPushButton *stopBtn = nullptr;
        QPushButton *resetBtn = nullptr;
        QPushButton *openBtn = nullptr;
        QVector<double> powerHistory; // 近若干次功率读数(滚动展示)
        double baseEnergyKwh = 0.0;  // 本次充电开始时累计电量
        QString lastStatus;          // 上一帧协议状态(用于识别切换)
        bool stopRequested = false;  // 已自动下发停止,防重复
    };

    void refreshAll();
    PileCard createCard(const PileRuntimeInfo &info);
    void updateCard(PileCard &card, const PileRuntimeInfo &info);
    void openPileApp(const QString &pileCode, const QString &model);
    void appendLog(const QString &text);

    PlatformController *m_controller = nullptr;
    quint16 m_httpPort = 0;
    QHash<QString, PileCard> m_cards;
    QHash<QString, PileMainWindow *> m_pileWindows;
    DemoPiles *m_demoPiles = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QLabel *m_summaryLabel = nullptr;
    QCheckBox *m_autoStopBox = nullptr;
    QDoubleSpinBox *m_targetSpin = nullptr;
    QWidget *m_cardsHost = nullptr;
    QVBoxLayout *m_cardsLayout = nullptr;
    QPlainTextEdit *m_log = nullptr;
};
