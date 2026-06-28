# ZWMobileGuard SDK 接入说明

本文档基于当前仓库代码现状整理，目的是让你明天可以直接把 SDK 接到正式项目里验证。

当前结论先说在前面：

- `iOS` 可以先接，推荐优先用“源码接入”方式验证。
- `iOS` 也可以做“静态库接入”，但建议等这轮验证稳定后再固化成库产物。
- `Android` 当前这套新 `ZWMobileGuard*` 代码还没有把 JNI/CMake 层完全迁过来，所以暂时不建议按当前仓库直接正式接入。
- 当前 iOS 崩溃日志是直接写本地 `json` 文件，不是 `.ips`。
- 当前本地 crash 的符号化，核心依赖“同一次构建产物对应的 App 可执行文件 + dSYM”。

## 1. 当前 SDK 能力范围

当前 SDK 已覆盖的 iOS 能力：

- C++ 未捕获异常兜底
- Objective-C 未捕获异常兜底
- POSIX Signal 崩溃兜底
- breadcrumb 操作路径记录
- 活跃图纸上下文记录
- 崩溃日志本地落盘
- 下次启动后读取本地崩溃日志

当前 iOS 本地落盘文件名规则：

- 目录：`Documents/ZWCrashLogs`
- 文件名：`crash_<timestamp_us>_<pid>.json`

例如：

```text
.../Documents/ZWCrashLogs/crash_1782711708073000_12345.json
```

## 2. 推荐接入方式

推荐你明天先用“源码接入”，原因很直接：

- 现在还在验证阶段，源码接入最容易改、最容易打断点、最容易排查
- 你这轮重点是验证正式项目里能不能稳定初始化、能不能正常落盘、崩溃后日志能不能拿出来符号化
- 如果直接先做静态库，出了问题会多一层“库产物是否同步、符号是否对应、头文件是否一致”的排查成本

推荐顺序：

1. 先在正式项目里按“源码接入”跑通
2. 验证几类 crash 都能落盘
3. 验证日志字段够不够
4. 验证符号化流程跑通
5. 再决定是否固化成静态库交付

## 3. iOS 源码接入步骤

### 3.1 需要带进工程的目录

建议把下面这些目录整体加入 iOS 工程：

- `sdk/core`
- `sdk/ios`

如果你是手工拖进 Xcode，建议使用：

- `Create groups`
- 勾选目标 Target

如果你们主工程有自己统一的源码管理方式，也可以直接把这两个目录纳入主工程仓库，再配 Xcode 引用。

### 3.2 需要加入编译的源码

当前 iOS 侧主要会编译这些类型的文件：

- `.cpp`
- `.mm`
- `.c`

其中最关键的入口层文件是：

- `/Users/kwb/CrashGuardDemo/crash_project/sdk/ios/ZWMobileIOSGuard.h`
- `/Users/kwb/CrashGuardDemo/crash_project/sdk/ios/ZWMobileIOSGuard.mm`
- `/Users/kwb/CrashGuardDemo/crash_project/sdk/core/ZWMobileGuard.h`

### 3.3 Xcode Build Settings 建议

至少确认下面几项：

- `Compile Sources As` 保持工程默认即可
- 需要支持 `Objective-C++`，因为有 `.mm` 文件
- `C++ Language Dialect` 建议和主工程保持一致
- `C++ Standard Library` 与主工程保持一致

如果工程里还没配 C++ 运行时，通常 Xcode 会自动处理；如果链接时报 `c++abi` / `libc++` 相关符号缺失，再补查链接设置。

### 3.4 头文件包含方式

业务层直接使用 iOS 包装类：

```objc
#import "ZWMobileIOSGuard.h"
```

如果头文件路径不在默认搜索路径，需要给工程补充 Header Search Paths，确保能搜到：

- `sdk/ios`
- `sdk/core`

### 3.5 初始化时机

建议在 App 启动尽可能早的位置初始化。

当前 Demo 的做法是在：

- [/Users/kwb/CrashGuardDemo/crash_project/demo/ios/ZWCADGuardDemo/ZWCADGuardDemo/AppDelegate.m](/Users/kwb/CrashGuardDemo/crash_project/demo/ios/ZWCADGuardDemo/ZWCADGuardDemo/AppDelegate.m:18)

