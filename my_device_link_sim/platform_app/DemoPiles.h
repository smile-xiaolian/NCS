#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>

class PileController;

// 内置虚拟桩组:在平台进程内模拟 N 台桩,每台桩作为 WebSocket 客户端
// 主动连回平台自身端口,按与 pile_app 完全相同的桩状态机运行。
// 用于没有真实桩、也不想另开 pile_app 窗口时的独立演示与联调测试。
class DemoPiles : public QObject {
    Q_OBJECT
public:
    // 一台内置虚拟桩的启动参数(编号 + 型号)
    struct PileSeed {
        QString code;
        QString model;
    };

    // 按数量生成内置虚拟桩(编号 DEMO-001、DEMO-002...)
    explicit DemoPiles(quint16 wsPort, int count = 3, QObject *parent = nullptr);
    // 按给定清单生成内置虚拟桩(如 NCS 项目数据库中的电桩 S1-01...)
    explicit DemoPiles(quint16 wsPort, const QList<PileSeed> &seeds, QObject *parent = nullptr);

    void start();
    // 按桩编号取内置桩控制器(用于平台端打开对应桩窗口)
    PileController *pileController(const QString &code) const;

signals:
    void logMessage(const QString &text);

private:
    QUrl m_serverUrl;
    QList<PileSeed> m_seeds;
    QList<PileController *> m_piles;
};