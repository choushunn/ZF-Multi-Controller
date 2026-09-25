# Multi-Controller 项目改造方案

## 摘要

本文档既是 Multi-Controller 的改造建议，也是一份按软件系统工程要求组织的工程方案：以需求追踪为主线、以分层架构为核心、以阶段门禁为交付判据，覆盖风险、涉众、交付物与范围边界。目标是让项目从"单文件、硬编码、厂商 SDK 耦合"演进为可维护、可扩展、可交付的工程化上位机软件。截至现状，git 治理、3rdparty 瘦身（含 ToupTek SDK 按惯例入库）、core/hardware 分层、CLI 入口、配置与测试体系均已完成并经双工具链实证（MinGW 与 MSVC 构建通过、ctest 3/3 全绿、CLI 运行正常、厂商 DLL 自动部署）。剩余待办集中在 spdlog 日志基座、NSIS 安装包实测、TOUPCam 硬件实测与 CI 远端验证。建议按「基础治理 → 架构重构 → 工程化增强 → 相机功能」推进，每个方向均可独立落地、独立验证，避免一次性大重构的回归风险。

## 当前状态快照

技术栈为 Qt6 Widgets + CMake 3.16 + C++17，Windows/MSVC2022 环境构建，已接入 Thorlabs KDC101 电控；DVP2 相机仅为库与 DLL 拷贝，无采图实现。对照已完成工作，当前真实状态如下。

