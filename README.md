# Multi-Controller

一个基于Qt的C++多控制器应用程序，用于集成和控制Thorlabs KDC101电机控制器和DVP2相机设备。

## 项目概述

Multi-Controller是一个专业的硬件控制软件，提供直观的图形界面来控制精密电机运动和相机图像采集。该应用程序主要用于显微镜、光学实验和精密定位系统。

### 主要功能

- **电机控制**
  - 连接和控制Thorlabs KDC101电机控制器
  - 精确位置控制（微米级精度）
  - 速度和加速度参数调节
  - 点动控制和归位操作
  - 实时状态监控

- **相机控制**
  - DVP2相机连接和图像采集
  - 实时图像预览
  - 图像保存功能

- **用户界面**
  - 直观的操作界面
  - 实时状态显示
  - 操作日志记录

## 系统要求

### 硬件要求
- Thorlabs KDC101电机控制器
- DVP2兼容相机
- USB连接线

### 软件要求
- Windows 10/11 (64位)
- Qt 6.10.1 或更高版本
- Visual Studio 2022 或兼容的C++17编译器
- CMake 3.16 或更高版本

## 安装说明

### 1. 克隆仓库
```bash
git clone https://github.com/choushunn/ZF-Multi-Controller.git
cd ZF-Multi-Controller
```

### 2. 安装依赖
- 确保已安装Qt 6.10.1或更高版本
- 安装Visual Studio 2022或兼容的C++17编译器
- 安装CMake 3.16或更高版本

### 3. 构建项目
```bash
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH="<Qt安装路径>"
cmake --build .
```

构建选项（CMake 选项）：
- `MC_ENABLE_HARDWARE=ON/OFF` - 启用/禁用硬件适配层（默认 ON，MinGW 构建时设为 OFF）
- `MC_ENABLE_TESTS=ON/OFF` - 构建/跳过单元测试（默认 ON）
- `MC_ENABLE_SIM_CAMERA=ON/OFF` - 构建/跳过仿真相机（默认 ON）

运行测试：
```bash
cd build
ctest
```

### 4. 运行应用程序
```bash
./Multi-Controller.exe
```

## 使用说明

### 电机控制

1. **连接设备**
   - 点击"刷新"按钮扫描可用设备
   - 从下拉列表中选择设备序列号
   - 点击"连接"按钮建立连接

2. **归位操作**
   - 首次使用或更换设备后，点击"归位"按钮
   - 等待归位操作完成

3. **位置控制**
   - 在"目标位置"输入框中设置目标位置（微米）
   - 点击"移动"按钮执行移动

4. **速度设置**
   - 在"速度"和"加速度"输入框中设置参数
   - 点击"设置速度"按钮应用更改

5. **点动控制**
   - 设置点动步长
   - 使用"正向点动"或"反向点动"按钮进行微调

### 相机控制

1. **连接相机**
   - 点击"连接相机"按钮
   - 等待相机初始化完成

2. **图像采集**
   - 点击"捕获图像"按钮获取当前图像
   - 点击"保存图像"按钮保存图像到本地

## 配置管理

应用程序在首次运行时自动在可执行文件旁生成 `config.json`，包含：
- 默认设备序列号（上次连接的设备）
- 电机参数（位置换算系数、限位范围、默认速度/加速度）
- 日志设置（目录、级别、最大文件大小）
- 相机设置（UserID、图像保存目录）

修改 `config.json` 可在不重新编译的情况下调整参数。模板见 `config.json.example`。

## 技术细节

### 项目结构
```
Multi-Controller/
├── CMakeLists.txt           # CMake构建配置
├── CMakePresets.json        # MSVC/MinGW 双工具链预设
├── config.json.example      # 配置文件模板
├── src/
│   ├── app/                 # GUI应用层
│   │   ├── main.cpp         # 应用程序入口点
│   │   ├── mainwindow.h/.cpp# 主窗口类
│   │   └── mainwindow.ui    # 主窗口UI定义
│   ├── core/                # 纯逻辑层（无硬件依赖）
│   │   ├── motorstatus.h/.cpp# 电机状态解析
│   │   ├── motorconfig.h    # 电机参数配置（运行时可配置）
│   │   ├── logger.h/.cpp    # 分级日志系统
│   │   ├── appconfig.h/.cpp # JSON 配置管理
│   │   ├── icamera.h        # 相机统一接口契约
│   │   └── hardware/        # 硬件适配层（依赖厂商SDK，仅MSVC构建）
│   │       ├── kcubemotor.h/.cpp# KDC101电机控制封装
│   │       ├── dvp_camera.h/.cpp# DVP2 相机实现
│   │       ├── toup_camera.h/.cpp# ToupCam 相机实现
│   │       └── sim_camera.h     # 仿真相机（测试用）
│   ├── cli/                 # 命令行入口（复用core）
│   │   └── main.cpp
│   ├── tests/               # 单元测试
│   │   ├── CMakeLists.txt
│   │   ├── test_motorstatus.cpp
│   │   ├── test_motorconfig.cpp
│   │   └── test_appconfig.cpp
│   └── resources/           # 资源文件
│       ├── app.rc           # Windows资源（图标+版本）
│       ├── app.qrc          # Qt资源文件
│       └── icons/app.ico    # 应用图标
├── packaging/               # 打包脚本
│   └── installer.nsi        # NSIS安装包脚本
├── .github/workflows/       # CI/CD
│   ├── ci.yml               # 持续集成
│   └── release.yml          # 发布打包
├── 3rdparty/                # 第三方库（仅保留构建必需文件）
│   ├── KDC101/              # Thorlabs电机控制器库
│   └── DVP2/                # DVP2相机库
└── docs/                    # 文档
```

### 依赖库
- Qt6 Widgets模块
- Thorlabs KDC101运动控制库 [KDC101 SDK](https://www.thorlabs.com/software_pages/ViewSoftwarePage.cfm?Code=Motion_Control&viewtab=0)
- DVP2相机控制库[DVP2 SDK](https://cdhbuy.oss-cn-beijing.aliyuncs.com/Public/statics/files/SDK%E4%BA%8C%E6%AC%A1%E5%BC%80%E5%8F%91%E5%8C%85.rar)

### 编译器要求
- C++17标准支持
- MSVC 2022或兼容编译器

## 故障排除

### 常见问题

1. **设备连接失败**
   - 检查设备是否正确连接
   - 确认设备驱动程序已安装
   - 尝试重新插拔USB连接

2. **编译错误**
   - 确保所有依赖项已正确安装
   - 检查Qt和CMake版本兼容性
   - 确认Visual Studio版本支持C++17

3. **运行时错误**
   - 检查DLL文件是否在可执行文件目录中
   - 确认第三方库文件路径正确

## 贡献指南

欢迎提交问题报告和功能请求！如果您想贡献代码，请遵循以下步骤：

1. Fork本仓库
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 创建Pull Request

## 许可证

本项目采用MIT许可证 - 详见[LICENSE](LICENSE)文件

## 联系方式

- 项目主页: https://github.com/choushunn/ZF-Multi-Controller
- 问题报告: https://github.com/choushunn/ZF-Multi-Controller/issues

## 更新日志

详见[CHANGELOG.md](docs/CHANGELOG.md)

## 依赖项

详见[DEPENDENCIES.md](docs/DEPENDENCIES.md)