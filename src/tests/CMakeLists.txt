# 单元测试：通过 CTest 接入，对应 NFR-Test

# MotorStatus 状态位解析测试
qt_add_executable(test_motorstatus test_motorstatus.cpp)
target_link_libraries(test_motorstatus PRIVATE mc_core Qt${QT_VERSION_MAJOR}::Test)
add_test(NAME motorstatus COMMAND test_motorstatus)

# MotorConfig 单位换算与限位校验测试
qt_add_executable(test_motorconfig test_motorconfig.cpp)
target_link_libraries(test_motorconfig PRIVATE mc_core Qt${QT_VERSION_MAJOR}::Test)
add_test(NAME motorconfig COMMAND test_motorconfig)

# AppConfig 配置加载/持久化测试
qt_add_executable(test_appconfig test_appconfig.cpp)
target_link_libraries(test_appconfig PRIVATE mc_core Qt${QT_VERSION_MAJOR}::Test)
add_test(NAME appconfig COMMAND test_appconfig)

# mc_core 静态库中的硬件适配对象（kcubemotor/dvp_camera）会引入厂商 SDK 导入库，
# 任何引用这些符号的测试 exe 在加载期都需要厂商 DLL 位于 exe 同目录。
if(MC_ENABLE_HARDWARE)
    set(MC_TEST_VENDOR_DLLS
        "${CMAKE_SOURCE_DIR}/3rdparty/KDC101/Thorlabs.MotionControl.DeviceManager.dll"
        "${CMAKE_SOURCE_DIR}/3rdparty/KDC101/Thorlabs.MotionControl.KCube.DCServo.dll"
        "${CMAKE_SOURCE_DIR}/3rdparty/DVP2/bin/x64/DVPCamera64.dll"
    )
    foreach(_mc_test test_motorstatus test_motorconfig test_appconfig)
        add_custom_command(TARGET ${_mc_test} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${MC_TEST_VENDOR_DLLS} "$<TARGET_FILE_DIR:${_mc_test}>"
            COMMENT "Copying vendor DLLs for ${_mc_test}"
            VERBATIM
        )
    endforeach()
endif()
