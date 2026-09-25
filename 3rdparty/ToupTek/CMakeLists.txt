# CMakeLists.txt for ToupTek ToupCam camera SDK
cmake_minimum_required(VERSION 3.16)

set(TOUPTEK_ROOT ${CMAKE_CURRENT_LIST_DIR})

# Imported library target
# 当前 toup_camera.cpp 经 QLibrary 运行期加载 toupcam.dll，不直接编译链接；
# 声明导入目标以保持与其他厂商 SDK 一致，便于日后切换为编译期链接。
add_library(ToupCam STATIC IMPORTED GLOBAL)
set_target_properties(ToupCam PROPERTIES
    IMPORTED_LOCATION "${TOUPTEK_ROOT}/lib/x64/toupcam.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${TOUPTEK_ROOT}/include"
)

# DLL 路径供父工程 POST_BUILD 拷贝到可执行文件目录
set(TOUPTEK_DLL_PATHS
    "${TOUPTEK_ROOT}/bin/x64/toupcam.dll"
    PARENT_SCOPE
)
