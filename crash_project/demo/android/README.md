# Android Demo 规划

当前仓库中的 Android Demo 已独立放在：

- [ZWCADGuardAndroidDemo](/Users/kwb/Downloads/开题报告/未命名文件夹/crash_project/demo/android/ZWCADGuardAndroidDemo)

该 Demo 的定位是“独立验证工程”，不作为最终 SDK 交付物的一部分。

该工程用于演示：

- Java/Kotlin 未捕获异常
- JNI 边界包装拦截 C++ 异常
- native signal 崩溃
- 图纸上下文设置
- breadcrumb 记录
- 启动后扫描本地崩溃日志
- 上传接口占位
- 图纸本体授权上传开关

正式接入时，业务 JNI 导出函数建议统一引入：

- [zwcad_jni_guard.h](/Users/kwb/Downloads/开题报告/未命名文件夹/crash_project/sdk/android/zwcad_jni_guard.h)

并在每个 JNI 边界函数中套用：

```cpp
ZWCAD_JNI_GUARD_BEGIN
// business code
ZWCAD_JNI_GUARD_END_RETURN(env, fallback)
```
