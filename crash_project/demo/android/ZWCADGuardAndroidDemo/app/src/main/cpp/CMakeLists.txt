cmake_minimum_required(VERSION 3.22.1)

project(ZWCADGuardDemoNative LANGUAGES C CXX)

add_subdirectory(../../../../../../sdk/android ${CMAKE_CURRENT_BINARY_DIR}/zwcadguard_sdk)

add_library(zwcadguarddemo SHARED
    native-lib.cpp
)

target_include_directories(zwcadguarddemo PRIVATE
    ../../../../../../sdk/android
    ../../../../../../sdk/core
)

find_library(log-lib log)

target_link_libraries(zwcadguarddemo
    ZWCADGuard
    ${log-lib}
)
