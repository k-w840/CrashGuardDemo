#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <mach-o/dyld.h>
#include <pthread.h>
#include <stdlib.h>
#include <typeinfo>
#include "fishhook.h"

extern "C" void zwMobileGuardRecordThrowState(void* thrown_exception, std::type_info* tinfo);

typedef void (*CxaThrowType)(void*, std::type_info*, void (*)(void*));
typedef void (*CxaRethrowType)();

struct OriginalCxaThrow {
    const void* imageBase;
    CxaThrowType function;
};

static pthread_mutex_t g_cxaHookMutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_cxaThrowRebindingInstalled = false;
static thread_local void* g_lastRecordedThrownException = nullptr;
// 原始 throw 函数缓存
static OriginalCxaThrow* g_originalCxaThrows = nullptr;
// 缓存的 throw 函数的真实数量
static size_t g_originalCxaThrowCount = 0;
// 缓存原始 throw 函数的数组容量
static size_t g_originalCxaThrowCapacity = 0;

// 查找下一个 __cxa_throw 方法
static CxaThrowType lookupOriginalCxaThrow() {
    return (CxaThrowType)dlsym(RTLD_NEXT, "__cxa_throw");
}

// 查找下一个 rethrow 方法
static CxaRethrowType lookupOriginalCxaRethrow() {
    return (CxaRethrowType)dlsym(RTLD_NEXT, "__cxa_rethrow");
}

// 记录原始 __cxa_throw 函数
static void rememberOriginalCxaThrow(const void* binaryImageBase, CxaThrowType function) {
    if (binaryImageBase == nullptr || function == nullptr) {
        return;
    }
    // 避免重复存入，一个 binaryImageBase 对应一个方法
    for (size_t i = 0; i < g_originalCxaThrowCount; ++i) {
        if (g_originalCxaThrows[i].imageBase == binaryImageBase) {
            g_originalCxaThrows[i].function = function;
            return;
        }
    }

    if (g_originalCxaThrowCount == g_originalCxaThrowCapacity) {
        // 初始容量32，后面一次扩容一倍
        // 崩溃时尽量不使用vector降低依赖 C++ runtime 用底层函数
        size_t newCapacity = g_originalCxaThrowCapacity == 0 ? 32 : g_originalCxaThrowCapacity * 2;
        OriginalCxaThrow* newItems = (OriginalCxaThrow*)realloc(g_originalCxaThrows, newCapacity * sizeof(OriginalCxaThrow));
        if (newItems == nullptr) {
            return;
        }
        g_originalCxaThrows = newItems;
        g_originalCxaThrowCapacity = newCapacity;
    }

    g_originalCxaThrows[g_originalCxaThrowCount++] = {binaryImageBase, function};
}

// 根据 imageBase 查找原始 __cxa_throw 函数
static CxaThrowType originalCxaThrowForImage(const void* imageBase) {
    for (size_t i = 0; i < g_originalCxaThrowCount; ++i) {
        if (g_originalCxaThrows[i].imageBase == imageBase) {
            return g_originalCxaThrows[i].function;
        }
    }
    return lookupOriginalCxaThrow();
}

// 同时绑定了动态和静态两种hook，以提高成功率
// 但存在动态符号重绑的是静态替换的，导致重复执行，这里增加过滤
static void recordThrowStateIfNeeded(void* thrown_exception, std::type_info* tinfo) {
    if (thrown_exception == g_lastRecordedThrownException) {
        return;
    }
    g_lastRecordedThrownException = thrown_exception;
    zwMobileGuardRecordThrowState(thrown_exception, tinfo);
}

#pragma mark - 静态链接时自动拦截替换 __cxa_throw
// 用 weak 避免和其他库符号冲突编译报错
extern "C" __attribute__((weak, visibility("default")))
void __cxa_throw(void *thrown_exception, std::type_info *tinfo, void (*dest)(void *)) {
    // 此时栈还没有展开，在此处获取调用栈
    recordThrowStateIfNeeded(thrown_exception, tinfo);
    
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

#pragma mark - 动态链接HOOK,符号重绑加载的动态库的__cxa_throw
static void zwMobileGuardCxaThrowDecorator(void* thrown_exception,
                                           std::type_info* tinfo,
                                           void (*dest)(void*)) {
    // 获取堆栈
    recordThrowStateIfNeeded(thrown_exception, tinfo);
    
    // 取当前调用栈前两层，frames[1] 是被重绑后的 __cxa_throw 调用点，以此获取原处理函数
    CxaThrowType original = nullptr;
    void* frames[2];
    int frameCount = backtrace(frames, 2);
    if (frameCount >= 2) {
        Dl_info callerInfo;
        if (dladdr(frames[1], &callerInfo) && callerInfo.dli_fbase != nullptr) {
            original = originalCxaThrowForImage(callerInfo.dli_fbase);
        }
    }
    if (original == nullptr) {
        original = lookupOriginalCxaThrow();
    }
    original(thrown_exception, tinfo, dest);
    __builtin_unreachable();
}

static void rebindCxaThrowForImage(const struct mach_header* header, intptr_t slide) {
    Dl_info info;
    // 获取不到所属的二进制模块信息,无法继续动态替换
    if (header == nullptr || dladdr(header, &info) == 0) {
        return;
    }
    // 使用finish hook 替换 __cxa_throw
    CxaThrowType original = nullptr;
    struct rebinding rebindings[] = {
        {"__cxa_throw", (void*)zwMobileGuardCxaThrowDecorator, (void**)&original}
    };
    rebind_symbols_image((void*)header, slide, rebindings, 1);
    
    // 存储原__cxa_throw函数
    if (original != nullptr && original != (CxaThrowType)zwMobileGuardCxaThrowDecorator) {
        rememberOriginalCxaThrow(info.dli_fbase, original);
    }
}

extern "C" void zwMobileGuardInstallCxaThrowHook(void) {
    pthread_mutex_lock(&g_cxaHookMutex);
    if (!g_cxaThrowRebindingInstalled) {
        g_cxaThrowRebindingInstalled = true;
        //  动态符号重绑，并记录替换的原函数
        uint32_t imageCount = _dyld_image_count();
        for (uint32_t i = 0; i < imageCount; ++i) {
            rebindCxaThrowForImage(_dyld_get_image_header(i), _dyld_get_image_vmaddr_slide(i));
        }
        // 不止对当前挂载的处理，后续挂载的也处理
        _dyld_register_func_for_add_image(rebindCxaThrowForImage);
    }
    pthread_mutex_unlock(&g_cxaHookMutex);
}
