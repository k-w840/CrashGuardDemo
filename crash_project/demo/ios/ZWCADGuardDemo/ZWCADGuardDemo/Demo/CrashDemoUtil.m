//
//  CrashDemoUtil.m
//  ZWCADGuardDemo
//
//  Created by kwb on 2026/6/21.
//

#import "CrashDemoUtil.h"

NS_ASSUME_NONNULL_BEGIN

@implementation CrashDemoUtil

NSString *g_currentDrawingStatus = @"图纸状态: 未打开任何图纸\n(崩溃日志将不携带图纸哈希)";

+ (NSArray *)demoTestData {
    return @[[self drawingCrashData],
             [self cCrashData],
             [self crossLingualCrashData],
             [self ocCrashData],
             [self posixCrashData]];
}

+ (NSDictionary *)drawingCrashData {
    return @{DataTitleID:@"活跃图纸状态与埋点操作",
             DataValuesID:@[@"模拟打开图纸",@"模拟双指缩放",@"模拟绘制圆形",@"模拟绘制线条",@"模拟关闭图纸"],
             DataTypeId:@[@(ActionTypeOpenDWG),@(ActionTypeDoubleFinger),@(ActionTypeDrawCircle),@(ActionTypeDrawLine),@(ActionTypeCloseDWG)]};
}

+ (NSDictionary *)cCrashData {
    return @{DataTitleID:@"C++ 异常场景验证",
                 DataValuesID:@[@"未捕获 std::runtime_error",@"未捕获自定义异常类",@"抛出基本数据类型 (throw 42)",@"析构函数双重抛出 (std::terminate)"],
             DataTypeId:@[@(ActionTypeUncatchRuntime),@(ActionTypeUncatchCustom),@(ActionTypeThrowBaseData),@(ActionTypeDestructor)]};
}

+ (NSDictionary *)crossLingualCrashData {
    return @{DataTitleID:@"跨语言异常安全防护防火墙",
                 DataValuesID:@[@"触发 C++ 异常并被 OC 防火墙安全拦截",@"模拟 JNI 跨语言异常安全转换"],
             DataTypeId:@[@(ActionTypeFireWallCatch),@(ActionTypeJNIExceptionSafeConv)]};
}

+ (NSDictionary *)ocCrashData {
    return @{DataTitleID:@"Objective-C 异常验证",
                 DataValuesID:@[@"数组越界 (NSException)",@"未实现方法 (Unrecognized Selector)"],
             DataTypeId:@[@(ActionTypeCrossing),@(ActionTypeUnrecognizedSelector)]};
}

+ (NSDictionary *)posixCrashData {
    return @{DataTitleID:@"POSIX 操作系统信号崩溃兜底验证",
                 DataValuesID:@[@"SIGSEGV (野指针空指针写入)",@"SIGABRT (调用 abort)",@"SIGILL (非法指令 __builtin_trap)",@"SIGFPE (除零异常/算术错误)",@"线程栈溢出崩溃 (死循环深度递归)"],
             DataTypeId:@[@(ActionTypeWildOrNullPointer),@(ActionTypeSIGABRT),@(ActionTypeSIGILL),@(ActionTypeSIGFPE),@(ActionTypeStackOverflow)]};
}
@end

NS_ASSUME_NONNULL_END
