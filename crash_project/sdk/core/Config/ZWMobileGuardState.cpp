#include "ZWMobileGuardState.h"

static volatile sig_atomic_t g_fatalCrashHandlingState = 0;

ZWMobileCrashHandlingResult zwMobileGuardEnterFatalCrashHandling(void) {
    // 原子性修改崩溃状态，处理中再次崩溃，外部不继续处理
    if (__sync_bool_compare_and_swap(&g_fatalCrashHandlingState, 0, 1)) {
        return ZWMobileCrashHandlingResultFirstCrash;
    }
    return ZWMobileCrashHandlingResultRecrash;
}

// 使用 thread_local 保存每个线程的最新 throw 堆栈
thread_local ThreadExceptionInfo g_threadException = {{0}, 0, {0}, false};

// 操作路径缓存
BreadCrumbState g_breadCrumbs = {{{{0}}, 0, 0}, PTHREAD_MUTEX_INITIALIZER};
// 操作路径快照
BreadCrumbStore g_breadCrumbSnapshot = {{{0}}, 0, 0};

// 图纸信息缓存
ActiveDrawingState g_activeDrawing = {{{0}, {0}, 0, {0}, {0}, {0}, {0}, false}, PTHREAD_MUTEX_INITIALIZER};
// 图纸信息快照
ActiveDrawingInfo g_activeDrawingSnapshot = {{0}, {0}, 0, {0}, {0}, {0}, {0}, false};

// SDK全局配置
SDKConfig g_sdkConfig = {{0}, nullptr, {{0}}, false};
