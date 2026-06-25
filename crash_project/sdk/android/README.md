# Android 接入说明

## 当前形态

Android 侧当前按“一个 SDK + 宿主工程自有 shared library 链接静态库”的方式组织。

也就是说：

- 最终核心产物是 `ZWCADGuard.a`
- 宿主 App 仍需要有自己的 JNI `so`
- 该 `so` 负责：
  - 链接 `ZWCADGuard.a`
  - 暴露 Java 层会调用到的 JNI 符号
  - 调用业务自身的 native 代码

## 当前目录

- [CMakeLists.txt](/Users/kwb/Downloads/开题报告/未命名文件夹/crash_project/sdk/android/CMakeLists.txt)
- [zwcad_guard_jni.h](/Users/kwb/Downloads/开题报告/未命名文件夹/crash_project/sdk/android/zwcad_guard_jni.h)
- [zwcad_guard_jni.cpp](/Users/kwb/Downloads/开题报告/未命名文件夹/crash_project/sdk/android/zwcad_guard_jni.cpp)
- [zwcad_jni_guard.h](/Users/kwb/Downloads/开题报告/未命名文件夹/crash_project/sdk/android/zwcad_jni_guard.h)
- [cxa_throw_hook.cpp](/Users/kwb/Downloads/开题报告/未命名文件夹/crash_project/sdk/android/cxa_throw_hook.cpp)
- [java/com/zwsoft/zwcadguard/ZWCADGuard.java](/Users/kwb/Downloads/开题报告/未命名文件夹/crash_project/sdk/android/java/com/zwsoft/zwcadguard/ZWCADGuard.java)

## 已具备的接入能力

- JNI 层初始化入口
- breadcrumb 记录
- 活跃图纸上下文记录
- Java 默认未捕获异常处理器
- JNI 边界异常防火墙宏
- native signal 崩溃兜底
- Android 侧 `__cxa_throw` hook 文件

## 宿主工程接入原则

### 1. 宿主先加载自己的 JNI so

Java 层不能直接加载 `.a`，因此宿主需要：

```java
System.loadLibrary("your_host_so");
```

该 `your_host_so` 内部再去链接 `ZWCADGuard.a`。

### 2. Java 层使用 SDK 包装类

可直接复用：

- [ZWCADGuard.java](/Users/kwb/Downloads/开题报告/未命名文件夹/crash_project/sdk/android/java/com/zwsoft/zwcadguard/ZWCADGuard.java)

### 3. 业务 JNI 导出函数必须统一做边界包装

可直接复用：

- [zwcad_jni_guard.h](/Users/kwb/Downloads/开题报告/未命名文件夹/crash_project/sdk/android/zwcad_jni_guard.h)

示例：

```cpp
extern "C" JNIEXPORT jint JNICALL
Java_com_example_demo_NativeBridge_nativeDoWork(JNIEnv* env, jclass clazz) {
    (void)clazz;
    ZWCAD_JNI_GUARD_BEGIN

    // 调用可能抛出 C++ 异常的业务代码
    return 0;

    ZWCAD_JNI_GUARD_END_RETURN(env, -1)
}
```

## 当前仍未完成的部分

- Android Demo 还未做实际编译验证
- 真实上传接口未接入
- 与 zwProject 真实 JNI 边界尚未逐个收口
- Java 崩溃日志当前以文本堆栈形式写入，未做结构化线程栈分解
