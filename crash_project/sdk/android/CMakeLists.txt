cmake_minimum_required(VERSION 3.22.1)

project(ZWCADGuardAndroid LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ZWCADGuard STATIC
    ../core/ZWMobileGuard.cpp
    ../core/Backtrace/ZWMobileGuardBacktrace.cpp
    ../core/Config/ZWMobileGuardInternal.cpp
    ../core/Config/ZWMobileGuardState.cpp
    ../core/Monitor/ZWMobileGuardPOSIX.cpp
    ../core/Monitor/ZWMobileGuardThrow.cpp
    ../core/Record/ZWMobileGuardContext.cpp
    ../core/Record/ZWMobileGuardExceptionRecord.cpp
    ../core/Report/KSJsonUtil/KSFileUtils.c
    ../core/Report/KSJsonUtil/KSJSONCodec.c
    ../core/Report/ZWMobileGuardBinaryImages.cpp
    ../core/Report/ZWMobileGuardRawReport.cpp
    ../core/Report/ZWMobileGuardReportJSON.cpp
    zwcad_guard_jni.cpp
    cxa_throw_hook.cpp
)

target_include_directories(ZWCADGuard PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/Backtrace
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/Config
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/Monitor
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/Record
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/Report
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/Report/KSJsonUtil
)

find_library(log-lib log)

target_link_libraries(ZWCADGuard
    ${log-lib}
    dl
)
