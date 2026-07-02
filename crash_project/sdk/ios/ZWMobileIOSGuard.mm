#import "ZWMobileIOSGuard.h"
#include "ZWMobileGuard.h"
#import <UIKit/UIKit.h>
// 中望接入时直接使用现有工具类
//#import "NSString+ZwDateString.h"
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

- (void)uploadCrashReport {
    NSError *error = nil;
    NSArray<NSString *> *files = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:self.logDir error:&error];
    if (error) {
        NSLog(@"[ZWMobileIOSGuard] 获取崩溃存储路径失败:%@", error);
        return;
    }
    __weak __typeof(self) weakSelf = self;
    for (NSString *file in files) {
        // 过滤以 crash_ 开头并以 .json 结尾的崩溃报告文件
        if ([file hasPrefix:@"crash_"] && [file hasSuffix:@".json"]) {
            // carsh 报告 文件名中时间戳转为固定格式时间
            NSArray *pathArr = [file componentsSeparatedByString:@"_"];
            if (pathArr.count != 3) {
                continue;
            }
            // 中望接入直接使用现有工具类
//            NSString *carshTime = [NSString dateStringWithTimestamp:[pathArr objectAtIndex:1] format:@"yyyy-MM-dd-HHmmss"];
            NSString *carshTime = [[self class] dateStringWithTimestamp:[pathArr objectAtIndex:1] format:@"yyyy-MM-dd-HHmmss"];
            NSString *uploadFileName = [NSString stringWithFormat:@"%@_%@_%@", [pathArr objectAtIndex:0], carshTime, [pathArr objectAtIndex:2]];
            NSString *filePath = [self.logDir stringByAppendingPathComponent:file];
            [[ZWApiService shareInstance] uploadCrashReport:[NSData dataWithContentsOfFile:filePath] fileName:uploadFileName callBack:^(id  _Nullable response, NSError * _Nullable error) {
                // 上传成功后，清空本地数据
                if (!error) {
                    [weakSelf deleteCrashLogAtPath:filePath];
                }
            }];
        }
    }
}

- (BOOL)deleteCrashLogAtPath:(NSString *)path {
    NSError *error = nil;
    BOOL res = [[NSFileManager defaultManager] removeItemAtPath:path error:&error];
    if (error) {
        NSLog(@"[ZWMobileIOSGuard] 删除崩溃文件失败: %@，%@", path, error);
    }
    return res;
}

// 中望接入该部分代码删除，用现有工具类 begig
+ (NSString *)dateStringWithTimestamp:(NSString *)timestampStr format:(NSString *)format {
    if (timestampStr.length <= 0 || format.length <= 0) {
        return @"";
    }
    NSTimeInterval timeInterval = [timestampStr doubleValue];
    // √ä√ò¬¥√Å√ü√≠√Å‚à´√ü√ã¬∂√Ö√à√¥¬ß1000
    if (timestampStr.length == 13) {
        timeInterval = timeInterval / 1000.0;
    }
    NSDate *date = [NSDate dateWithTimeIntervalSince1970:timeInterval];
    return [NSString getTimeStringWithFormat:format date:date];
}

+ (NSString *)getTimeStringWithFormat:(NSString *)format date:(NSDate *)date {
    NSDateFormatter *dateFormat = [[NSDateFormatter alloc] init];
    [dateFormat setDateFormat:format];
    return [dateFormat stringFromDate:date];
}
// 中望接入该部分代码删除，用现有工具类 end
@end
