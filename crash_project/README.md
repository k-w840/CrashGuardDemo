# ZWCADGuard

`ZWCADGuard` 是面向 `zwProject / 外网365` 的跨语言异常防护与崩溃收集 SDK。  
目标不是只做普通 crash 收集，而是覆盖 C++ 异常跨 Objective-C / Java 边界时的防护、崩溃现场保存、操作路径记录、图纸上下文关联，以及“本地落盘 -> 下次启动扫描 -> 上传接口预留”的完整链路。

## 当前仓库定位

当前仓库已经包含：

- 一套单一 SDK 的代码骨架
- iOS 接入层
- Android 接入层
- iOS Demo
- 独立 Android Demo 骨架
- 需求评审与架构建议文档

当前仓库尚未完成：

- Android Demo 实际编译验证
- 真正可用的上传实现
- 与友盟 / 现有 `ZWCrashReporter` 的正式整合
- 面向 `zwProject` 的业务级接入改造

## 目录结构

```text
crash_project/
├── sdk/
│   ├── core/              # 共享 C/C++ 核心能力
│   ├── ios/               # iOS 接入层
│   └── android/           # Android JNI 接入层、__cxa_throw hook 与边界防火墙辅助头文件
├── demo/
│   ├── ios/               # iOS 验证 Demo
│   └── android/           # 独立 Android 验证工程
├── 需求评审与架构建议.md
└── README.md
```

## 已确认的正式方案口径

- 最终只交付一个 SDK。
- SDK 内部允许分目录分层，但对外不拆成多个独立 SDK。
- iOS 与 Android 都必须支持。
- Android 当前交付形态按 `.a` 处理。
- “支持”的边界以《C++异常跨OCJava捕获异常的防护调研报告》为准。
- Android 正式主路径采用统一 JNI 边界包装，不以全局 `RegisterNatives` Hook 作为主方案。
- 默认不自动上传图纸本体。

## 核心能力

### 1. C++ `throw` 时刻抓原始栈

iOS / Android 都围绕 `__cxa_throw` 记录原始 `throw` 现场，避免异常最终走到 `std::terminate` 时只剩下终止现场。

### 2. 边界防火墙

- iOS：Objective-C++ 边界用 `try/catch` 拦截 C++ 异常
- Android：JNI 边界统一包装，用 `try/catch` 把 C++ 异常转换为 Java pending exception

Android 侧当前提供了 [zwcad_jni_guard.h](/Users/kwb/Downloads/开题报告/未命名文件夹/crash_project/sdk/android/zwcad_jni_guard.h)，用于统一包装 JNI 导出函数。

Android 侧接入说明见：

- [sdk/android/README.md](/Users/kwb/Downloads/开题报告/未命名文件夹/crash_project/sdk/android/README.md)

### 3. 崩溃兜底

- `std::set_terminate`
- Objective-C 未捕获异常处理
- POSIX signal
- iOS 后续可继续增强 Mach 异常处理

### 4. 业务上下文记录

SDK 当前支持记录：

- breadcrumb 操作路径
- 活跃图纸文件名、路径、大小、哈希
- 文件 ID
- 项目 ID
- 项目名

这些字段是为了贴近 `外网365项目.docx` 中的真实图纸与项目上下文。

## Android JNI 边界包装示例

业务 JNI 导出函数建议统一采用边界防火墙宏，而不是依赖全局 Hook：

```cpp
#include "zwcad_jni_guard.h"

JNIEXPORT jint JNICALL Java_com_example_demo_NativeBridge_nativeDoWork(JNIEnv* env, jclass clazz) {
    (void)clazz;
    ZWCAD_JNI_GUARD_BEGIN

    // 调用可能抛出 C++ 异常的业务代码
    return 0;

    ZWCAD_JNI_GUARD_END_RETURN(env, -1)
}
```

## iOS 当前接入方式

初始化：

```objc
[[ZWCADGuard sharedInstance] initializeSDK];
```

记录图纸与项目上下文：

```objc
[[ZWCADGuard sharedInstance] setActiveDrawingName:@"building.dwg"
                                             path:@"/Documents/building.dwg"
                                             size:1048576
                                             hash:@"md5_hash"
                                           fileID:@"dwg_file_1024"
                                        projectID:@"project_365_a01"
                                      projectName:@"BuildingDesign"];
```

记录操作路径：

```objc
[[ZWCADGuard sharedInstance] addBreadcrumbCategory:@"Draw"
                                            action:@"Circle"
                                           details:@"radius=10"];
```

## 当前 iOS Demo 覆盖内容

当前 iOS Demo 已覆盖以下验证方向：

- C++ 未捕获异常
- 边界防火墙拦截
- Objective-C 未捕获异常
- 常见 POSIX signal 崩溃
- 图纸上下文记录
- breadcrumb 记录
- 本地崩溃日志浏览
- 图纸授权上传开关演示

## 后续仍需继续推进的内容

- Android Demo 工程补齐
- `zwProject / 外网365` 真实事件流接入
- 与友盟和 `ZWCrashReporter` 的关系收敛
- 上传参数与接口正式定稿
- 更严格的 signal-safe 落盘路径优化

## 相关文档

- [需求评审与架构建议.md](/Users/kwb/Downloads/开题报告/未命名文件夹/crash_project/需求评审与架构建议.md)
