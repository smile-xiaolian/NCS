# 充电桩通信模拟器(my_device_link_sim)

独立于 NCS 主工程的拔高模块:通用设备通信框架 + 桩端/平台端模拟器。
覆盖需求文档的完整功能:桩状态机、心跳/离线判定、指数退避重连、故障注入、
多桩并发管理、HTTP 远程重启触发口;两端均为 GUI。

## 构建

```bash
cmake -S . -B build
cmake --build build
```

依赖:Qt6 Core / Gui / Widgets / Network / WebSockets / Sql / Test

> 注意:Qt 6.9+ 的官方安装包默认**不包含** Qt WebSockets 模块。若 CMake 配置报
> `Failed to find required Qt component "WebSockets"`,需用 Qt Maintenance Tool
> 为对应版本额外安装 Add-ons 下的 "Qt WebSockets"(组件名形如
> `qt.qt6.6112.addons.qtwebsockets`),再重新配置即可。

## 运行

```bash
# 终端1:平台
./build/platform_app/platform_app --port 9000 --http-port 9080

# 无真实桩时的独立演示:进程内自动内置 3 台虚拟桩(不另开 pile_app)
./build/platform_app/platform_app --port 9000 --demo
# 指定内置虚拟桩数量(1-8)
./build/platform_app/platform_app --port 9000 --demo --demo-piles 5

# 终端2~N:多个桩
./build/pile_app/pile_app --code PILE-001 --url ws://127.0.0.1:9000
./build/pile_app/pile_app --code PILE-002 --url ws://127.0.0.1:9000
./build/pile_app/pile_app --code PILE-003 --url ws://127.0.0.1:9000

# 桩端独立自测:内置模拟平台,界面提供"模拟平台下发指令"按钮
./build/pile_app/pile_app --code PILE-001 --demo
# 指定内置模拟平台端口
./build/pile_app/pile_app --code PILE-001 --demo --mock-port 9200
```

平台界面里每张桩卡片都带"打开桩窗口"按钮,可直接打开该桩对应的桩端界面:

- `--demo` 演示桩:在平台进程内直接弹出桩端窗口,与平台共享同一连接,双向实时联动
  (点击"手动断线/模拟故障"等会即时反映到平台卡片与日志)。
- 真实桩:桩离线时卡片会显示该按钮,点击后自动拉起同级目录部署的 `pile_app`,
  并以该桩的编号/型号自动连接平台。

> 增量更新注意:本轮新增了 `pile_logic`/`pile_gui` 静态库目标与演示模式文件,
> 请删除旧的 `build/` 目录后重新执行上面的 configure + build;
> 用 Qt Creator 打开时按提示重新配置即可(且需先为当前 Qt 版本安装 WebSockets 组件)。

## 测试

```bash
ctest --test-dir build --output-on-failure
```

## 与主工程可选挂钩

平台提供:`http://127.0.0.1:9080/reset?pileCode=XXX`

主工程 `config.ini` 开启:

```ini
[device_link]
hook_enabled=true
reset_url=http://127.0.0.1:9080/reset
```

## 目录

- `core/` — 与业务无关的通信框架(MessageFrame / DeviceConnection / PendingRequestTable / ActionDispatcher / DeviceRegistry)
- `pile_app/` — 桩模拟器(状态徽章/输出功率条/功率滚动 + 故障注入界面)
- `platform_app/` — 平台监控台(每桩卡片:状态灯、功率与本次充电进度条、功率序列滚动、
  在线/离线灰显、可调演示目标与自动停止、卡片可打开对应桩端窗口——内置演示桩在进程内
  打开、真实离线桩自动拉起外部 pile_app;右侧运行日志;另有 HTTP 触发口)
- `docs/protocol.md` — 消息协议定义与 Action 清单
- `tests/` — 消息编解码 / 超时 / 孤儿响应 / echo / 并发乱序关联 / 分发器测试

## 功能对照(需求文档第 5 节验收标准)

- 平台监听端口,3 台以上桩同时在线可见
- 充电中桩每 2 秒上报 MeterValues,平台界面实时刷新功率与电量
- 平台下发 RemoteStartTransaction 后桩切换 Charging 并持续上报
- 桩断线(杀进程/断网)后平台在 30 秒内判定离线并在界面灰显
- 桩恢复后按指数退避自动重连并重新上线
- 并发请求按消息 ID 关联响应,不串号(含核心层单元测试覆盖)

## 连接 NCS 主工程充电桩(项目数据库)

平台端启动时会按主工程规则自动查找 `charge_platform.db`
(Linux `~/.local/share/NCS/charge_platform.db`;Windows `%LOCALAPPDATA%/NCS/charge_platform.db`),
也可用 `--db <路径>` 显式指定,用 `--no-db` 完全关闭:

- 数据库可用时,`--demo` 默认把项目数据库里登记的全部充电桩(如 S1-01...)作为内置虚拟桩接入平台,无需逐个启动 pile_app;
- 任意桩以项目在册编号注册(含外部 `pile_app --code S1-01 --url ws://127.0.0.1:9000`)时,平台自动识别所属电站/类型/额定功率;
- 模拟状态(空闲/充电/故障/下线)实时回写主工程 `charger.status`(0 空闲/1 充电/2 故障)并追加 `ops_log`,管理端刷新即可看到;
- 数据库缺失、未初始化或使用 `--no-db` 时,平台退回原有独立 DEMO 桩演示,不依赖主工程。
- 平台监控台顶部提供搜索框,可按 桩编号 / 型号 / 所属电站 / 状态 实时过滤卡片。

示例:

```bash
# 项目桩演示:自动接入数据库全部在册电桩(可用 --demo-piles N 限制数量)
./build/platform_app/platform_app --port 9000 --demo

# 只启动前 12 台项目桩
./build/platform_app/platform_app --port 9000 --demo --demo-piles 12

# 外部桩连平台,编号命中项目电桩时同样会被识别并回写
./build/pile_app/pile_app --code S1-01 --url ws://127.0.0.1:9000

# 完全独立模式(不读项目数据库)
./build/platform_app/platform_app --port 9000 --demo --no-db
```

> 前提:主工程需先运行一次(`ncs_user`/`ncs_admin`)生成并初始化 `charge_platform.db`。