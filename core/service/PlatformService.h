#pragma once

#include "core/models/User.h"
#include <QVariantList>
#include <QString>

class PlatformService {
public:
    // ==================== 系统初始化与基础服务 ====================
    static bool initialize(QString *error = nullptr);
    static QString databasePath();

    // ==================== 用户端核心业务 ====================
    // 用户手机号登录或自动注册
    static User loginOrRegister(const QString &phone, QString *error = nullptr);
    // 获取充电站列表（支持按经纬度计算距离并排序）
    static QVariantList stations(double lat = 31.2304, double lon = 121.4737);
    // 获取单个充电站详情
    static QVariantMap station(int stationId);
    // 获取电桩列表
    static QVariantList chargers(int stationId = 0);
    // 获取用户的充电订单列表
    static QVariantList orders(int userId = 0);
    
    // ==================== 用户账户与订单操作 ====================
    // 用户充值
    static bool recharge(int userId, double amount, QString *error = nullptr);
    // 修改用户昵称
    static bool updateNickname(int userId, const QString &nickname, QString *error = nullptr);
    // 预约电桩（含未结算拦截与余额校验）
    static bool reserve(int userId, int chargerId, QString *error = nullptr);
    // 开始充电
    static bool start(int userId, QString *error = nullptr);
    // 结束充电并结算订单（扣费、更新电量与电桩状态）
    static bool settle(int userId, QString *error = nullptr);
    // 取消预约
    static bool cancelReservation(int userId, QString *error = nullptr);

    // ==================== 管理端核心业务 ====================
    // 管理员登录验证
    static bool adminLogin(const QString &account, const QString &password);
    // 获取用户列表（支持手机号模糊搜索）
    static QVariantList users(const QString &keyword = {});
    // 获取指定天数的营收统计
    static QVariantList revenueDays(int days = 30);
    // 获取后台首页核心指标数据
    static QVariantMap metrics();
    // 获取电桩总览与健康度统计
    static QVariantMap chargerOverview();
    // 获取负荷预测数据
    static QVariantList predictions();
    // 设置用户状态（正常/冻结）
    static bool setUserStatus(int userId, int status);
    // 设置电桩状态
    static bool setChargerStatus(int chargerId, int status);
    // 远程重启电桩
    static bool restartCharger(int chargerId, QString *error = nullptr);
    // 新增或修改充电站
    static bool saveStation(int id, const QString &name, const QString &addr, double lon, double lat, double price, QString *error = nullptr);
    // 删除充电站
    static bool deleteStation(int id, QString *error = nullptr);
    // 新增或修改电桩
    static bool saveCharger(int id, int stationId, const QString &code, const QString &type, double power, QString *error = nullptr);
    // 删除电桩
    static bool deleteCharger(int id, QString *error = nullptr);
};