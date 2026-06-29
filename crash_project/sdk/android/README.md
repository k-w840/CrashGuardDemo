# Android 接入说明

本文档基于当前仓库最新 Android 代码整理，目标是让业务工程可以按现在这套 `ZWMobileGuard` / `ZWCADGuard` 接口直接完成接入验证。

当前结论先说在前面：

- Android 当前推荐按“源码接入 + 宿主 JNI so 链接 SDK 静态库”的方式接入。
- Java 层通过 `com.zwsoft.zwcadguard.ZWCADGuard` 调用 SDK。
- native 层通过宿主自己的 `.so` 链接 `sdk/android` 和 `sdk/core`。
- JNI 边界统一使用 `zwcad_jni_guard.h` 做异常防火墙。
- 崩溃日志当前落盘为本地 `json` 文件，后续可按业务需要接上传。

## 1. 当前 Android SDK 能力范围

当前 Android 侧已经覆盖：

- Java / Kotlin 未捕获异常记录
- JNI 边界 C++ 异常拦截并转换为 Java `RuntimeException`
- `std::terminate` 兜底
- POSIX signal 崩溃兜底
- `__cxa_throw` 时刻抓取原始 C++ throw 栈
- breadcrumb 操作路径记录
- 活跃图纸上下文记录
- 崩溃日志本地落盘

当前 Android Demo 已演示：

- Java 崩溃
- 受控 JNI C++ 异常
- 未捕获 C++ 异常
- native signal 崩溃
- 图纸上下文设置
- breadcrumb 记录
- 本地日志扫描

## 2. 当前 Android 目录结构

- [sdk/android/CMakeLists.txt](/Users/kwb/CrashGuardDemo/crash_project/sdk/android/CMakeLists.txt)
- [sdk/android/java/com/zwsoft/zwcadguard/ZWCADGuard.java](/Users/kwb/CrashGuardDemo/crash_project/sdk/android/java/com/zwsoft/zwcadguard/ZWCADGuard.java)
- [sdk/android/zwcad_guard_jni.cpp](/Users/kwb/CrashGuardDemo/crash_project/sdk/android/zwcad_guard_jni.cpp)
- [sdk/android/zwcad_jni_guard.h](/Users/kwb/CrashGuardDemo/crash_project/sdk/android/zwcad_jni_guard.h)
- [sdk/android/cxa_throw_hook.cpp](/Users/kwb/CrashGuardDemo/crash_project/sdk/android/cxa_throw_hook.cpp)
- [sdk/core/ZWMobileGuard.h](/Users/kwb/CrashGuardDemo/crash_project/sdk/core/ZWMobileGuard.h)

Demo 参考工程：

- [demo/android/ZWCADGuardAndroidDemo](/Users/kwb/CrashGuardDemo/crash_project/demo/android/ZWCADGuardAndroidDemo)

## 3. 推荐接入方式

当前推荐接法是：

1. 宿主 App 保留自己的 JNI `so`
2. 这个宿主 `so` 通过 CMake `add_subdirectory()` 链接 `sdk/android`
3. Java 层加载宿主 `so`
4. Java 业务代码调用 `ZWCADGuard.initialize(...)`
5. 所有业务 JNI 导出函数统一套 `ZWCAD_JNI_GUARD_*`

原因很直接：

- Java 不能直接加载静态库
- 业务 JNI 代码本来就要留在宿主工程
- 宿主 `so` 统一链接 SDK，最容易跟现有工程融合
- JNI 边界统一收口后，跨语言 C++ 异常防护最稳定

## 4. 接入步骤

### 4.1 Java 层接入

当前对外 Java 包装类是：

- [sdk/android/java/com/zwsoft/zwcadguard/ZWCADGuard.java](/Users/kwb/CrashGuardDemo/crash_project/sdk/android/java/com/zwsoft/zwcadguard/ZWCADGuard.java)

业务工程有两种常见接法：

- 直接把 `sdk/android/java` 作为源码目录加入 Android module
- 把 `ZWCADGuard.java` 拷到业务工程并保持和 SDK 同步

