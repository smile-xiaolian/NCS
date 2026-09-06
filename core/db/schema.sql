-- ============================================================
-- NCS 电动汽车充电桩应用管理平台 - 数据库结构脚本
-- 需求：UC-D-01 核心数据表设计 / UC-D-03 外键约束
-- 当前结构版本：1（与 core/service/PlatformService.cpp 中
--                kSchemaVersion 保持一致）
-- 说明：本脚本由程序内嵌（Qt 资源）执行，全部使用
--       CREATE TABLE IF NOT EXISTS，可重复执行。
-- ============================================================

-- 结构版本表（UC-D-02 版本管理）
CREATE TABLE IF NOT EXISTS schema_version(version INTEGER);

-- 用户表（车主）：手机号唯一标识，支持冻结，新增 debt 记录欠费金额
CREATE TABLE IF NOT EXISTS user(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    phone TEXT UNIQUE,
    nickname TEXT,
    avatar TEXT,
    balance REAL DEFAULT 0,
    debt REAL DEFAULT 0,
    status INTEGER DEFAULT 1,
    created_at TEXT
);

-- 管理员表：账号 + SHA-256 加盐密码哈希
CREATE TABLE IF NOT EXISTS admin(
    id INTEGER PRIMARY KEY,
    account TEXT UNIQUE,
    password_hash TEXT,
    salt TEXT
);

-- 充电站表
CREATE TABLE IF NOT EXISTS station(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT,
    address TEXT,
    longitude REAL,
    latitude REAL,
    price REAL
);

-- 电桩表：属于某个电站，BR-10 电站有电桩时禁止删除
CREATE TABLE IF NOT EXISTS charger(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    station_id INTEGER NOT NULL,
    code TEXT UNIQUE,
    type TEXT,
    power REAL,
    status INTEGER DEFAULT 0,
    total_count INTEGER DEFAULT 0,
    total_minutes REAL DEFAULT 0,
    CONSTRAINT fk_charger_station FOREIGN KEY(station_id)
        REFERENCES station(id) ON DELETE RESTRICT
);

-- 充电订单表：预约(0)/充电中(1)/已完成(2)/已取消(3)
CREATE TABLE IF NOT EXISTS charging_order(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    charger_id INTEGER NOT NULL,
    status INTEGER,
    created_at TEXT,
    start_time TEXT,
    end_time TEXT,
    energy REAL DEFAULT 0,
    amount REAL DEFAULT 0,
    CONSTRAINT fk_order_user FOREIGN KEY(user_id)
        REFERENCES user(id) ON DELETE RESTRICT,
    CONSTRAINT fk_order_charger FOREIGN KEY(charger_id)
        REFERENCES charger(id) ON DELETE CASCADE
);

-- 充值流水表（UC-U-05 可选）
CREATE TABLE IF NOT EXISTS recharge_log(
    id INTEGER PRIMARY KEY,
    user_id INTEGER,
    amount REAL,
    created_at TEXT,
    CONSTRAINT fk_recharge_user FOREIGN KEY(user_id)
        REFERENCES user(id) ON DELETE RESTRICT
);

-- 运维日志表
CREATE TABLE IF NOT EXISTS ops_log(
    id INTEGER PRIMARY KEY,
    charger_id INTEGER,
    action TEXT,
    created_at TEXT,
    CONSTRAINT fk_ops_charger FOREIGN KEY(charger_id)
        REFERENCES charger(id) ON DELETE CASCADE
);

-- 负荷预测结果表（UC-M-03 回写）
CREATE TABLE IF NOT EXISTS load_prediction(
    id INTEGER PRIMARY KEY,
    station_id INTEGER,
    target_time TEXT,
    predicted_energy REAL,
    predicted_idle INTEGER,
    is_peak INTEGER,
    CONSTRAINT fk_pred_station FOREIGN KEY(station_id)
        REFERENCES station(id) ON DELETE CASCADE
);
