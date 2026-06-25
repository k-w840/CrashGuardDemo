cmake_minimum_required(VERSION 3.22.1)

project(ZWCADGuardAndroid LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ZWCADGuard STATIC
    ../core/dwg_guard.cpp
    ../core/dwg_guard_backtrace.cpp
    zwcad_guard_jni.cpp
    cxa_throw_hook.cpp
)

target_include_directories(ZWCADGuard PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
)

find_library(log-lib log)

target_link_libraries(ZWCADGuard
    ${log-lib}
    dl
)
