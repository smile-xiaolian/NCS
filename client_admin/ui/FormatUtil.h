#pragma once

#include <QString>

namespace ncs {

inline QString chargerStatusText(int status)
{
    switch (status) {
    case 0: return QStringLiteral("空闲");
    case 1: return QStringLiteral("使用中");
    case 2: return QStringLiteral("故障");
    default: return QStringLiteral("未知");
    }
}

inline QString orderStatusText(int status)
{
    switch (status) {
    case 0: return QStringLiteral("预约中");
    case 1: return QStringLiteral("充电中");
    case 2: return QStringLiteral("已完成");
    case 3: return QStringLiteral("已取消");
    default: return QStringLiteral("未知");
    }
}

inline QString maskedPhone(const QString &phone)
{
    if (phone.size() == 11) {
        return phone.left(3) + QStringLiteral("****") + phone.right(4);
    }
    return phone;
}

inline QString number(double value, int decimals = 2)
{
    return QString::number(value, 'f', decimals);
}

} // namespace ncs