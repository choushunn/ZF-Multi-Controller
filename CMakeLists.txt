cmake_minimum_required(VERSION 3.16)

project(Multi-Controller VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- 选项 ----
option(MC_ENABLE_HARDWARE "Enable hardware adapter layer (requires MSVC ABI vendor SDKs)" ON)
option(MC_ENABLE_TESTS "Build unit tests" ON)
option(MC_ENABLE_SIM_CAMERA "Build simulated camera (no hardware needed)" ON)

# GUI 和 CLI 依赖硬件层（KCubeMotor），MinGW 构建时禁用
if(NOT MC_ENABLE_HARDWARE)
    set(MC_ENABLE_GUI OFF)
else()
    option(MC_ENABLE_GUI "Build GUI executable" ON)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui)
find_package(Qt${QT_VERSION_MAJOR} OPTIONAL_COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} OPTIONAL_COMPONENTS Test)

# ---- 厂商 SDK（仅硬件层启用时需要） ----
if(MC_ENABLE_HARDWARE)
    add_subdirectory(3rdparty/KDC101)
    add_subdirectory(3rdparty/DVP2)
endif()

# ---- Core library ----
# 纯逻辑层（无硬件依赖，MinGW 也可编译）
set(MC_CORE_SOURCES
        src/core/motorstatus.cpp
        src/core/motorstatus.h
        src/core/motorconfig.h
        src/core/logger.cpp
        src/core/logger.h
        src/core/appconfig.cpp
        src/core/appconfig.h
        src/core/icamera.h
)

# 硬件适配层（依赖厂商 SDK，仅 MSVC ABI）
if(MC_ENABLE_HARDWARE)
    list(APPEND MC_CORE_SOURCES
        src/core/kcubemotor.cpp
        src/core/kcubemotor.h
        src/core/dvp_camera.cpp
        src/core/dvp_camera.h
        src/core/toup_camera.cpp
        src/core/toup_camera.h
    )
endif()

add_library(mc_core STATIC ${MC_CORE_SOURCES})
target_include_directories(mc_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(mc_core PUBLIC Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui)

if(MC_ENABLE_HARDWARE)
    target_link_libraries(mc_core PUBLIC KDC101_Components DVPCamera64)
    target_compile_definitions(mc_core PUBLIC MC_HAS_HARDWARE=1)
else()
    target_compile_definitions(mc_core PUBLIC MC_HAS_HARDWARE=0)
endif()

if(MC_ENABLE_SIM_CAMERA)
    target_compile_definitions(mc_core PUBLIC MC_HAS_SIM_CAMERA=1)
endif()

# ---- GUI executable（需 Widgets + 硬件层） ----
if(MC_ENABLE_GUI AND TARGET Qt${QT_VERSION_MAJOR}::Widgets)
    set(PROJECT_SOURCES
            src/app/main.cpp
            src/app/mainwindow.cpp
            src/app/mainwindow.h
            src/app/mainwindow.ui
            src/resources/app.qrc
            src/resources/app.rc
    )

    if(MC_ENABLE_SIM_CAMERA)
        list(APPEND PROJECT_SOURCES src/core/sim_camera.h)
    endif()

    qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )

    # 确保 .rc 文件在图标变化时重新编译
    set_source_files_properties(src/resources/app.rc PROPERTIES
        OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/icons/app.ico"
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets mc_core)

    # ---- POST_BUILD: copy vendor DLLs to GUI output directory ----
    if(MC_ENABLE_HARDWARE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/3rdparty/KDC101/Thorlabs.MotionControl.DeviceManager.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Thorlabs.MotionControl.DeviceManager.dll"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/3rdparty/KDC101/Thorlabs.MotionControl.KCube.DCServo.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Thorlabs.MotionControl.KCube.DCServo.dll"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/3rdparty/DVP2/bin/x64/DVPCamera64.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/DVPCamera64.dll"
            COMMENT "Copying vendor DLLs to GUI output directory"
        )
    endif()

    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
    )

    qt_finalize_executable(${PROJECT_NAME})

    install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# ---- CLI executable（需硬件层，因为 CLI 控制电机） ----
if(MC_ENABLE_HARDWARE)
    qt_add_executable(${PROJECT_NAME}CLI
            src/cli/main.cpp
    )
    target_link_libraries(${PROJECT_NAME}CLI PRIVATE Qt${QT_VERSION_MAJOR}::Core mc_core)

    # POST_BUILD: copy KDC101 DLLs to CLI output directory
    add_custom_command(TARGET ${PROJECT_NAME}CLI POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/3rdparty/KDC101/Thorlabs.MotionControl.DeviceManager.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}CLI>/Thorlabs.MotionControl.DeviceManager.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/3rdparty/KDC101/Thorlabs.MotionControl.KCube.DCServo.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}CLI>/Thorlabs.MotionControl.KCube.DCServo.dll"
        COMMENT "Copying KDC101 DLLs to CLI output directory"
    )

    install(TARGETS ${PROJECT_NAME}CLI
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

include(GNUInstallDirs)

# ---- Tests ----
if(MC_ENABLE_TESTS AND Qt${QT_VERSION_MAJOR}Test_FOUND)
    enable_testing()
    add_subdirectory(src/tests)
endif()

# ---- CPack / NSIS packaging ----
# 使用 packaging/deploy.cmake 统一管理 windeployqt + NSIS 打包
# 仅 GUI + 硬件层启用时打包（CLI 和 GUI 都需要存在）
if(MC_ENABLE_GUI AND MC_ENABLE_HARDWARE)
    include("${CMAKE_SOURCE_DIR}/packaging/deploy.cmake")
endif()
