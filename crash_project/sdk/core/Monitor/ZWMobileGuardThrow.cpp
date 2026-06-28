#include "ZWMobileGuardThrow.h"
#include "ZWMobileGuard.h"
#include "ZWMobileGuardBacktrace.h"
#include "ZWMobileGuardInternal.h"
#include "ZWMobileGuardReport.h"
#include "ZWMobileGuardState.h"
#include <cxxabi.h>
#include <exception>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <typeinfo>

// iOS 在初始化时显式安装 __cxa_throw Mach-O rebinding
#ifdef __APPLE__
extern "C" void zwMobileGuardInstallCxaThrowHook(void);
#endif

// 在 hook 中获取堆栈
extern "C" void zwMobileGuardRecordThrowState(void *thrown_exception, std::type_info *tinfo) {
    // 获取当前堆栈
    g_threadException.framesCount = zwMobileGuardCaptureBacktrace(g_threadException.backtraceBuffer, MAX_STACK_FRAMES);
    g_threadException.hasException = true;
    if (tinfo && tinfo->name()) {
        // 符号修饰名转为函数名
        char *demangled = zwMobileGuardDemangle(tinfo->name());
        // 转成功用实际函数名，没转成功用原始名
        if (demangled) {
            strncpy(g_threadException.exceptionType, demangled, sizeof(g_threadException.exceptionType) - 1);
            free(demangled);
        } else {
            strncpy(g_threadException.exceptionType, tinfo->name(), sizeof(g_threadException.exceptionType) - 1);
        }
    } else {
        strcpy(g_threadException.exceptionType, "Unknown Type");
    }
}

#pragma mark - Terminate崩溃捕获拦截处理
// Terminate 崩溃捕获拦截处理
static void zwMobileGuardTerminateHandler(void) {
    ZWMobileCrashHandlingResult handlingResult = zwMobileGuardEnterFatalCrashHandling();
    if (handlingResult == ZWMobileCrashHandlingResultRecrash) {
        _Exit(1);
    }

    void *crash_frames[MAX_STACK_FRAMES];
    int crash_frames_count = zwMobileGuardCaptureBacktrace(crash_frames, MAX_STACK_FRAMES);
    
    char reasonBuf[256] = {0};
    const char *exceptionTypeName = "Unknown C++ Exception";
    
    // 获取异常类型
    std::type_info *tinfo = __cxxabiv1::__cxa_current_exception_type();
    if (tinfo && tinfo->name()) {
        char *demangled = zwMobileGuardDemangle(tinfo->name());
        if (demangled) {
            strncpy(reasonBuf, demangled, sizeof(reasonBuf) - 1);
            free(demangled);
        } else {
            strncpy(reasonBuf, tinfo->name(), sizeof(reasonBuf) - 1);
        }
        exceptionTypeName = reasonBuf;
    }
    
    // 重抛异常以截获 what() 信息
    char message_buf[512] = {0};
    try {
        if (std::current_exception()) {
            std::rethrow_exception(std::current_exception());
        }
    } catch (const std::exception &e) {
        strncpy(message_buf, e.what(), sizeof(message_buf) - 1);
    } catch (const char *str) {
        strncpy(message_buf, str, sizeof(message_buf) - 1);
    } catch (...) {
        strcpy(message_buf, "Non-standard C++ exception thrown");
    }
    
    // 拼接最终的原因
    char final_reason[1024];
    snprintf(final_reason, sizeof(final_reason), "Type: %s, Message: %s", exceptionTypeName, strlen(message_buf) > 0 ? message_buf : "N/A");
    
    // 落盘写入
    dumpToFile("C++ Uncaught Exception (std::terminate)", final_reason, crash_frames, crash_frames_count);
    
    // 转发给原本的 terminate 处理器
    if (g_sdkConfig.originalTerminateHandler) {
        g_sdkConfig.originalTerminateHandler();
    } else {
        abort();
    }
}

#pragma mark - 对外 API 实现
/// throw/terminate注册
void zwMobileThrowInit(void) {
    // 设置 std::terminate 拦截器
    // 函数 std::set_terminate 会将传入的设置为新处理器，并返回旧处理器
    g_sdkConfig.originalTerminateHandler = std::set_terminate(zwMobileGuardTerminateHandler);

    // apple 时显式安装 __cxa_throw
#ifdef __APPLE__
    zwMobileGuardInstallCxaThrowHook();
#endif
}