当前 Demo 采用第一种，在 Gradle 中直接把 SDK Java 源码目录加进来：

- [demo/android/ZWCADGuardAndroidDemo/app/build.gradle](/Users/kwb/CrashGuardDemo/crash_project/demo/android/ZWCADGuardAndroidDemo/app/build.gradle:9)

### 4.2 Java 层加载宿主 so

在应用启动早期，先加载宿主自己的 JNI so：

```java
static {
    System.loadLibrary("your_host_so");
}
```

当前 Demo 示例：

- [demo/android/ZWCADGuardAndroidDemo/app/src/main/java/com/zwsoft/zwcadguard/demo/MainActivity.java](/Users/kwb/CrashGuardDemo/crash_project/demo/android/ZWCADGuardAndroidDemo/app/src/main/java/com/zwsoft/zwcadguard/demo/MainActivity.java:24)

### 4.3 Java 初始化 SDK

建议在 Application 或主进程启动早期初始化：

```java
File logDir = new File(getFilesDir(), "zwcadguard_logs");
ZWCADGuard.initialize(getApplicationContext(), logDir.getAbsolutePath());
```

当前 `initialize()` 会自动补齐：

- process name
- apk path
- package name
- version name
- version code
- Android release version
- device model

初始化代码见：

- [sdk/android/java/com/zwsoft/zwcadguard/ZWCADGuard.java](/Users/kwb/CrashGuardDemo/crash_project/sdk/android/java/com/zwsoft/zwcadguard/ZWCADGuard.java:18)

### 4.4 CMake 链接 SDK

宿主 JNI 模块推荐这样接：

```cmake
cmake_minimum_required(VERSION 3.22.1)

project(HostNative LANGUAGES C CXX)

add_subdirectory(/absolute/path/to/sdk/android ${CMAKE_CURRENT_BINARY_DIR}/zwcadguard_sdk)

add_library(your_host_so SHARED
    native-lib.cpp
)

target_include_directories(your_host_so PRIVATE
    /absolute/path/to/sdk/android
    /absolute/path/to/sdk/core
)

find_library(log-lib log)

target_link_libraries(your_host_so
    ZWCADGuard
    ${log-lib}
)
```

当前 Demo 参考：

- [demo/android/ZWCADGuardAndroidDemo/app/src/main/cpp/CMakeLists.txt](/Users/kwb/CrashGuardDemo/crash_project/demo/android/ZWCADGuardAndroidDemo/app/src/main/cpp/CMakeLists.txt:1)

SDK 自己的 CMake 入口：

- [sdk/android/CMakeLists.txt](/Users/kwb/CrashGuardDemo/crash_project/sdk/android/CMakeLists.txt:1)

### 4.5 JNI 边界异常防火墙

业务 JNI 导出函数必须统一引入：

- [sdk/android/zwcad_jni_guard.h](/Users/kwb/CrashGuardDemo/crash_project/sdk/android/zwcad_jni_guard.h)

推荐模板：

```cpp
#include "zwcad_jni_guard.h"

extern "C" JNIEXPORT jint JNICALL
Java_com_example_demo_NativeBridge_nativeDoWork(JNIEnv* env, jclass clazz) {
    (void)clazz;
    ZWCAD_JNI_GUARD_BEGIN

    // 这里调用可能抛 C++ 异常的业务代码
    return 0;

    ZWCAD_JNI_GUARD_END_RETURN(env, -1)
}
```

如果返回类型是 `void`：

```cpp
extern "C" JNIEXPORT void JNICALL
Java_com_example_demo_NativeBridge_nativeDoWork(JNIEnv* env, jclass clazz) {
    (void)clazz;
    ZWCAD_JNI_GUARD_BEGIN

    // business code

    ZWCAD_JNI_GUARD_END_VOID(env)
}
```

当前 Demo 参考：

- [demo/android/ZWCADGuardAndroidDemo/app/src/main/cpp/native-lib.cpp](/Users/kwb/CrashGuardDemo/crash_project/demo/android/ZWCADGuardAndroidDemo/app/src/main/cpp/native-lib.cpp:19)

