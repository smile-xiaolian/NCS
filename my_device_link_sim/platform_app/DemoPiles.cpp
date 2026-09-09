#include "DemoPiles.h"
#include "PileController.h"

#include <QStringList>
#include <QtGlobal>

DemoPiles::DemoPiles(quint16 wsPort, int count, QObject *parent)
    : QObject(parent)
    , m_serverUrl(QStringLiteral("ws://127.0.0.1:%1").arg(wsPort))
{
    static const QStringList models = {QStringLiteral("AC-7kW"), QStringLiteral("DC-120kW"),
                                       QStringLiteral("AC-22kW"), QStringLiteral("DC-60kW"),
                                       QStringLiteral("AC-7kW")};
    const int n = qBound(1, count, 8);
    m_seeds.reserve(n);
    for (int i = 0; i < n; ++i) {
        m_seeds.append(PileSeed{QStringLiteral("DEMO-%1").arg(i + 1, 3, 10, QLatin1Char('0')),
                                models.at(i % models.size())});
    }
}

DemoPiles::DemoPiles(quint16 wsPort, const QList<PileSeed> &seeds, QObject *parent)
    : QObject(parent)
    , m_serverUrl(QStringLiteral("ws://127.0.0.1:%1").arg(wsPort))
    , m_seeds(seeds)
{
    if (m_seeds.isEmpty())
        m_seeds.append(PileSeed{QStringLiteral("DEMO-001"), QStringLiteral("AC-7kW")});
}

void DemoPiles::start()
{
    for (const PileSeed &seed : m_seeds) {
        auto *pile = new PileController(seed.code, seed.model, m_serverUrl, this);
        connect(pile, &PileController::logMessage, this,
                [this, code = seed.code](const QString &text) {
                    emit logMessage(QStringLiteral("虚拟桩 %1:%2").arg(code, text));
                });
        m_piles.append(pile);
        pile->start();
    }
    emit logMessage(QStringLiteral("演示模式已启动 %1 台内置虚拟桩,"
                                   "桩将自动连接平台并完成开机注册")
                        .arg(m_seeds.size()));
}

PileController *DemoPiles::pileController(const QString &code) const
{
    for (PileController *pile : m_piles) {
        if (pile && pile->pileCode() == code)
            return pile;
    }
    return nullptr;
}