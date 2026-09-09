#pragma once

#include <QAbstractButton>
#include <QDialog>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QScreen>
#include <QVBoxLayout>
#include <QWidget>

namespace ncs {

// 指标色板:蓝=常规, 绿=健康/空闲, 橙=进行中, 红=告警/故障, 紫/青=补充信息
inline QString toneTextColor(const QString &tone)
{
    if (tone == QLatin1String("green"))
        return QStringLiteral("#1E9E62");
    if (tone == QLatin1String("orange"))
        return QStringLiteral("#E8890C");
    if (tone == QLatin1String("red"))
        return QStringLiteral("#D64545");
    if (tone == QLatin1String("purple"))
        return QStringLiteral("#7C6FF0");
    if (tone == QLatin1String("cyan"))
        return QStringLiteral("#0E9AA7");
    return QStringLiteral("#3D7CF2");
}

inline QString toneStripeColor(const QString &tone)
{
    if (tone == QLatin1String("green"))
        return QStringLiteral("#34C77B");
    if (tone == QLatin1String("orange"))
        return QStringLiteral("#F2A33C");
    if (tone == QLatin1String("red"))
        return QStringLiteral("#EF7A6B");
    if (tone == QLatin1String("purple"))
        return QStringLiteral("#9A8FF5");
    if (tone == QLatin1String("cyan"))
        return QStringLiteral("#45C3CF");
    return QStringLiteral("#4C8DFF");
}

// 指标卡片:白底圆角 + 语义色数值 + 底部细色条,让看板更有层次。
inline QWidget *metricCard(const QString &title, QLabel **valueLabel,
                           const QString &tone = QStringLiteral("blue"))
{
    auto *card = new QFrame;
    card->setObjectName(QStringLiteral("metricCard"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 12, 16, 0);
    layout->setSpacing(3);

    auto *caption = new QLabel(title, card);
    caption->setObjectName(QStringLiteral("metricLabel"));

    auto *value = new QLabel(QStringLiteral("0"), card);
    value->setObjectName(QStringLiteral("metricValue"));
    value->setStyleSheet(QStringLiteral("color:%1;").arg(toneTextColor(tone)));

    auto *stripe = new QFrame(card);
    stripe->setFixedHeight(3);
    stripe->setStyleSheet(
        QStringLiteral("background:%1;border-radius:1px;").arg(toneStripeColor(tone)));

    layout->addWidget(caption);
    layout->addWidget(value);
    layout->addStretch(1);
    layout->addWidget(stripe);

    if (valueLabel) {
        *valueLabel = value;
    }
    return card;
}

// 页面标题区:主标题 + 浅灰副标题。
inline QWidget *pageHeading(const QString &title, const QString &subtitle = QString())
{
    auto *heading = new QWidget;
    heading->setObjectName(QStringLiteral("pageHeading"));
    auto *layout = new QVBoxLayout(heading);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    auto *titleLabel = new QLabel(title, heading);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    layout->addWidget(titleLabel);

    if (!subtitle.isEmpty()) {
        auto *subLabel = new QLabel(subtitle, heading);
        subLabel->setObjectName(QStringLiteral("pageSub"));
        layout->addWidget(subLabel);
    }
    return heading;
}

struct Panel
{
    QFrame *card = nullptr;
    QHBoxLayout *header = nullptr; // 标题行,右侧可追加操作控件
    QVBoxLayout *body = nullptr;
    QLabel *title = nullptr;
};

// 统一的确认弹窗:窗口稍大 + 文字自动换行,确保提示完整可见。
inline bool confirm(QWidget *parent, const QString &title, const QString &text)
{
    QMessageBox box(QMessageBox::Question, title, text,
                    QMessageBox::Yes | QMessageBox::No, parent);
    box.setDefaultButton(QMessageBox::Yes);
    QAbstractButton *yes = box.button(QMessageBox::Yes);
    if (yes) {
        yes->setText(QStringLiteral("确定"));
    }
    QAbstractButton *no = box.button(QMessageBox::No);
    if (no) {
        no->setText(QStringLiteral("取消"));
    }
    box.setMinimumWidth(440);
    box.setMinimumHeight(340);
    if (QLabel *label = box.findChild<QLabel *>(QStringLiteral("qt_msgbox_label"))) {
        label->setMinimumWidth(380);
        label->setMinimumHeight(120);
        label->setWordWrap(true);
    }
    return box.exec() == QMessageBox::Yes;
}

// 信息/警告/错误弹窗:同样加高、自动换行,保证整段文字与按钮完整可见。
inline void showMessage(QMessageBox::Icon icon, QWidget *parent,
                        const QString &title, const QString &text)
{
    QMessageBox box(icon, title, text, QMessageBox::Ok, parent);
    QAbstractButton *ok = box.button(QMessageBox::Ok);
    if (ok) {
        ok->setText(QStringLiteral("知道了"));
    }
    box.setMinimumWidth(440);
    box.setMinimumHeight(340);
    if (QLabel *label = box.findChild<QLabel *>(QStringLiteral("qt_msgbox_label"))) {
        label->setMinimumWidth(380);
        label->setMinimumHeight(120);
        label->setWordWrap(true);
    }
    box.exec();
}

inline void info(QWidget *parent, const QString &title, const QString &text)
{
    showMessage(QMessageBox::Information, parent, title, text);
}

inline void warning(QWidget *parent, const QString &title, const QString &text)
{
    showMessage(QMessageBox::Warning, parent, title, text);
}

inline void critical(QWidget *parent, const QString &title, const QString &text)
{
    showMessage(QMessageBox::Critical, parent, title, text);
}

// 弹窗在 exec 前调用:按目标屏幕可用区收缩尺寸并居中,
// 避免窗口超过屏幕导致底部按钮(确定/取消)显示不全。
inline void fitToScreen(QDialog *dialog, QWidget *reference)
{
    if (!dialog) {
        return;
    }
    QScreen *screen = nullptr;
    if (reference && reference->screen()) {
        screen = reference->screen();
    }
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    QRect area = screen ? screen->availableGeometry()
                        : QRect(0, 0, 1280, 800);
    area.adjust(20, 20, -20, -20);

    if (dialog->layout()) {
        dialog->layout()->activate();
    }
    QSize desired = dialog->size().expandedTo(dialog->minimumSizeHint());
    if (desired.width() > area.width()) {
        desired.setWidth(area.width());
    }
    if (desired.height() > area.height()) {
        desired.setHeight(area.height());
    }
    dialog->resize(desired);
    dialog->move(area.center() - QPoint(desired.width() / 2, desired.height() / 2));
}

// 白色圆角内容卡:标题行(色条 + 标题 + 副标题) + 内容区。
inline Panel titledPanel(const QString &title, QWidget *parent = nullptr,
                         const QString &subtitle = QString())
{
    Panel panel;
    panel.card = new QFrame(parent);
    panel.card->setObjectName(QStringLiteral("card"));
    auto *layout = new QVBoxLayout(panel.card);
    layout->setContentsMargins(18, 14, 18, 16);
    layout->setSpacing(12);

    panel.header = new QHBoxLayout;
    panel.header->setSpacing(8);

    auto *tick = new QFrame(panel.card);
    tick->setObjectName(QStringLiteral("panelTick"));
    tick->setFixedSize(4, 16);
    panel.header->addWidget(tick);

    panel.title = new QLabel(title, panel.card);
    panel.title->setObjectName(QStringLiteral("panelTitle"));
    panel.header->addWidget(panel.title);

    if (!subtitle.isEmpty()) {
        auto *sub = new QLabel(subtitle, panel.card);
        sub->setObjectName(QStringLiteral("panelSub"));
        panel.header->addWidget(sub);
    }
    panel.header->addStretch(1);
    layout->addLayout(panel.header);

    panel.body = new QVBoxLayout;
    panel.body->setSpacing(10);
    layout->addLayout(panel.body, 1);
    return panel;
}

} // namespace ncs
