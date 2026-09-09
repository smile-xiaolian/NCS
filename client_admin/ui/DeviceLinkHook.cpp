#include "DeviceLinkHook.h"

#include <QCoreApplication>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace {

constexpr int kTimeoutMs = 2000;

// 每次调用都读一次配置文件,允许不重启管理端直接改配置
bool hookConfig(QString *resetUrl)
{
    const QSettings settings(QCoreApplication::applicationDirPath() + QStringLiteral("/config.ini"),
                             QSettings::IniFormat);
    const bool enabled = settings.value(QStringLiteral("device_link/hook_enabled"), true).toBool();
    if (!enabled)
        return false;
    *resetUrl = settings
                    .value(QStringLiteral("device_link/reset_url"),
                           QStringLiteral("http://127.0.0.1:9080/reset"))
                    .toString();
    return !resetUrl->isEmpty();
}

QNetworkAccessManager *networkManager()
{
    // 挂到应用对象上,进程生命周期内复用,退出时自动释放
    static auto *manager = new QNetworkAccessManager(QCoreApplication::instance());
    return manager;
}

} // namespace

void ncs::notifySimulatorReset(const QString &pileCode)
{
    if (pileCode.isEmpty())
        return;

    QString base;
    if (!hookConfig(&base))
        return;

    QUrl url(base);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("pileCode"), pileCode);
    url.setQuery(query);

    QNetworkReply *reply = networkManager()->get(QNetworkRequest(url));

    // 模拟器不在线时连接会立刻失败;兜底一个超时,防止极端情况下请求悬挂
    QTimer::singleShot(kTimeoutMs, reply, [reply]() {
        if (!reply->isFinished())
            reply->abort();
    });

    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, pileCode]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "[device_link] 已通知模拟器远程重启桩" << pileCode;
        } else {
            // 模拟器未运行属正常场景(挂钩为可选项),仅记录调试日志
            qDebug() << "[device_link] 模拟器未响应(可忽略):" << reply->errorString();
        }
        reply->deleteLater();
    });
}
