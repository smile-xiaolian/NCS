# NCS 电动汽车充电桩应用管理平台

Qt 6 + C++17 + SQLite 的跨平台演示项目，包含车主客户端 `ncs_user`、运营管理端 `ncs_admin`、Python 预测和 Vue/ECharts 数据看板。

## Windows 开发与 Ubuntu 22.04 交付

Ubuntu 安装依赖：`sudo apt install build-essential cmake qt6-base-dev qt6-charts-dev libqt6sql6-sqlite python3`。

构建：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/bin/ncs_user
./build/bin/ncs_admin
```

数据库保存在操作系统共享数据目录（Linux 为 `~/.local/share/NCS/charge_platform.db`，Windows 为本机 AppData 下的 `NCS` 目录），而非硬编码 Windows 路径。两个应用共享该数据库，并启用 WAL/外键。

管理员初始帐号：`admin` / `123456`。用户端验证码为界面显示的模拟验证码。

## 预测与大屏

完整用法见 [ml/使用说明.md](ml/使用说明.md)。

小时负荷训练集在 `ml/dataset/hourly_load.csv`（可先只保留表头）。有数据库后可导出：

```bash
python3 ml/predict.py --db ~/.local/share/NCS/charge_platform.db --export-dataset --train --evaluate --predict
```

无数据库时写入 CSV 后执行：

```bash
python3 ml/predict.py --train --evaluate --predict
```

预测结果写入 `web/public/data/prediction.json`，大屏右侧「未来 24 小时负荷预测」读取该文件。运营 JSON 仍放到 `web/public/data/dashboard.json`。

```bash
cd web && npm install && npm run dev
```