调用方式：

```objc
[[ZWMobileIOSGuard sharedInstance] initializeSDK];
```

建议正式项目也放在：

- `application:didFinishLaunchingWithOptions:`

如果你们是 `SceneDelegate` / 新架构，也仍然建议在应用初始化早期完成，而不是等业务页面出现后再初始化。

### 3.6 初始化后建议立刻打一条 breadcrumb

例如：

```objc
[[ZWMobileIOSGuard sharedInstance] addBreadcrumbCategory:@"App"
                                                  action:@"Launch"
                                                 details:@"didFinishLaunchingWithOptions"];
```

这样后面如果一启动就崩，日志里至少能看到“App 已启动到哪一步”。

## 4. 业务侧应该怎么接 SDK

### 4.1 记录操作路径

当前公开接口：

```objc
- (void)addBreadcrumbCategory:(NSString *)category
                       action:(NSString *)action
                      details:(NSString *)details;
```

建议记录的时机：

- 页面进入
- 关键按钮点击
- 命令执行
- 图纸打开/关闭
- 文件导入导出
- 崩溃高发路径上的关键步骤

推荐埋点风格：

```objc
[[ZWMobileIOSGuard sharedInstance] addBreadcrumbCategory:@"Drawing"
                                                  action:@"Open"
                                                 details:@"source=cloud,fileId=123456"];
```

建议约定好三段语义：

- `category`：模块，例如 `App` / `Drawing` / `UI` / `Command`
- `action`：动作，例如 `Open` / `Save` / `Export` / `TapButton`
- `details`：附加信息，例如 `fileId=xxx,projectId=yyy`

### 4.2 记录活跃图纸信息

当前公开接口：

```objc
- (void)setActiveDrawingName:(NSString *)name
                        path:(NSString *)path
                        size:(long)size
                        hash:(NSString *)hash
                      fileID:(NSString *)fileID
                   projectID:(NSString *)projectID
                 projectName:(NSString *)projectName;
```

建议在“图纸真正成为当前工作对象”时调用，例如：

- 打开图纸成功后
- 切换当前图纸后
- 从云端拉取并加载完成后

示例：

```objc
[[ZWMobileIOSGuard sharedInstance] setActiveDrawingName:@"A01.dwg"
                                                   path:@"/var/mobile/Containers/Data/Application/.../Documents/A01.dwg"
                                                   size:1048576
                                                   hash:@"md5_xxx"
                                                 fileID:@"file_1001"
                                              projectID:@"project_365"
                                            projectName:@"外网365项目"];
```

### 4.3 图纸关闭时清理上下文

当前公开接口：

```objc
- (void)clearActiveDrawing;
```

建议在这些时机调用：

- 图纸关闭
- 当前图纸被替换
- 退出到无图纸状态

### 4.4 子线程注册备用信号栈

底层公开了：

```c
void zwMobileGuardRegisterThreadSignalStack(void);
```

当前初始化时会为当前线程注册。  
如果你们项目里有自己创建的长期工作线程，并且这些线程里也可能发生栈溢出类崩溃，建议在线程入口额外调用一次。

如果你明天只是先做一期验证，这一步可以先不扩散到所有线程，先把主链路接通。

## 5. iOS 静态库接入步骤

如果你不想源码接入，也可以做静态库接入，但建议作为第二阶段。

### 5.1 静态库接入的典型形态

对外通常会提供：

- 一个 `libZWMobileGuard.a`
- 一组公开头文件

例如：

- `ZWMobileIOSGuard.h`
- `ZWMobileGuard.h`

### 5.2 静态库接入步骤

1. 把 `libZWMobileGuard.a` 加入主工程
2. 把 SDK 对外头文件加入工程
3. 配置 Header Search Paths
4. 确认 Target Link Binary With Libraries 已链接该 `.a`
5. 业务代码中按包装类接口初始化和埋点

### 5.3 为什么当前更推荐源码接入

因为你现在还处在“正式项目首轮试接”阶段，源码接入有几个优势：

