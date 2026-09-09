# 消息协议定义(V1.0)

本文档定义 `device_link_sim` 桩端与平台端的通信协议。协议与具体业务("充电桩")
无关,任何"多台设备连接一个平台"的场景都可以原样复用(core 层只认识
"设备 ID + 消息 + 回调")。

## 1. 传输与帧格式

- 传输层:WebSocket(TCP),文本帧,UTF-8 编码的 JSON。
- 连接方向:设备主动连接平台;平台监听固定端口,被动接受多个连接。
- 帧为数组格式(类 OCPP,不是严格 JSON-RPC 2.0):

```text
请求:    [2, "消息唯一ID", "Action名称", {payload}]
成功响应: [3, "消息唯一ID", {payload}]
失败响应: [4, "消息唯一ID", "错误码", "错误描述", {}]
```

消息 ID 由发送方生成并保证唯一;接收方回响应时原样带回该 ID,发送方据此把
响应与未完成的请求关联起来(乱序、并发都不受影响)。

## 2. 示例

设备上线注册:

```json
[2, "a1b2c3d4-1", "BootNotification", {"chargePointVendor": "NCS-Sim",
 "chargePointModel": "AC-7kW", "chargePointSerialNumber": "PILE-001"}]
```

```json
[3, "a1b2c3d4-1", {"status": "Accepted", "currentTime": "2026-09-08T10:00:00Z",
 "interval": 10}]
```

心跳(每 10 秒一次):

```json
[2, "e5f6-2", "Heartbeat", {}]
[3, "e5f6-2", {"currentTime": "2026-09-08T10:00:10Z"}]
```

平台拒绝某台桩的开始充电请求(例如该桩处于故障态):

```json
[4, "b2c3-9", "Faulted", "device is faulted", {}]
```

## 3. Action 清单

| 方向 | Action | 触发时机 | 关键 payload | 响应 |
| --- | --- | --- | --- | --- |
| 桩→平台 | `BootNotification` | 连接建立后立即发送一次 | `chargePointVendor` / `chargePointModel` / `chargePointSerialNumber` | `status`=Accepted / Rejected |
| 桩→平台 | `Heartbeat` | 每 10 秒 | 无 | `currentTime` |
| 桩→平台 | `StatusNotification` | 状态变化时 | `connectorId` / `status`(Idle/Charging/Faulted) / `timestamp` | 空对象 |
| 桩→平台 | `MeterValues` | 充电中每 2 秒 | `powerKw` / `energyKwh` / `timestamp` | 空对象 |
| 平台→桩 | `RemoteStartTransaction` | 用户点击"开始充电" | `connectorId` | `status`=Accepted 或错误码 |
| 平台→桩 | `RemoteStopTransaction` | 用户点击"停止充电" | 无 | `status`=Accepted 或错误码 |
| 平台→桩 | `RemoteReset` | 管理员触发(界面按钮或 HTTP 触发口) | `type`=Hard | `status`=Accepted |

## 4. 桩状态机

```text
Booting → Idle   收到 BootNotification 的 Accepted 响应
Idle → Charging  收到 RemoteStartTransaction
Charging → Idle  收到 RemoteStopTransaction,或本地模拟"充满"
任意 → Faulted   本地故障注入(界面"模拟故障")
Faulted → Idle   本地故障恢复(界面"恢复正常")
任意 → Booting   收到 RemoteReset(重启后重新 Boot)
```

状态切换后桩会发送 `StatusNotification` 通知平台;进入 Charging 后每 2 秒
发送一次 `MeterValues`。

## 5. 心跳与离线判定

- 桩每 10 秒发送一次 `Heartbeat`;充电中 `MeterValues` 的到达同样视为活跃。
- 平台记录每台桩"最后活跃时间",每 5 秒扫描一次。
- 超过 30 秒(3 个心跳周期)没有收到任何消息 → 判定离线:
  - 从在线管理列表(`DeviceRegistry`)移除;
  - 平台界面将该桩标记为"Offline"(灰显),直到它重新 Boot 上线。

## 6. 断线重连

- 桩端断线后自动重连,退避策略:1s → 2s → 4s → … 封顶 30s(指数退避,
  避免快速重试打爆平台)。
- 连接成功后退避计数清零。
- 平台侧主动断开或判定离线断开连接时,桩端同样会按退避策略重连并重新注册。

## 7. 异常处理

| 场景 | 处理 |
| --- | --- |
| 收到响应但找不到对应未完成请求 | 记录警告日志,不崩溃(`orphanResponse`) |
| 请求超时 | 触发回调 `ok=false`,不崩溃 |
| 收到无法解析的消息 | 丢弃并记录警告 |
| 平台收到未注册 Action | 回 `CallError`:`NotSupported` |
| 重复桩编号连接 | 平台顶掉旧连接,以新连接为准 |

## 8. 与主工程的可选联动

平台额外提供一个极简 HTTP 触发口(默认 `http://127.0.0.1:9080`):

```text
GET /reset?pileCode=PILE-001   # 触发一次 RemoteReset
GET /health                    # 健康检查
```

主工程管理端只需按 `config.ini` 中 `hook_enabled` 开关决定是否调用该接口,
不依赖本工程任何代码。
