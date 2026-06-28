#import "ZWMobileIOSGuard.h"
#include "ZWMobileGuard.h"
#import <UIKit/UIKit.h>
#import <sys/utsname.h>

extern "C" void zwMobileGuardRecordObjCCrash(const char* name, const char* reason, void** frames, int count);

// OC 原生异常处理
static void zwMobileGuardUncaughtExceptionHandler(NSException *exception) {
    // 获取崩溃堆栈的返回地址
    NSArray *callStack = exception.callStackReturnAddresses;
    NSUInteger count = callStack.count;
    void *frames[count];
    for (NSUInteger i = 0; i < count; i++) {
        // 地址转换为指针类型
        frames[i] = (void *)[callStack[i] unsignedLongValue];
    }
    
    // 记录崩溃异常到本地
    zwMobileGuardRecordObjCCrash([exception.name UTF8String],
                               [exception.reason UTF8String],
                               frames,
                               (int)count);
}

@interface ZWMobileIOSGuard()
@property(nonatomic, copy) NSString *logDir;

@end

@implementation ZWMobileIOSGuard

+ (instancetype)sharedInstance {
    static ZWMobileIOSGuard *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[ZWMobileIOSGuard alloc] init];
    });
    return instance;
}

- (BOOL)initializeSDK {
    // 获取 iOS 沙盒 Document 目录
    NSString *cacheDir = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    // 指定崩溃日志存放文件夹
    self.logDir = [cacheDir stringByAppendingPathComponent:@"ZWCrashLogs"];
    
    // 获取应用和环境元数据
    NSBundle *mainBundle = [NSBundle mainBundle];
    NSString *processPath = [mainBundle executablePath] ?: @"";
    NSString *processName = [processPath lastPathComponent] ?: @"";
    NSString *bundleId = [mainBundle bundleIdentifier] ?: @"";
    NSString *appVersion = [mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"] ?: @"";
    NSString *buildVersion = [mainBundle objectForInfoDictionaryKey:@"CFBundleVersion"] ?: @"";
    
    NSString *osVersion = [NSString stringWithFormat:@"iOS %@", [UIDevice currentDevice].systemVersion];
    
    // 使用 std utsname 现成方法获取具体硬件设备型号（如 iPhone16,2）
    struct utsname systemInfo;
    uname(&systemInfo);
    NSString *deviceModel = [NSString stringWithCString:systemInfo.machine encoding:NSUTF8StringEncoding] ?: @"";
    
    // 初始化 C++ 监控组件（注册 std::terminate 与 POSIX 信号拦截）
    int res = zwMobileGuardInit([self.logDir UTF8String],
                                [processName UTF8String],
                                [processPath UTF8String],
                                [bundleId UTF8String],
                                [appVersion UTF8String],
                                [buildVersion UTF8String],
                                [osVersion UTF8String],
                                [deviceModel UTF8String]);
    // 0为初始化成功
    if (res) {
        NSLog(@"[ZWMobileIOSGuard]初始化失败 %d", res);
        return NO;
    }
    
    // 注册 OC 异常拦截器
    NSSetUncaughtExceptionHandler(&zwMobileGuardUncaughtExceptionHandler);
    
    NSLog(@"[ZWMobileIOSGuard]初始化成功:%@", self.logDir);
    return YES;
}

- (void)setActiveDrawingName:(NSString *)name
                        path:(NSString *)path
                        size:(long)size
                        hash:(NSString *)hash
                      fileID:(NSString *)fileID
                   projectID:(NSString *)projectID
                 projectName:(NSString *)projectName {
    zwMobileGuardSetActiveDrawingContext([name UTF8String],
                                         [path UTF8String],
                                         size,
                                         [hash UTF8String],
                                         [fileID UTF8String],
                                         [projectID UTF8String],
                                         [projectName UTF8String]);
}

- (void)clearActiveDrawing {
    // 关闭图纸时，清除底层的图纸元数据关联
    zwMobileGuardClearActiveDrawing();
}

// 记录用户操作日志
- (void)addBreadcrumbCategory:(NSString *)category
                       action:(NSString *)action
                      details:(NSString *)details {
    zwMobileGuardAddBreadcrumb([category UTF8String],
                             [action UTF8String],
                             [details UTF8String]);
}

- (NSArray<NSString *> *)getCrashLogPaths {
    if (!self.logDir) return @[];
    
    NSError *error = nil;
    NSArray<NSString *> *files = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:self.logDir error:&error];
    if (error) {
        NSLog(@"[ZWMobileIOSGuard] 获取崩溃存储路径失败:%@", error);
        return @[];
    }
    
    NSMutableArray<NSString *> *paths = [NSMutableArray array];
    for (NSString *file in files) {
        // 过滤以 crash_ 开头并以 .json 结尾的崩溃报告文件
        if ([file hasPrefix:@"crash_"] && [file hasSuffix:@".json"]) {
            [paths addObject:[self.logDir stringByAppendingPathComponent:file]];
        }
    }
    
    return [paths sortedArrayUsingSelector:@selector(localizedStandardCompare:)];
}

