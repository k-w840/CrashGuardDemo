#include "ZWMobileGuardPOSIX.h"
#include "ZWMobileGuard.h"
#include "ZWMobileGuardBacktrace.h"
#include "ZWMobileGuardInternal.h"
#include "ZWMobileGuardRawReport.h"
#include "ZWMobileGuardState.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 只处理这五个即可，其余信号为系统层面不能捕获，或者是用户主动操作的正常信号
static const int kZwMobileGuardFatalSignals[] = {SIGSEGV, SIGABRT, SIGILL, SIGFPE, SIGBUS};

// 恢复原有 signal handler，避免崩溃处理过程中再次崩溃导致错误
static void restoreOriginalSignalHandlers(void) {
    for (int i = 0; i < 5; ++i) {
        int sig = kZwMobileGuardFatalSignals[i];
        sigaction(sig, &g_sdkConfig.originalSigactions[sig], nullptr);
    }
}

// POSIX 崩溃捕获拦截处理
static void zwMobileGuardSignalHandler(int sig, siginfo_t *info, void *context) {
    ZWMobileCrashHandlingResult handlingResult = zwMobileGuardEnterFatalCrashHandling();
    // 先把自己卸载掉，再继续处理
    restoreOriginalSignalHandlers();
    if (handlingResult != ZWMobileCrashHandlingResultFirstCrash) {
        raise(sig);
        return;
    }

    // 获取堆栈
    void *crashFrames[MAX_STACK_FRAMES];
    // signal 场景优先使用上下文中的寄存器现场展开堆栈
    int crashFramesCount = zwMobileGuardCaptureSignalBacktrace(context, crashFrames, MAX_STACK_FRAMES);
    
    // signal 场景直接落盘格式化的 JSON 数据
    zwMobileGuardWriteReportInternal(ZWRawCrashTypeSignal, sig, info, context, nullptr, nullptr, crashFrames, crashFramesCount);
    
    struct sigaction originalAction = g_sdkConfig.originalSigactions[sig];

    // SIG_DFL(系统默认处理) 和 SIG_IGN(忽略信号) 原本没自定义处理，直接触发一次信号
    if (originalAction.sa_handler == SIG_DFL || originalAction.sa_handler == SIG_IGN) {
        raise(sig);
    } else if (originalAction.sa_handler != SIG_IGN) {
        // 有自定义处理，根据参数类型 返回自定义处理
        if (originalAction.sa_flags & SA_SIGINFO) {
            originalAction.sa_sigaction(sig, info, context);
        } else {
            originalAction.sa_handler(sig);
        }
    }
}

#pragma mark - 线程备用信号栈
static pthread_key_t g_alternateStackKey;
static pthread_once_t g_alternateStackKeyOnce = PTHREAD_ONCE_INIT;

// 线程退出时，释放备用栈
static void destroyAlternateStack(void *ptr) {
    if (ptr) {
        stack_t altStack;
        altStack.ss_sp = nullptr;
        altStack.ss_size = 0;
        altStack.ss_flags = SS_DISABLE;
        sigaltstack(&altStack, nullptr);
        free(ptr);
    }
}

// 备用栈初始化回掉函数
static void alternateStackInitFunc() {
    // 线程安全执行析构
    pthread_key_create(&g_alternateStackKey, destroyAlternateStack);
}

#pragma mark - 对外 API 实现
/// posix注册
void zwMobilePosixInit(void) {
    // 注册 POSIX 信号处理器
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    // 设置信号崩溃时处理函数
    sa.sa_sigaction = zwMobileGuardSignalHandler;
    // 使用三参数崩溃信号函数，以便能获取物理地址
    // 使用备用信号栈防栈溢出
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    
    for (int i = 0; i < 5; ++i) {
        int sig = kZwMobileGuardFatalSignals[i];
        // 设置崩溃信号自定义处理器，并存储原处理
        sigaction(sig, &sa, &g_sdkConfig.originalSigactions[sig]);
    }
    
    // 默认为主线程/当前线程注册备用信号栈
    zwMobileGuardRegisterThreadSignalStack();
}

// 注册备用信号栈
extern "C" void zwMobileGuardRegisterThreadSignalStack(void) {
    // 线程安全防护 && 只执行一次
    pthread_once(&g_alternateStackKeyOnce, alternateStackInitFunc);
    // g_alternateStackKey 保证线程安全，每个线程只有一个备用栈
    if (pthread_getspecific(g_alternateStackKey)) {
        return;
    }
    
    // 备用栈大小为默认大小
    void *stackBase = malloc(SIGSTKSZ);
    if (!stackBase) {
        return;
    }
    
    stack_t alternateSingleStack;
    // 配置备用栈信息
    alternateSingleStack.ss_sp = stackBase;
    alternateSingleStack.ss_size = SIGSTKSZ;
    alternateSingleStack.ss_flags = 0;
    // 创建备用信号栈，当线程因栈溢出崩溃时，能捕获崩溃
    if (sigaltstack(&alternateSingleStack, nullptr) == 0) {
        pthread_setspecific(g_alternateStackKey, stackBase);
    } else {
        free(stackBase);
    }
}