- 能快速看见具体哪个文件编不过
- 能直接打断点进 signal/exception/落盘逻辑
- 发生符号问题时更容易确认到底是 SDK 代码还是宿主工程设置问题

## 6. 当前本地 crash 文件长什么样

当前不是写成 iOS 设置里拿到的 `.ips` 文件格式，而是 SDK 自己写的结构化 `json`。

顶层大致是：

```json
{
  "report": {
    "report": {},
    "binary_images": [],
    "system": {},
    "crash": {},
    "user": {}
  }
}
```

里面的关键内容包括：

- `report.report`
  - 崩溃时间
  - report id
  - 进程名
- `report.binary_images`
  - 当前进程加载的二进制镜像列表
  - 每个 image 的基址、大小、uuid、名称、路径、架构
- `report.system`
  - bundle id
  - app version
  - build version
  - os version
  - device model
  - process id
- `report.crash.error`
  - signal / C++ exception / OC exception 等分类
- `report.crash.threads[0].backtrace.contents`
  - 崩溃线程栈帧地址
- `report.crash.threads[0].registers`
  - 寄存器快照
- `report.user.breadcrumbs`
  - 操作路径
- `report.user.active_drawing`
  - 图纸上下文

## 7. 本地 crash 文件怎么拿

当前 iOS 包装层已经提供了接口：

```objc
- (NSArray<NSString *> *)getCrashLogPaths;
- (NSString *)readCrashLogContentAtPath:(NSString *)path;
- (BOOL)deleteCrashLogAtPath:(NSString *)path;
```

也就是说你可以有两种方式拿日志：

### 7.1 App 内部提供“崩溃列表页”

这也是当前 Demo 在做的方式。  
优点是方便测试同学直接在手机里看。

### 7.2 从沙盒导出

如果是开发包，也可以直接从 App 的 `Documents/ZWCrashLogs` 导出 `json` 文件。

## 8. 当前本地 crash 怎么符号化

这一段最关键。

先记住一个原则：

- 符号化一定要使用“和崩溃那次安装包完全对应的构建产物”

也就是说你至少要保留：

- 那次构建出来的 App 可执行文件
- 那次构建对应的 `.dSYM`

### 8.1 iOS 为什么能符号化

因为当前日志里已经写了：

- 栈帧地址 `instruction_addr`
- 二进制镜像基址 `image_addr`
- image 大小 `image_size`
- image 的 `uuid`

这几项已经够后端或本地离线符号化使用。

### 8.2 先判断某一帧属于哪个 image

日志里每个栈帧只有地址，例如：

```text
instruction_addr = 0x104123abc
```

你需要在 `binary_images` 里找到满足下面条件的 image：

```text
image_addr <= instruction_addr < image_addr + image_size
```

找到后，就知道这帧属于哪个二进制。

### 8.3 如果 SDK 代码是静态库链接进主 App，会用哪个 dSYM

如果 SDK 最终是以静态库方式链接进主 App：

- SDK 里的符号会并入主 App 可执行文件
- 符号化时通常看“主 App 的 dSYM”

也就是说，哪怕崩在 SDK 代码里，只要它最终被静态链接进宿主 App，通常还是用宿主 App 对应的 dSYM 来还原。

不是“SDK 一定单独有一份 dSYM”。

### 8.4 如果是独立动态 Framework

如果将来 SDK 变成独立动态库或独立 framework：

- 那就要使用那个独立二进制自己的 dSYM

所以到底用哪个 dSYM，不是看“代码逻辑属于谁”，而是看“崩溃地址最终落在哪个二进制 image 里”。

### 8.5 本地用 `atos` 符号化

当你已经知道：

- 崩溃地址 `instruction_addr`
- 所属 image 的 `image_addr`
- 所属 image 的二进制文件
- 对应的 dSYM

就可以用类似下面的方式本地符号化：

```bash
atos -arch arm64 -o <AppBinaryPath> -l <image_addr> <instruction_addr>
```

例如：

```bash
atos -arch arm64 -o ZWCAD.app/ZWCAD -l 0x104000000 0x104123abc
```

这里参数含义：

- `-o`：那次构建对应的可执行文件
- `-l`：该 image 在崩溃时的加载基址，也就是日志里的 `image_addr`
- 最后一个参数：要解析的帧地址，也就是 `instruction_addr`

