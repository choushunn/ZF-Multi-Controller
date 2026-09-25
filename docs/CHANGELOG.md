# 更新日志

本文档记录了Multi-Controller项目的所有重要更改。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)，
并且本项目遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [未发布]

### 新增
- ICamera 统一接口契约，DVP2 与 TOUPCam 可按契约接入
- DvpCamera 实现：封装 DVP2 SDK，后台 std::thread 采帧，frameReady 信号回传 UI
- SimCamera 仿真相机：无硬件依赖，验证采帧通路与 UI 显示
- mc::Logger 分级日志系统：Debug/Info/Warn/Error 分级 + 控制台输出 + 按日期/大小轮转文件持久化
- mc::AppConfig 配置管理：JSON 加载/持久化，含默认设备/速度/限位/日志/相机参数，消除代码内硬编码
- MotorConfig 改为运行时可配置，由 AppConfig 注入
- 软件图标 app.ico（多分辨率）+ Windows .rc 版本信息 + Qt qrc 资源
- 单元测试：MotorStatus 状态位解析、MotorConfig 单位换算与限位校验、AppConfig 加载/持久化
- CMakePresets.json：MSVC 与 MinGW 双工具链预设
- GitHub Actions CI：双工具链构建 + ctest
- NSIS 安装包打包：CPack + windeployqt + 卸载/快捷方式/依赖收集
- MC_ENABLE_HARDWARE / MC_ENABLE_SIM_CAMERA / MC_ENABLE_TESTS 构建选项

### 更改
- DVP2 从死代码（无 target 链接）升级为相机承载落点
- KCubeMotor 支持运行时 MotorConfig 注入
- MainWindow 集成相机控制（连接/采集/保存）与分级日志显示
- CLI 入口集成 AppConfig 配置加载与 Logger 日志
- CMakeLists.txt 重构：选项化构建、POST_BUILD DLL 拷贝、CPack 集成

### 计划中
- 引入 spdlog（经 vcpkg）替代自研 Logger 后端
- TOUPCam 相机接入（按 ICamera 契约新增实现）
- 电机运动轨迹记录
- 多设备同时控制支持

## [0.1.0] - 2024-01-XX

### 新增
- 初始版本发布
- 基于Qt6的图形用户界面
- Thorlabs KDC101电机控制器集成
  - 设备连接和断开
  - 位置控制（微米级精度）
  - 速度和加速度参数设置
  - 点动控制（正向/反向）
  - 归位操作
  - 实时状态监控
- DVP2相机集成
  - 相机连接功能
  - 图像采集预览
  - 图像保存功能
- 操作日志记录系统
- 实时状态显示

### 技术实现
- 使用CMake构建系统
- C++17标准实现
- 模块化设计，支持第三方库集成
- 跨平台兼容性（主要针对Windows）

### 文档
- 基本使用说明
- 安装和构建指南
- 项目结构说明

## [0.0.1] - 开发阶段

### 新增
- 项目初始化
- 基本项目结构搭建
- 第三方库集成框架
- Git版本控制初始化

---

## 版本说明

### 版本号格式
本项目使用语义化版本号：`主版本号.次版本号.修订号`

- **主版本号**：不兼容的API修改
- **次版本号**：向下兼容的功能性新增
- **修订号**：向下兼容的问题修正

### 更新类型
- `新增` - 新功能
- `更改` - 对现有功能的更改
- `弃用` - 即将移除的功能
- `移除` - 已移除的功能
- `修复` - 问题修复
- `安全` - 安全相关的修复