#ifndef CHARGINGPROGRESSWIDGET_H
#define CHARGINGPROGRESSWIDGET_H

#include <QWidget>
#include <QColor>
#include <QTimer>

class ChargingProgressWidget : public QWidget
{
    Q_OBJECT
public:
    enum class Mode { Idle, Reserved, Charging, Full };

    explicit ChargingProgressWidget(QWidget *parent = nullptr);

    void setProgress(double percent);
    void setMode(Mode mode);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void tickAnimation();
    QColor accentColor() const;

    Mode m_mode = Mode::Idle;
    double m_target = 0.0;
    double m_display = 0.0;
    double m_phase = 0.0;
    QTimer m_animTimer;
};

#endif
