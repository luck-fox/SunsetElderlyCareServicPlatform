# 智慧夕阳服务管理系统

基于Qt框架开发的老年人健康监测平台，提供实时健康数据监测、老人信息管理、数据趋势分析等功能。

## ✨ 核心功能

- **实时健康监测**：心率、血压、体温、血氧等 vital signs 实时显示
- **老人信息管理**：支持添加、删除、查看老人基本信息和紧急联系人
- **健康数据趋势图**：可视化展示健康数据变化趋势，支持双击全屏查看
- **串口设备通信**：支持手环等健康监测设备接入，自动重连
- **HTTP数据推送**：健康数据自动推送至云端/家属端
- **数据导出**：支持导出健康数据为CSV格式
- **数据统计分析**：提供心率、血压等指标的统计分析
- **模拟数据生成**：内置模拟数据功能，方便测试演示

## 🛠 技术栈

- **框架**：Qt 6 (Core, GUI, Widgets, Charts, SerialPort, Network, SQL)
- **数据库**：SQLite
- **通信**：串口通信、HTTP/HTTPS
- **编译器**：MSVC 2022 / MinGW

## 📦 项目结构

```
SunsetElderlyCareServicPlatform/
├── main.cpp                  # 程序入口
├── mainwindow.h/cpp          # 主窗口UI和业务逻辑
├── healthdatamanager.h/cpp   # 健康数据管理（增删改查）
├── serialdevice.h/cpp        # 串口设备通信
├── httppushservice.h/cpp     # HTTP推送服务
├── threadworker.h/cpp        # 多线程工作器
├── resources.qrc             # Qt资源文件
└── SunsetElderlyCareServicPlatform.pro  # Qt项目配置
```

## 🚀 快速开始

### 环境要求

- Qt 6.5+
- C++17 编译器（MSVC 2022 或 MinGW）

### 编译运行

1. 使用 Qt Creator 打开 `SunsetElderlyCareServicPlatform.pro`
2. 配置 Kit（选择 Desktop Qt 6.x MSVC2022 64bit 或 MinGW）
3. 点击运行按钮或按 `Ctrl+R`

### 首次使用

- 程序启动后会自动生成模拟数据
- 点击"添加老人"按钮录入老人信息
- 双击趋势图可全屏查看详细数据

## 📊 功能截图

（待添加）

## 📝 数据库说明

程序使用 SQLite 数据库存储健康数据，数据库文件自动生成在程序运行目录：

- `智慧夕阳健康数据.db` - 主数据库文件

### 数据表结构

**elder_info** - 老人信息表
- elder_id (主键)
- name, age, gender
- phone, emergency_contact, emergency_phone
- address, notes

**health_data** - 健康数据表
- id (主键)
- elder_id, record_time
- heart_rate, systolic_pressure, diastolic_pressure
- temperature, spo2, step_count
- emotion_level, pressure_level
- battery, sos_status

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 📄 许可证

MIT License

## 👨‍💻 作者

luck-fox

---

**智慧养老，科技守护** 🌅
