# Android 符号化说明

本文档说明当前 `ZWMobileGuard` Android 崩溃日志怎么做本地离线符号化，以及后端要保存哪些构建产物。

当前结论先说在前面：

- Android 符号化一定要使用“和崩溃那次安装包完全对应的 native so”。
- 如果开启了符号裁剪，还需要保留未裁剪符号文件。
- 当前日志里已经有 `binary_images` 和 `instruction_addr`，足够做 native 栈离线还原。
- Java managed 异常主要看日志里的异常名和 Java stacktrace 文本；native 符号化重点是 signal / C++ 崩溃。

## 1. 当前日志里哪些字段和符号化有关

当前 JSON 序列化逻辑见：

- [sdk/core/Report/ZWMobileGuardRawReport.cpp](/Users/kwb/CrashGuardDemo/crash_project/sdk/core/Report/ZWMobileGuardRawReport.cpp:319)

对 Android native 符号化最关键的是这几块：

- `report.binary_images[*].image_addr`
- `report.binary_images[*].image_size`
- `report.binary_images[*].name`
- `report.binary_images[*].path`
- `report.crash.threads[0].backtrace.contents[*].instruction_addr`
- `report.system.cpu_arch`

它们的意义分别是：

- `image_addr`：该 native image 在崩溃进程中的加载基址
- `image_size`：image 大小，用来判断某个地址落在哪个 image 内
- `name` / `path`：对应哪个 so
- `instruction_addr`：原始 PC / 返回地址
- `cpu_arch`：符号化时要用对架构

## 2. Android 为什么能做离线符号化

因为当前日志已经保留了：

- 绝对指令地址
- image 的加载基址
- image 名称和路径

所以可以先做这一步：

```text
relative_pc = instruction_addr - image_addr
```

然后再用对应版本的 `so` 去查符号。

这和 iOS 用 `atos` / dSYM 的思路是同一类问题，只是 Android 更常见的是：

- `llvm-addr2line`
- `llvm-symbolizer`
- `ndk-stack`

## 3. 你必须保留哪些构建产物

至少保留：

1. 崩溃那次版本对应的 APK / AAB 版本号
2. 对应 ABI 的 native so
3. 未 strip 的符号文件，或者带调试符号的 so
4. 该版本的 Git commit / CI build number

推荐按 ABI 分目录保存，例如：

```text
symbols/
  1.0.0+1001/
    arm64-v8a/libzwcadguarddemo.so
    armeabi-v7a/libzwcadguarddemo.so
```

如果业务侧还有别的 native 模块，也要一并保留它们对应的 so。

## 4. 当前最常见的符号化对象

当前最常见的是这几类：

- 宿主 JNI so，例如 `libzwcadguarddemo.so`
- 业务自己的其他 so
- 系统 so，例如 `libc.so`

一般来说：

- 业务自有 so 可以完整符号化
- 系统 so 通常只能看到系统符号，不会还原你业务源码行号

## 5. 手工判断某个地址属于哪个 so

假设日志里有：

```json
{
  "image_addr": 735929470976,
  "image_size": 241664,
  "name": "libzwcadguarddemo.so"
}
```

以及某一帧：

```json
{
  "instruction_addr": 735929498124
}
```

则先判断：

```text
image_addr <= instruction_addr < image_addr + image_size
```

如果成立，这一帧就属于这个 image。

然后计算：

```text
relative_pc = instruction_addr - image_addr
```

接着再拿这个 `relative_pc` 去查符号。

## 6. 用 `llvm-addr2line` 本地符号化

这是最稳的一种方式。

命令大致如下：

```bash
llvm-addr2line -Cfpe /path/to/libzwcadguarddemo.so 0x69fc
```

参数含义：

- `-C`：demangle C++
- `-f`：输出函数名
- `-p`：单行友好格式
- `-e`：指定 so

如果你手里拿到的是十进制相对地址，先转成十六进制再查更顺手。

## 7. 用 `llvm-symbolizer` 本地符号化

也可以用：

```bash
llvm-symbolizer --obj=/path/to/libzwcadguarddemo.so 0x69fc
```

这个输出通常会更适合脚本处理。

## 8. `ndk-stack` 什么时候有用

`ndk-stack` 更适合解析 Android tombstone 或 logcat 原始 backtrace 文本。

当前 `ZWMobileGuard` 落的是自定义 JSON，不是 tombstone 原文，所以：

- `ndk-stack` 不是当前主流程
- 如果你们后续把 crash 同步写成 logcat 样式，`ndk-stack` 就会更方便

因此当前项目更推荐：

- 直接从 JSON 中读 `instruction_addr`
- 自己匹配 `binary_images`
- 再调用 `llvm-addr2line` 或 `llvm-symbolizer`

## 9. 一个实际的离线符号化流程

建议按这个顺序来：

1. 拿到崩溃 json
2. 确认版本号、ABI、包名
3. 找到同版本对应的未 strip so
4. 从 `binary_images` 中找到目标 image 的 `image_addr`
5. 对每个 `instruction_addr` 计算 `relative_pc`
6. 用 `llvm-addr2line` 或 `llvm-symbolizer` 查函数名和行号
7. 输出成可读栈

## 10. Java 崩溃和 native 符号化的区别

这块很容易混，所以单独说明一下。

### 10.1 Java / Kotlin 未捕获异常

Java managed 异常当前会记录：

- 异常类名
- 异常 message
- Java stacktrace 文本

它主要靠文本堆栈定位，不需要 native so 符号化。

### 10.2 JNI C++ 异常

如果 JNI 边界用了 `ZWCAD_JNI_GUARD_*`：

- C++ 异常会被转成 Java `RuntimeException`
- 这类问题优先看 Java 抛出的 message 和业务 JNI 边界

如果 JNI 边界没包住：

- 可能最终落到 native 崩溃
- 这时才需要做 native 栈符号化

### 10.3 signal 崩溃

例如：

- `SIGSEGV`
- `SIGABRT`
- `SIGILL`

这类最需要 native 符号化。

## 11. 后端自动符号化至少要保存什么

如果后端想做自动还原，建议每次发版时就把这些元数据存好：

- app version
- build version
- ABI
- so 文件名
- so 构建产物路径
- Git commit
- 是否 strip

后端拿到 crash 后：

1. 根据版本找到构建产物
2. 根据 ABI 和 image 名选择正确 so
3. 计算 `relative_pc`
4. 调 `addr2line` / `symbolizer`
5. 输出归档后的可读栈

## 12. 当前项目下最实用的验证方式

你现在最值得跑的是这套小闭环：

1. Android demo 或正式工程接入 SDK
2. 触发一次 `signal` 崩溃
3. 导出 `json`
4. 找到对应 ABI 的 `libzwcadguarddemo.so`
5. 手工拿 1 到 3 个地址跑 `llvm-addr2line`
6. 确认能还原到函数名

一旦这条链跑通，后面无论是本地脚本还是后端批量化，都是工程化问题了。

## 13. 当前文档对应的接入入口

Android SDK 接入说明见：

- [sdk/android/README.md](/Users/kwb/CrashGuardDemo/crash_project/sdk/android/README.md)
