# CMakeLists.txt for Thorlabs KDC101 library
cmake_minimum_required(VERSION 3.16)

# Define the KDC101 library
add_library(KDC101 STATIC IMPORTED)

# Set the library file path
set_target_properties(KDC101 PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/Thorlabs.MotionControl.KCube.DCServo.lib"
)

# Add include directory
target_include_directories(KDC101 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/inc
)

# Create an interface library that combines all KDC101 components
add_library(KDC101_Components INTERFACE)
target_link_libraries(KDC101_Components INTERFACE
    KDC101
)
target_include_directories(KDC101_Components INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/inc
)

# Set variables for DLL paths to be used by the parent project
set(KDC101_DLL_PATHS
    "${CMAKE_CURRENT_LIST_DIR}/Thorlabs.MotionControl.DeviceManager.dll"
    "${CMAKE_CURRENT_LIST_DIR}/Thorlabs.MotionControl.KCube.DCServo.dll"
    PARENT_SCOPE
)