### 8.6 如何确认 dSYM 是否匹配

可以用：

```bash
dwarfdump --uuid <YourApp.dSYM>
```

看 dSYM 的 UUID 是否和日志里 `binary_images` 对应 image 的 `uuid` 一致。

只有 UUID 对上，符号化结果才可信。

### 8.7 后端符号化需要上传什么

如果你们后端要做自动符号化，至少要保存：

- 本地 crash json 原文
- 对应构建版本号
- 对应 build 产物的 dSYM

后端按日志中的：

- `uuid`
- `image_addr`
- `instruction_addr`

即可完成离线符号化。

### 8.8 用项目自带脚本批量符号化

当前工程已经提供了一个本地批量符号化脚本：

- [tools/symbolicate_zwmobileguard.py](/Users/kwb/CrashGuardDemo/crash_project/demo/ios/ZWCADGuardDemo/tools/symbolicate_zwmobileguard.py)

它是按当前 `ZWMobileGuard` 落地的 crash json 格式写的，适合你现在这套日志。

最常用命令：

```bash
python3 /Users/kwb/CrashGuardDemo/crash_project/demo/ios/ZWCADGuardDemo/tools/symbolicate_zwmobileguard.py \
  --report /path/to/crash_123.json \
  --main-dsym /path/to/ZWCADGuardDemo.app.dSYM
```

如果你手上拿的是主程序可执行文件，也可以这样：

```bash
python3 /Users/kwb/CrashGuardDemo/crash_project/demo/ios/ZWCADGuardDemo/tools/symbolicate_zwmobileguard.py \
  --report /path/to/crash_123.json \
  --main-binary /path/to/ZWCADGuardDemo.app/ZWCADGuardDemo
```

如果要把结果写到文件：

```bash
python3 /Users/kwb/CrashGuardDemo/crash_project/demo/ios/ZWCADGuardDemo/tools/symbolicate_zwmobileguard.py \
  --report /path/to/crash_123.json \
  --main-dsym /path/to/ZWCADGuardDemo.app.dSYM \
  --output /tmp/symbolicated.txt
```

如果你要一起解自定义 framework / 动态库，可以继续补 image 映射：

```bash
python3 /Users/kwb/CrashGuardDemo/crash_project/demo/ios/ZWCADGuardDemo/tools/symbolicate_zwmobileguard.py \
  --report /path/to/crash_123.json \
  --main-dsym /path/to/ZWCADGuardDemo.app.dSYM \
  --image MyFramework=/path/to/MyFramework.framework.dSYM
```

如果想在输出里顺便看寄存器：

```bash
python3 /Users/kwb/CrashGuardDemo/crash_project/demo/ios/ZWCADGuardDemo/tools/symbolicate_zwmobileguard.py \
  --report /path/to/crash_123.json \
  --main-dsym /path/to/ZWCADGuardDemo.app.dSYM \
  --show-registers
```

脚本会做这些事：

- 读取 crash json
- 按 `instruction_addr` 命中所属 `binary_image`
- 批量调用 `atos`
- 输出主线程回溯的符号化结果

当前脚本更适合解“主程序 + 静态链接进主程序的 SDK 代码”。  
如果是其他独立 framework，建议继续用 `--image` 传入对应符号文件路径。

## 9. 当前日志里哪些字段最值得你先验证

明天正式接入后，我建议你优先验证下面这些字段是否都正确：

### 9.1 基础信息

- `bundle id`
- `app version`
- `build version`
- `process_name`
- `device model`
- `os version`

### 9.2 崩溃现场

- `error.type`
- `signal.signal`
- `signal.code`
- `error.address`
- `backtrace.contents`
- `registers.basic.pc`

### 9.3 业务上下文

- `breadcrumbs` 是否有最近几步操作
- `active_drawing.name/path/file_id/project_id` 是否正确

### 9.4 image 信息

- `binary_images` 是否完整
- 崩溃帧是否能在某个 image 地址范围内命中
- `uuid` 是否能和 dSYM 对上

## 10. 建议你明天的验证顺序

建议按这个顺序来，效率最高：

