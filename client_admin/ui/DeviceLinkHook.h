#pragma once

#include <QString>

namespace ncs {

// 可选联动(my_device_link_sim 拔高模块):
// 管理端「远程重启」成功后,异步通知模拟器平台向对应桩下发 RemoteReset。
//
// 设计约束(见模拟器需求文档第 6 节):主工程完全不依赖该挂钩也能正常编译运行。
// 模拟器不在线 / 请求超时 / 任何网络错误均静默忽略(仅 qDebug 日志),
// 不弹窗、不阻塞 UI、不影响原有数据库逻辑。
//
// 配置(可选): 在 ncs_admin 同级目录放 config.ini:
//   [device_link]
//   hook_enabled=true                          ; 缺省 true
//   reset_url=http://127.0.0.1:9080/reset      ; 缺省该值
void notifySimulatorReset(const QString &pileCode);

} // namespace ncs