## 5. 业务侧建议接法

### 5.1 记录 breadcrumb

Java 层直接调用：

```java
ZWCADGuard.addBreadcrumb("Drawing", "Open", "source=cloud,fileId=123456");
```

建议记录这些时机：

- App 启动
- 页面进入
- 图纸打开 / 保存 / 导出
- 命令执行
- 崩溃高发路径上的关键步骤

### 5.2 记录活跃图纸上下文

如果当前业务已拿到图纸元信息，建议在图纸真正成为当前工作对象时调用：

```java
ZWCADGuard.setActiveDrawingContext(
        "A01.dwg",
        "/storage/emulated/0/ZWCAD/A01.dwg",
        1048576L,
        "md5_xxx",
        "file_1001",
        "project_365",
        "外网365项目");
```

接口位置：

- [sdk/android/java/com/zwsoft/zwcadguard/ZWCADGuard.java](/Users/kwb/CrashGuardDemo/crash_project/sdk/android/java/com/zwsoft/zwcadguard/ZWCADGuard.java:55)

### 5.3 图纸关闭时清理上下文

```java
ZWCADGuard.clearActiveDrawing();
```

### 5.4 模拟崩溃写入

不想真把 App 干掉时，可以先验证落盘链路：

```java
ZWCADGuard.simulateCrash("manual smoke test");
```

这条适合做：

- 接入烟测
- 本地日志读取验证
- 上传接口联调前的占位验证

## 6. 当前本地崩溃日志位置

当前 Android 由业务初始化时决定日志目录。

Demo 里使用的是：

```java
new File(getFilesDir(), "zwcadguard_logs")
```

因此 Demo 默认日志目录大致是：

```text
/data/data/com.zwsoft.zwcadguard.demo/files/zwcadguard_logs
```

文件名规则：

```text
crash_<timestamp_us>_<pid>.json
```

## 7. 当前崩溃日志里有什么

当前 JSON 主要包含：

- `report.report`：崩溃基础元数据
- `report.binary_images`：本次进程加载的 native image 列表
- `report.system`：包名、版本、系统版本、设备型号、架构、pid
- `report.crash.error`：signal / cpp / managed 类型信息
- `report.crash.threads[0].backtrace.contents`：native 指令地址列表
- `report.user.breadcrumbs`：用户操作路径
- `report.user.active_drawing`：活跃图纸上下文

序列化逻辑位置：

- [sdk/core/Report/ZWMobileGuardRawReport.cpp](/Users/kwb/CrashGuardDemo/crash_project/sdk/core/Report/ZWMobileGuardRawReport.cpp:319)

## 8. 目前 Android 侧的边界说明

当前 Android 方案有几个需要明确的点：

- Java 未捕获异常会记录成 managed crash
- 受 `ZWCAD_JNI_GUARD_*` 保护的 C++ 异常会转换成 Java `RuntimeException`，不会走 native 崩溃落盘
- 未被保护的 C++ 异常最终可能落到 `std::terminate`
- signal 崩溃会写 native crash 报告
- 当前 JSON 重点是 native 地址、image 列表和业务上下文，不是 Java 层 `.stacktrace` 专用格式

如果业务目标是“所有 JNI 导出函数都不允许把 C++ 异常炸回 Java”，那就必须把 JNI 边界宏覆盖到位。

## 9. 下一步建议

推荐按这个顺序推进：

1. 在正式项目接入 `ZWCADGuard.initialize()`
2. 先打通 breadcrumb 和图纸上下文
3. 选 1 到 2 个 JNI 导出函数试套 `ZWCAD_JNI_GUARD_*`
4. 验证 `simulateCrash()` 能落盘
5. 验证 Java 崩溃 / native 崩溃各自产生什么日志
6. 再接上传
7. 最后跑符号化链路

Android 本地符号化说明见：

- [demo/android/Android符号化说明.md](/Users/kwb/CrashGuardDemo/crash_project/demo/android/Android符号化说明.md)
