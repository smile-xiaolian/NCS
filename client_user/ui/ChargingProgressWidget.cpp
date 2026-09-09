#include "ChargingProgressWidget.h"

#include <QPainter>
#include <QConicalGradient>
#include <QRadialGradient>
#include <QtMath>

ChargingProgressWidget::ChargingProgressWidget(QWidget *parent) : QWidget(parent)
{
    setFixedSize(220, 220);
    m_animTimer.setInterval(16);
    connect(&m_animTimer, &QTimer::timeout, this, &ChargingProgressWidget::tickAnimation);
    m_animTimer.start();
}

void ChargingProgressWidget::setProgress(double percent)
{
    m_target = qBound(0.0, percent, 100.0);
    if (!m_animTimer.isActive())
        m_animTimer.start();
}

void ChargingProgressWidget::setMode(Mode mode)
{
    if (m_mode == mode) return;
    m_mode = mode;
    if (m_mode == Mode::Charging && !m_animTimer.isActive())
        m_animTimer.start();
    update();
}

QColor ChargingProgressWidget::accentColor() const
{
    switch (m_mode) {
    case Mode::Reserved: return QColor("#f59e0b");
    case Mode::Full:     return QColor("#059669");
    case Mode::Charging: return QColor("#10b981");
    case Mode::Idle:
    default:             return QColor("#94a3b8");
    }
}

void ChargingProgressWidget::tickAnimation()
{
    bool moving = false;
    const double gap = m_target - m_display;
    if (qAbs(gap) > 0.04) {
        m_display += gap * 0.18;
        moving = true;
    } else {
        m_display = m_target;
    }

    if (m_mode == Mode::Charging) {
        m_phase += 0.07;
        moving = true;
    }

    if (moving)
        update();
    else if (m_mode != Mode::Charging)
        m_animTimer.stop();
}

void ChargingProgressWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    const QPointF c(width() / 2.0, height() / 2.0);
    const qreal side = qMin(width(), height());
    const qreal penW = 16.0;
    const qreal radius = side / 2.0 - penW - 8.0;
    const QRectF arc(c.x() - radius, c.y() - radius, radius * 2, radius * 2);
    const QColor accent = accentColor();
    const double percent = m_display;

    if (m_mode == Mode::Charging || m_mode == Mode::Full) {
        const int pulse = (m_mode == Mode::Charging)
            ? int(28 + 18 * qSin(m_phase))
            : 36;
        QRadialGradient halo(c, radius + 28);
        QColor glow = accent;
        glow.setAlpha(pulse);
        halo.setColorAt(0.62, glow);
        halo.setColorAt(1.0, Qt::transparent);
        p.setPen(Qt::NoPen);
        p.setBrush(halo);
        p.drawEllipse(c, radius + 22, radius + 22);
    }

    QRadialGradient plate(c, radius - penW);
    plate.setColorAt(0.0, QColor("#f8fffb"));
    plate.setColorAt(1.0, QColor("#ffffff"));
    p.setPen(Qt::NoPen);
    p.setBrush(plate);
    p.drawEllipse(c, radius - penW / 2.0 - 2.0, radius - penW / 2.0 - 2.0);

    QPen track(QColor("#e8eef4"), penW, Qt::SolidLine, Qt::RoundCap);
    p.setBrush(Qt::NoBrush);
    p.setPen(track);
    p.drawEllipse(arc);

    if (percent > 0.02) {
        const int start = 90 * 16;
        const int span = -int(percent * 3.6 * 16);

        QColor glow = accent;
        glow.setAlpha(70);
        QPen glowPen(glow, penW + 8, Qt::SolidLine, Qt::RoundCap);
        p.setPen(glowPen);
        p.drawArc(arc, start, span);

        QConicalGradient ring(c, 90);
        ring.setColorAt(0.00, accent.lighter(135));
        ring.setColorAt(0.35, accent);
        ring.setColorAt(0.70, accent.darker(115));
        ring.setColorAt(1.00, accent.lighter(135));
        QPen arcPen(QBrush(ring), penW);
        arcPen.setCapStyle(Qt::RoundCap);
        p.setPen(arcPen);
        p.drawArc(arc, start, span);

        const double theta = qDegreesToRadians(90.0 - percent * 3.6);
        const QPointF knob(c.x() + radius * qCos(theta), c.y() - radius * qSin(theta));
        p.setPen(Qt::NoPen);
        QColor knobHalo = accent;
        knobHalo.setAlpha(90);
        p.setBrush(knobHalo);
        p.drawEllipse(knob, 9, 9);
        p.setBrush(Qt::white);
        p.drawEllipse(knob, 6.2, 6.2);
        p.setBrush(accent);
        p.drawEllipse(knob, 4.2, 4.2);
    }

    QFont numberFont = font();
    numberFont.setPixelSize(34);
    numberFont.setBold(true);
    p.setFont(numberFont);
    p.setPen(QColor("#0f172a"));
    p.drawText(QRectF(0, c.y() - 34, width(), 42), Qt::AlignHCenter | Qt::AlignVCenter,
               QString("%1%").arg(percent, 0, 'f', 1));

    QFont captionFont = font();
    captionFont.setPixelSize(12);
    captionFont.setBold(true);
    p.setFont(captionFont);
    p.setPen(accent.darker(110));
    QString caption = QStringLiteral("SoC");
    if (m_mode == Mode::Charging) caption = QStringLiteral("充电中 · SoC");
    else if (m_mode == Mode::Full) caption = QStringLiteral("已充满");
    else if (m_mode == Mode::Reserved) caption = QStringLiteral("待启动");
    else caption = QStringLiteral("待机");
    p.drawText(QRectF(0, c.y() + 12, width(), 22), Qt::AlignHCenter, caption);
}