1. 先把 SDK 源码接进正式项目
2. 只做初始化，不先改太多业务逻辑
3. 启动后打一条 `App Launch` breadcrumb
4. 在一个稳定入口手动打一条图纸上下文
5. 人工制造一次简单崩溃
6. 确认本地生成 `crash_*.json`
7. 导出该文件，检查字段完整性
8. 用那次构建的 dSYM 做一次本地 `atos` 符号化
9. 再补更多业务 breadcrumb

推荐先验证的崩溃类型：

- Objective-C 未捕获异常
- C++ 未捕获异常
- `SIGSEGV`

## 11. 当前版本你需要知道的限制

### 11.1 当前不是 `.ips`

当前落盘不是系统 `.ips` 文件，而是 SDK 自定义 `json`。

这不是问题，只要里面包含：

- 栈地址
- image 地址
- uuid
- 版本信息

后端一样可以符号化。

### 11.2 当前上传接口还只是占位

`ZWMobileIOSGuard.mm` 里虽然有：

- `uploadCrashLogAtPath:uploadDrawing:completion:`

但当前真正上传逻辑还是示例性质，不是正式网络上报实现。  
所以你明天接入时，先把目标放在：

- 能初始化
- 能埋 breadcrumb
- 能落盘
- 能导出
- 能符号化

不要把“正式上传”作为第一天阻塞项。

### 11.3 Android 先不要按当前仓库直接上正式项目

当前 Android JNI 层还是旧接口，和新 `ZWMobileGuard*` 核心层没有完全对齐。  
如果你明天的目标是“先在正式项目试一轮”，建议只做 iOS。

## 12. 明天接入时最小可行代码

### 12.1 App 启动初始化

```objc
#import "ZWMobileIOSGuard.h"

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    [[ZWMobileIOSGuard sharedInstance] initializeSDK];
    [[ZWMobileIOSGuard sharedInstance] addBreadcrumbCategory:@"App"
                                                      action:@"Launch"
                                                     details:@"didFinishLaunchingWithOptions"];
    return YES;
}
```

### 12.2 打开图纸时记录上下文

```objc
[[ZWMobileIOSGuard sharedInstance] setActiveDrawingName:drawingName
                                                   path:drawingPath
                                                   size:fileSize
                                                   hash:fileHash
                                                 fileID:fileId
                                              projectID:projectId
                                            projectName:projectName];

[[ZWMobileIOSGuard sharedInstance] addBreadcrumbCategory:@"Drawing"
                                                  action:@"OpenSuccess"
                                                 details:[NSString stringWithFormat:@"fileId=%@", fileId ?: @""]];
```

### 12.3 关键操作记录 breadcrumb

```objc
[[ZWMobileIOSGuard sharedInstance] addBreadcrumbCategory:@"Command"
                                                  action:@"ExportPDF"
                                                 details:@"source=drawing_detail_page"];
```

### 12.4 关闭图纸时清理

```objc
[[ZWMobileIOSGuard sharedInstance] clearActiveDrawing];
[[ZWMobileIOSGuard sharedInstance] addBreadcrumbCategory:@"Drawing"
                                                  action:@"Close"
                                                 details:@"user_close"];
```

## 13. 接入完成后的自检清单

你明天接完后，可以按这个清单快速自检：

- 工程能正常编译
- App 启动时 SDK 初始化成功
- 沙盒里已创建 `Documents/ZWCrashLogs`
- 触发崩溃后生成 `crash_*.json`
- `json` 中有 `binary_images`
- `json` 中有 `breadcrumbs`
- `json` 中有 `active_drawing`
- `instruction_addr` 能命中某个 `binary_images` 范围
- `uuid` 能和那次构建的 dSYM 对上
- `atos` 能还原出可读函数名

## 14. 最后建议

如果你明天是“正式项目第一次试接”，我的建议很明确：

- 先只接 iOS
- 先用源码接入
- 先不纠结上传
- 先把落盘和符号化链路跑通

只要这四件事成立：

- 能初始化
- 能记录 breadcrumb / 图纸上下文
- 能崩溃后落盘
- 能拿到 dSYM 做符号化

这轮试接就是成功的。
