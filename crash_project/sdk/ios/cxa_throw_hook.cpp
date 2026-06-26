#include <cxxabi.h>
#include <dlfcn.h>
#include <pthread.h>
#include <typeinfo>
#include "fishhook.h"

extern "C" void zwMobileGuardRecordThrowState(void* thrown_exception, std::type_info* tinfo);

typedef void (*CxaThrowType)(void*, std::type_info*, void (*)(void*));
typedef void (*CxaRethrowType)();

static pthread_mutex_t g_cxaHookMutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_cxaThrowRebindingInstalled = false;
static thread_local bool g_skipNextThrowRecord = false;
static CxaThrowType g_originalCxaThrow = nullptr;

// 查找原throw方法
static CxaThrowType lookupOriginalCxaThrow() {
    return (CxaThrowType)dlsym(RTLD_NEXT, "__cxa_throw");
}

// 查找原rethrow方法
static CxaRethrowType lookupOriginalCxaRethrow() {
    return (CxaRethrowType)dlsym(RTLD_NEXT, "__cxa_rethrow");
}

#pragma mark - 静态链接时自动拦截 __cxa_throw
// 用 weak 避免和其他库符号冲突编译报错
extern "C" __attribute__((weak, visibility("default")))
void __cxa_throw(void *thrown_exception, std::type_info *tinfo, void (*dest)(void *)) {
    // 如果在动态替换中已记录，这里不再记录只转发
    if (!g_skipNextThrowRecord) {
        // 此时栈还没有展开，在此处获取调用栈
        zwMobileGuardRecordThrowState(thrown_exception, tinfo);
    }
    g_skipNextThrowRecord = false;
    
    // 获取系统的原始 __cxa_throw
    static CxaThrowType originalCxaThrow = lookupOriginalCxaThrow();
    
    // 转发原始异常
    originalCxaThrow(thrown_exception, tinfo, dest);
    // 因为 __cxa_throw 是 noreturn，标记为不可达
    __builtin_unreachable();
}

// 异常重抛，预留扩展，该函数和 __cxa_throw关系紧密，一起重写，避免在安卓等静态链接时符号拉取冲突报错
extern "C" __attribute__((weak, visibility("default")))
void __cxa_rethrow() {
    static CxaRethrowType original_cxa_rethrow = lookupOriginalCxaRethrow();
    
    // 转发重抛
    original_cxa_rethrow();
    __builtin_unreachable();
}

#pragma mark - 动态链接HOOK,拦截并替换加载的动态库的__cxa_throw
static void zwMobileGuardCxaThrowDecorator(void* thrown_exception,
                                           std::type_info* tinfo,
                                           void (*dest)(void*)) {
    // 获取堆栈
    zwMobileGuardRecordThrowState(thrown_exception, tinfo);
    
    CxaThrowType original = g_originalCxaThrow;
    if (original == nullptr) {
        original = lookupOriginalCxaThrow();
    }
    // 如果动态替换的throw是静态替换后的方法，静态替换只转发，不记录carsh
    if (original == (CxaThrowType)__cxa_throw) {
        g_skipNextThrowRecord = true;
    }
    original(thrown_exception, tinfo, dest);
    __builtin_unreachable();
}

extern "C" void zwMobileGuardInstallCxaThrowHook(void) {
    pthread_mutex_lock(&g_cxaHookMutex);
    if (!g_cxaThrowRebindingInstalled) {
        g_cxaThrowRebindingInstalled = true;
        
        struct rebinding rebindings[] = {
            {"__cxa_throw", (void*)zwMobileGuardCxaThrowDecorator, (void**)&g_originalCxaThrow}
        };
        rebind_symbols(rebindings, 1);
    }
    pthread_mutex_unlock(&g_cxaHookMutex);
}