已完成并验证：主窗口源码已迁入 `src/app/`，由约 540 行的单文件拆分为三层。[src/core](file:///d:/CPP/Multi-Controller-Source/src/core) 承载纯逻辑（[motorstatus.cpp](file:///d:/CPP/Multi-Controller-Source/src/core/motorstatus.cpp)、[motorconfig.h](file:///d:/CPP/Multi-Controller-Source/src/core/motorconfig.h)），硬件适配层独立为 [src/core/hardware](file:///d:/CPP/Multi-Controller-Source/src/core/hardware)（[kcubemotor.cpp](file:///d:/CPP/Multi-Controller-Source/src/core/hardware/kcubemotor.cpp)、dvp_camera、toup_camera、sim_camera），状态掩码已收敛为具名枚举，物理单位换算与限位校验下沉到 core（devicePositionToUm 已从硬件层迁为 MotorConfig 静态方法，MinGW 亦可单测）。[src/cli](file:///d:/CPP/Multi-Controller-Source/src/cli) 新增 `Multi-ControllerCLI` 命令行入口，与 GUI 共用 `mc_core`。日志从 UI 坐标解耦至 core 的 logMessage 信号，UI 订阅刷新。3rdparty 已从约 112.8 MB 瘦身至构建必需文件，.gitignore、build 目录忽略、厂商二进制白名单已建立，新克隆开箱可构建。

经验证本段原文已过时：DVPCamera64 实际已被 mc_core 链接（dvp_camera.cpp 编译进硬件层），构建后 DVPCamera64.dll、Thorlabs 各 dll、toupcam.dll 均自动拷贝至输出目录，无孤儿 DLL；单元测试（motorstatus/motorconfig/appconfig）与配置管理（config.json + AppConfig）已存在，ctest 双工具链 3/3 通过；日志分级与文件持久化已落地（CLI 运行日志实测写入 logs/）。本次验证新发现与修复见文末「验证记录」。

## 改进方向与建议

### 方向一：git 项目治理　【已完成】

核心已完成：标准 .gitignore 已建立（覆盖 build 目录、CMake 生成物、CMakeLists.txt.user 等），厂商 SDK 已瘦身为构建必需文件，必需厂商二进制通过白名单直接入库（详见「决策记录」）。仍待办的配套：统一分支/提交规范（conventional commits）、main 保护与受保护分支规则、语义化版本 tag、CI 门禁、CHANGELOG。这些将随方向七落地。

### 方向二：vcpkg 包管理　【待办 · 未启动】

当前项目无任何开源第三方通用依赖，故尚未引入 vcpkg；厂商闭源 SDK 也无法通过 vcpkg 拉取。当前厂商二进制以"直接入库 + .gitignore 白名单"方式引入（见「决策记录」）。当引入 spdlog、yaml-cpp/nlohmann-json 等通用依赖时，建议以 vcpkg.json 声明并用 CMake toolchain 对接，实现版本锁定与可复现构建；届时 CMake 供应商路径可参数化（DVP2\_ROOT/KDC101\_ROOT/TOUCAM\_ROOT）配合 CMake presets。通用依赖与厂商二进制分离、版本可控、团队构建一致。

### 方向三：日志系统　【部分完成】

已完成业务/视图解耦：logMessage 已下沉至 core 的 [kcubemotor.h](file:///d:/CPP/Multi-Controller-Source/src/core/hardware/kcubemotor.h) 并经 Qt 信号供 UI 订阅，接口签名是"业务只管记录、视图只管展示"。日志分级（debug/info/warn/error）与按日期文件持久化已由自研 logger 落地（CLI 运行实测写入 logs/）。待办：是否引入 spdlog 作为统一日志基座（经 vcpkg，注意与 Qt 编译选项一致）替换自研实现，属可选优化而非缺口。

### 方向四：Agent CLI 程序　【已完成并验证】

已新增 `Multi-ControllerCLI` 可执行目标（[src/cli/main.cpp](file:///d:/CPP/Multi-Controller-Source/src/cli/main.cpp)），用 Qt 的 QCommandLineParser 实现参数解析，与 GUI 共用 `mc_core`。支持 --list、--serial、--home、--stop、-m/--move、-r/--relative、--status、--velocity/--acceleration，2026-09 已通过 MSVC Release 构建与运行时验证（--help/--list 正常输出，日志落盘）。用于无人值守、脚本化、自动化电机控制；CLI 参数解析将纳入后续测试体系（见方向七）。遗留：控制台输出中文存在编码乱码（UTF-8 源码 × GBK 控制台），列入方向七处理。

### 方向五：系统架构分层　【已完成并验证】

核心已落地：单文件已按三层重构——`src/app`（GUI 装配）、`src/core`（硬件抽象与纯逻辑，mc\_core 库）、`src/cli`（复用 core）。单位换算与限位校验已下沉 core，状态位解析收敛为 mc::MotorStatus 具名枚举，原 mainwindow\.h 中无用的 DVP2 include 已移除。落地与原始建议的差异见「决策记录」：实际用 `src/app` 承载界面与装配（未单独拆 `src/ui`），且按 YAGNI 暂未引入 IMotor/ICamera 抽象接口（当前仅 KDC101 单实现）。待接入第二种电机或相机时，再依据「架构基线」补接口与适配器。

### 方向六：设计模式优化　【待办】

在已落地的分层基础上，针对具体场景引入贴合的设计模式即可，避免过度设计。KDC101 与 DVP/TOUPCam 分别用适配器模式封装为统一接口，屏蔽厂商 API 差异（当前因单机种未引入，见「架构基线」）。设备生命周期与单实例性用单例或依赖注入管理。硬件连接/未连接/操作中等状态建议用有限状态机明确建模，替代散落的 if/return 判断。单位换算与默认参数用统一配置对象承载。UI 侧保持 MVC/信号槽自然形态，不引入重型框架。核心原则是"接口稳定、实现可替换、状态显式"，让新增设备或相机时只需新增实现类、不改 UI。

### 方向七：软件工程优化　【待办】

最后阶段补齐工程化短板，拆为五条待办：一是配置管理，用 JSON/YAML 读取设备序列号默认选择、速度加速度初始值、限位范围、日志级别与文件路径等，消除代码内硬编码，用户在 GUI 的设置可持久化并在下次启动恢复。二是测试体系，为 core 层的单位换算、状态位解析、限位校验、CLI 参数解析补充单元测试（Qt Test 或 Catch2，经 vcpkg 管理），并用 CTest 让构建即测试（对应「需求与追踪」中测试性需求）。三是持续集成与交付，配 GitHub Actions 在 Windows 执行 configure、build、test，合并前保持健康并产出安装包。四是文档与贡献规范统一，保持 README、DEPENDENCIES、CHANGELOG 与真实代码同步。五是错误处理与用户交互优化，对硬件通信失败给出明确提示与可恢复路径，替代大量平铺的"警告:xxx失败"日志。

### TOUPCam 相机集成专项　【接入就绪 · 待硬件实测】

已完成：3rdparty/ToupTek 按惯例仅入库构建必需文件（include/toupcam.h、lib/x64/toupcam.lib、bin/x64/toupcam.dll），新增 ToupCam 导入库目标接入硬件层，GUI 构建自动拷贝 toupcam.dll（2026-09 实测拷贝成功）；[toup_camera.cpp](file:///d:/CPP/Multi-Controller-Source/src/core/hardware/toup_camera.cpp) 经 QLibrary 提供连接/采帧/保存接口。前置已随 ToupTek 落地完成。待办：连接真实相机实测后台采帧与实时画面显示，验证 ICamera 备选契约。

### 方向八：NSIS 打包　【待办】

引入 Nullsoft Scriptable Install System（NSIS）生成 Windows 安装包：以 windeployqt 收集 Qt 运行库，携带 exe、厂商 SDK 的 dll、示例配置与日志目录，创建桌面快捷方式与开始菜单项，写入必要环境路径。安装包带版本号、卸载程序与升级覆盖逻辑，通过 CMake 打包 target（或在 CI 调 makensis 与 windeployqt）让打包成为构建一等步骤。用 windeployqt 校验依赖，避免干净环境缺 dll 无法启动。依赖方向九图标作安装程序图标。

### 方向九：软件图标　【待办】

为"电机 + 相机多控制器"主题设计 logo，导出多分辨率 ico（含 16/32/48/256 以保证高 DPI 清晰），通过 Windows 资源文件（.rc 的 ICON 指令）挂载到可执行文件，使 exe、任务栏、窗口标题栏、安装包与快捷方式统一；亦可借 Qt qrc 资源系统在窗口层设置 QApplication 图标作兜底。图标同时纳入 NSIS 作安装程序图标，保证从安装到运行全链路视觉统一。

### 方向十：MSYS2 编译工具链支持　【待办】

当前构建链强依赖 MSVC2022。建议新增 MSYS2/MinGW 构建预设（声明编译器、toolchain、可选 vcpkg toolchain），使同源 MSVC 与 MinGW 均可编译。已核实厂商 ABI（见「已核实结论」）：KDC101/DVP2/ToupTek 仅提供 MSVC ABI 的 .lib、无 .a，因此 MinGW 只能编译 core 纯逻辑与测试，硬件适配层受 ABI 限制；CLI 在 CMakeLists 中由 MC_ENABLE_HARDWARE 门控，MinGW 构建不产出 CLI（修正原表述）。2026-09 实测：MinGW 与 MSVC 双路径均构建通过、ctest 3/3 全绿。CI 可在 MSVC 与 MSYS2 两条路径分别验证构建。

## 剩余待办里程碑与门禁

已完成基础治理与架构分层；剩余按里程碑推进，每个里程碑含明确交付物与完成门禁（Gate）。

阶段 M1 · DVP2 死代码清理　【已完成】。经核实 DVPCamera64 一直为 dvp_camera.cpp 所用并被 mc_core 链接，并无死代码；2026-09 构建实证 DVPCamera64.dll 正确部署，无孤儿 DLL。关联需求 FR-Cam。

阶段 M2 · 日志系统与软件图标　【日志/图标已完成，spdlog 待定】。日志分级与文件持久化已由自研 logger 落地（CLI 运行实测写入 logs/），app.ico 经 app.rc 挂载进 exe（构建实证）。待办：是否以 spdlog 替换自研基座（依赖方向二 vcpkg）。关联 FR-Log、NFR-Tool。

阶段 M3 · 配置管理与测试体系　【已完成并验证】。config.json + AppConfig 已实现，单元测试 3 项（motorstatus/motorconfig/appconfig），ctest 在 MinGW 与 MSVC 双工具链 3/3 全绿（2026-09 实测）。待办：CLI 参数解析的单元测试。关联 FR-Cfg、NFR-Test。

阶段 M4 · MSYS2 与 CI　【构建部分已完成，CI 待远端验证】。CMakePresets 已修正为真实环境（C:/Programs/Qt/6.10.1 + Qt 自带 mingw1310_64），MinGW 与 MSVC 双路径本地构建通过；GH Actions workflow 已存在，需在远端实测。修正：MinGW 仅 core/tests，CLI 随硬件层走 MSVC。关联 NFR-Tool、FR-CI。

阶段 M5 · NSIS 打包　【构建阶段已跑通，待安装实测】。deploy.cmake 在构建中实测跑通 windeployqt → installer-staging，makensis 在位；待办：实际生成安装包并在干净环境安装/卸载验证。依赖 M2 图标与 windeployqt。关联 FR-Pkg。

阶段 M6 · TOUPCam 相机接入　【SDK 就绪，待硬件实测】。交付物已落地：3rdparty/ToupTek（include/lib/bin）+ ToupCam 导入目标 + toupcam.dll 自动部署；toup_camera.cpp 提供连接/采帧接口。门禁：连接真实相机并显示实时画面（需硬件，待验证）。关联 FR-Cam。

整体以"先解耦、再扩展"为主线：硬件与 UI 已解耦，后续功能在 core 与接口上叠加，避免在未解耦代码上继续累积复杂度。

## 需求与追踪

功能性需求（FR）

- FR-Con 连接/断开 KDC101，刷新设备列表 → 方向五/四（已完成）。
- FR-Move 归位、绝对/相对移动、停止、设置加减速度；限位校验 → 方向五（已完成，core KCubeMotor）。
- FR-Sta 读取位置与状态位，具名解析 → 方向五（已完成，mc::MotorStatus）。
- FR-CLI CLI 参数化电机操作（--list/--home/--move/--relative/--status 等）→ 方向四（已完成）。
- FR-Log 分级日志 + 控制台/文件持久化 + 轮转 → 方向三（部分，待持久化）→ 阶段 M2。
- FR-Cfg 配置管理（默认设备/速度/限位/日志）持久化与恢复 → 方向七（待办）→ M3。
- FR-Pkg NSIS 安装包，含卸载/快捷方式/依赖收集 → 方向八（待办）→ M5。
- FR-Cam DVP2/TOUPCam 相机接入与实时显示 → TOUPCam 专项（待办）→ M6。
- FR-CI 双工具链持续集成与门禁 → 方向十/七（待办）→ M4。

非功能性需求（NFR）

- NFR-Test core 纯逻辑与 CLI 可单元测试，CTest 接入 → 方向七 → M3。
- NFR-Tool 同一源码支持 MSVC 与 MinGW 编译（core/CLI 为底线）→ 方向十 → M4。
- NFR-Deploy 新克隆开箱可构建、交付安装包可安装 → 方向一/八 → 已完成/M5。
- NFR-Maintain 状态显式、接口可替换、新增设备不改 UI → 方向六 → 随功能接入。

追踪矩阵：上述需求均已在对应方向标注完成状态并映射到待办里程碑门禁，避免"无人认领"需求。

## 涉众与约束

现场调试/操作者：关注易用性、明确错误提示、可恢复操作路径（对应 FR-Con/Move、NFR-Maintain）。设备维护/售后：关注可诊断性（FR-Log 文件轮转）、配置可调（FR-Cfg）。开发者：关注可测试性（NFR-Test）、可构建性（NFR-Deploy/Tool）、接口稳定（NFR-Maintain）。未来相机集成者：关注 ICamera 契约与后台采帧复用（FR-Cam）。约束：厂商闭源二进制仅 MSVC ABI；目标平台 Windows；不引入重型 UI 框架与商用许可。

## 风险评估

- R1 厂商库仅 MSVC ABI，MinGW 无法链接硬件层（高）。缓解：MinGW 只要求 core/CLI 通过，硬件适配层仍用 MSVC；CI 双工具链分级验证（M4）。
- R2 TOUPCam 与 DVP2 接口差异过大（中）。缓解：回退为"各自封装 + UI 按设备类型分发"，不影响分层（见假设/架构基线）。
- R3 硬件不可用导致端到端验证受限（中）。缓解：将单位换算、状态解析、限位校验、CLI 参数解析下沉 core 并配单元测试，硬件操控以 CLI 冒烟与构造验证弥合（M3）。
- R4 范围蔓延/需求无边界（中）。缓解：需求追踪矩阵 + 里程碑门禁锁定每次迭代范围（M3 起生效）。
- R5 厂商二进制入库致仓库膨胀（低）。缓解：白名单仅保留构建必需文件，体积已从约 112.8 MB 降至 \~12 MB。

## 范围与例外

明确不做，除非另行提出：跨平台移植（Linux/macOS）；多相机并发采集框架（当前仅单画面；FR-Cam 限定单相机显示）；商用授权/许可证加密；硬实时控制；远程/网络化控制（当前仅本机）；中文本地化以外的多语言。范围以本文档需求与门禁为准，避免无边界扩展。

## 假设与决策

其一，厂商 SDK 保留在 3rdparty，vcpkg 只负责通用开源依赖，二者不互相替代。其二，TOUPCam 与 DVP2 差异能被统一 ICamera 契约收敛；若差异过大，回退为"各自封装 + UI 按设备类型分发"，回退不影响分层。其三，架构分层时保持现有 UI 行为与 KDC101 控制逻辑不变，只做职责迁移而非功能重写。其四，每里程碑完成后先验证构建与基本操控再推进下一个。其五，以需求追踪与阶段门禁作为系统性交付判据（本文档即按此修订）。

## 决策记录

记录与原始建议不一致的已落地决策，便于后续实现者理解现实。

- 厂商二进制改为"直接入库 + .gitignore 白名单"，而非原始建议的"从仓库剔除或走 LFS"——优先保证新克隆开箱可构建。
- 未引入 vcpkg：当前无开源第三方依赖；待引入 spdlog/yaml-cpp 等时再按方向二接入。
- 未建 IMotor/ICamera 抽象接口：当前仅 KDC101 单机种，按 YAGNI 暂用具体类 KCubeMotor + mc 命名空间状态模型；接入第二种设备/相机时再依据「架构基线」补接口与适配器。
- 目录采用 src/app（含装配+界面），未单独拆 src/ui。
- CLI 与 GUI 共用同一 mc\_core。
- ToupTek SDK 按惯例仅入库构建必需文件（include/toupcam.h、lib/x64/toupcam.lib、bin/x64/toupcam.dll），约 100MB 示例/驱动/winrt 不入库；当前 toup_camera 经 QLibrary 运行期加载，ToupCam 导入目标仅为编译期可用性准备。
- src/core 进一步拆出 hardware/ 子目录（kcubemotor/dvp_camera/toup_camera/sim_camera），纯逻辑与硬件适配边界显式化。
- devicePositionToUm 由 kcubemotor.h/.cpp 下沉为 MotorConfig 静态方法（纯逻辑，MinGW 无硬件亦可单测），修复了 MinGW 构建 test_motorconfig 的链接失败。

## 已核实结论

厂商 ABI 已核实：`3rdparty/` 下 KDC101、DVP2 与 ToupTek 均只提供 MSVC ABI 的 `.lib`，无 MinGW 兼容的 `.a`。因此 MSYS2/MinGW 只能保证 core 纯逻辑与测试可编译链接，硬件适配层受 ABI 限制——为方向十 MinGW 验证范围提供事实依据。2026-09 已用 Qt 6.10.1 在 MinGW（C:/Programs/Qt/Tools/mingw1310_64）与 MSVC（VS 2026）双路径实测构建通过。

## 架构基线

已落地基线：`mc_core` 静态库由 [kcubemotor.cpp](file:///d:/CPP/Multi-Controller-Source/src/core/hardware/kcubemotor.cpp)（KCubeMotor:QObject，封装 CC\_/TLI\_ 并发射 logMessage/connectedChanged 信号）、[motorstatus.cpp](file:///d:/CPP/Multi-Controller-Source/src/core/motorstatus.cpp)（mc::MotorStatus 具名状态模型）、[motorconfig.h](file:///d:/CPP/Multi-Controller-Source/src/core/motorconfig.h)（参数常量 + devicePositionToUm 静态方法）组成；硬件适配层位于 `src/core/hardware/`（kcubemotor/dvp_camera/toup_camera/sim_camera），`src/app` 装配 UI，`src/cli` 复用 mc\_core。core 仅依赖 Qt Core + 厂商 SDK，不依赖 UI。

未来扩展契约（备选签名，按需再实现）：IMotor 提供 connectTo/disconnect/home/moveTo/moveRelative/setVelocity/position/status；ICamera 提供 connect/startCapture/stopCapture/grabFrame/save。引入第二类电机或 TOUPCam 时据此新增实现与适配器，UI 面向接口编程；若厂商差异过大，回退为"各自封装 + UI 按设备类型分发"。

## 验证方式

本文档采用系统化验收（Definition of Done）而非空泛门槛。已完成的四个方向（一/四/五，三之解耦部分）已通过现实验证（配置+构建+链接+CLI 运行）。对剩余待办，以里程碑完成门禁为验收：M1 清理完成（无孤儿 DLL、构建通过）；M2 日志分级落盘、图标统一到 exe/任务栏；M3 ctest 全绿且配置可不编译即调整；M4 MSVC 与 MinGW（core/CLI 必过，硬件层视 ABI）双路径构建通过；M5 干净环境可安装/卸载、无缺 dll；M6 TOUPCam 可连接并实时显示。所有门禁需以实际产物与 ctest 实证，杜绝"仅无报错即断言成功"。

## 验证记录（2026-09-26）

按本文档门禁在本机环境（Qt 6.10.1 at C:/Programs/Qt，Qt 自带 MinGW 13.1.0，VS 2026）实测，结果如下。

双工具链 configure 与 build 全部通过。MinGW（MC_ENABLE_HARDWARE=OFF）产出 mc_core 与 3 个测试 exe；MSVC（Visual Studio 18 2026 生成器，Release）产出 Multi-Controller.exe、Multi-ControllerCLI.exe、测试 exe 及 installer-staging（windeployqt 已在构建期跑通）。厂商 DLL（DVPCamera64.dll、Thorlabs.MotionControl.*.dll、toupcam.dll）均经 POST_BUILD 自动拷贝至输出目录，无缺失、无孤儿 DLL。ctest 在 MinGW 与 MSVC 两条路径均为 3/3 通过（motorstatus/motorconfig/appconfig）。CLI 实测：--help 与 --list 正常输出，日志分级与文件持久化落地（Release/logs/multi-controller_2026-09-26.log）；发现控制台中文输出存在编码乱码（UTF-8 源码 × GBK 控制台），列入方向七。

验证暴露并修复一处真实缺陷：MinGW 构建 test_motorconfig 链接失败（devicePositionToUm 定义位于硬件层 kcubemotor.cpp，硬件关闭时不可见），已将其下沉为 MotorConfig 静态方法，MinGW 与 MSVC 均回归通过。另修正 CMakePresets 的过时配置：Qt 路径由 6.7.3/C:/Qt 改为 6.10.1/C:/Programs/Qt，并修正 MinGW 范围描述（CLI 由 MC_ENABLE_HARDWARE 门控，MinGW 不产出 CLI）。

仍未完成、需后续处理的事项：CI 需在 GitHub Actions 远端实测；NSIS 安装包需在干净环境生成并安装/卸载验证；TOUPCam 需连接真实相机实测采帧与实时画面；CLI 参数解析尚无单元测试；控制台中文乱码待修复。