- (NSString *)readCrashLogContentAtPath:(NSString *)path {
    NSError *error = nil;
    NSString *content = [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:&error];
    if (error) {
        NSLog(@"[ZWMobileIOSGuard] 读取崩溃文件失败: %@，%@", path, error);
        return @"";
    }
    return content;
}

- (void)uploadCrashLogAtPath:(NSString *)logPath
               uploadDrawing:(BOOL)uploadDrawing
                  completion:(void (^)(BOOL success, NSString *message))completion {
    // 读取日志内容
    NSString *logContent = [self readCrashLogContentAtPath:logPath];
    if ([logContent length] <= 0) {
        if (completion) completion(NO, @"崩溃日志内容为空");
        return;
    }
    
//    __block NSString *drawingPath = nil;
//    __block NSString *drawingName = nil;
//    
//    NSData *jsonData = [logContent dataUsingEncoding:NSUTF8StringEncoding];
//    if (jsonData) {
//        NSError *error = nil;
//        NSDictionary *jsonDict = [NSJSONSerialization JSONObjectWithData:jsonData options:0 error:&error];
//        if (!error && [jsonDict isKindOfClass:[NSDictionary class]]) {
//            NSDictionary *activeDrawing = jsonDict[@"active_drawing"];
//            if ([activeDrawing isKindOfClass:[NSDictionary class]]) {
//                NSNumber *isActive = activeDrawing[@"is_active"];
//                if (isActive && [isActive boolValue]) {
//                    drawingPath = activeDrawing[@"path"];
//                    drawingName = activeDrawing[@"name"];
//                }
//            }
//        }
//    }
    
    // 模拟网络请求延时 1.5 秒
//    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
//        NSMutableString *resultMsg = [NSMutableString stringWithString:@"[模拟服务器上报成功]\n"];
//        [resultMsg appendString:@"1. 崩溃元数据及面包屑 JSON 上传成功。\n"];
//        
//        if (drawingPath && [drawingPath length] > 0) {
//            if (uploadDrawing) {
//                // 用户授权同意，模拟上传图纸本体
//                [resultMsg appendFormat:@"2. [用户授权已勾选] 图纸文件已成功上报: %@\n", drawingName];
//            } else {
//                // 用户未授权，严格遵照合规红线，只传元数据，不传图纸文件本体
//                [resultMsg appendFormat:@"2. [安全合规限制] 拒绝自动上传图纸本体文件。仅上报该图纸的哈希指纹以作比对。\n"];
//            }
//        } else {
//            [resultMsg appendString:@"2. 本次崩溃无关联图纸文件。\n"];
//        }
//        
//        if (completion) {
//            completion(YES, resultMsg);
//        }
//    });
}

- (BOOL)deleteCrashLogAtPath:(NSString *)path {
    NSError *error = nil;
    BOOL res = [[NSFileManager defaultManager] removeItemAtPath:path error:&error];
    if (error) {
        NSLog(@"[ZWMobileIOSGuard] 删除崩溃文件失败: %@，%@", path, error);
    }
    return res;
}

@end
