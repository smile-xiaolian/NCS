#pragma once

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

namespace ncs {

// 生成一个指标卡片。valueLabel 用于接收数值 Label 的指针,由调用方负责刷新文本。
inline QWidget *metricCard(const QString &title, QLabel **valueLabel,
                           const QString &valueColor = QStringLiteral("#1f6feb"))
{
    auto *card = new QFrame;
    card->setFrameShape(QFrame::StyledPanel);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 10, 12, 10);

    auto *caption = new QLabel(title);
    caption->setStyleSheet(QStringLiteral("color:#8b949e;font-size:12px;"));
    caption->setAlignment(Qt::AlignCenter);

    auto *value = new QLabel(QStringLiteral("0"));
    value->setAlignment(Qt::AlignCenter);
    value->setStyleSheet(QStringLiteral("font-size:26px;font-weight:bold;color:%1;").arg(valueColor));

    layout->addWidget(caption);
    layout->addWidget(value);
    if (valueLabel) {
        *valueLabel = value;
    }
    return card;
}

} // namespace ncs