# 依赖项

本文档列出了Multi-Controller项目所需的所有依赖项及其版本要求。

## 系统依赖

### 操作系统
- Windows 10/11 (64位)

### 编译器
- Visual Studio 2022 或兼容的C++17编译器
- 支持C++17标准的编译器

## 构建工具

### CMake
- 版本要求：3.16 或更高
- 下载地址：https://cmake.org/download/

### Qt
- 版本要求：Qt 6.10.1 或更高版本
- 必需组件：Widgets
- 下载地址：https://www.qt.io/download

## 第三方库

### Thorlabs KDC101 运动控制库
- 库文件：
  - Thorlabs.MotionControl.KCube.DCServo.lib
  - Thorlabs.MotionControl.DeviceManager.dll
  - Thorlabs.MotionControl.KCube.DCServo.dll
- 头文件：
  - Thorlabs.MotionControl.KCube.DCServo.h
- 包含在项目的 `3rdparty/KDC101/` 目录中

### DVP2 相机控制库
- 库文件：
  - DVPCamera64.lib
  - DVPCamera64.dll
- 头文件：
  - DVPCamera.h
- 包含在项目的 `3rdparty/DVP2/` 目录中

## 必需组件
- Qt6Widgets 模块（GUI 构建）
- Qt6Core 模块（CLI 与 core 库）

## 可选依赖

### 开发工具
- Qt Creator (推荐IDE)
- Git (版本控制)

### 测试
- Qt6Test 模块（单元测试，CMake 选项 MC_ENABLE_TESTS=ON 时需要）

### 打包
- NSIS (Nullsoft Scriptable Install System) - 生成 Windows 安装包
  - 下载地址：https://nsis.sourceforge.io/
- windeployqt (随 Qt 安装) - 收集 Qt 运行时依赖

### 双工具链支持
- MSYS2/MinGW (可选构建路径，CMake 选项 MC_ENABLE_HARDWARE=OFF)
  - 厂商 SDK 仅提供 MSVC ABI，MinGW 仅编译 core/CLI 纯逻辑与测试

## 依赖项安装指南

### 1. 安装Visual Studio
1. 从Visual Studio官网下载Visual Studio 2022
2. 在安装过程中选择"使用C++的桌面开发"工作负载
3. 确保安装了Windows 10/11 SDK

### 2. 安装Qt
1. 从Qt官网下载Qt 6.10.1或更高版本的Open Source或Commercial版本
2. 在安装过程中选择Qt Creator和MSVC 2022 64-bit组件
3. 记录安装路径，后续配置CMake时需要

### 3. 安装CMake
1. 从CMake官网下载最新版本的CMake
2. 运行安装程序，选择"Add CMake to the system PATH"选项
3. 验证安装：在命令提示符中输入 `cmake --version`

### 4. 配置环境变量
确保以下路径已添加到系统PATH环境变量：
- Qt安装目录的bin路径（例如：`C:\Qt\6.10.1\msvc2022_64\bin`）
- CMake安装目录的bin路径（例如：`C:\Program Files\CMake\bin`）

## 构建依赖检查

项目包含了一个CMake配置文件，会自动检查所需的依赖项。如果构建失败，请检查：

1. 所有依赖项是否已正确安装
2. 环境变量是否正确设置
3. Qt和CMake版本是否兼容

## 许可证注意事项

- Qt框架遵循LGPLv3许可证
- Thorlabs和DVP2库的使用可能需要遵守各自的许可协议
- 请确保在使用这些库时遵守相应的许可证要求