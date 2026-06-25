#include <cxxabi.h>
#include <dlfcn.h>
#include <typeinfo>

// 声明 core 层记录抛出状态的 C 接口
extern "C" void zwMobileGuardRecordThrowState(void* thrown_exception, std::type_info* tinfo);

// 强行覆盖 libc++abi 的 weak 符号 __cxa_throw，从而在抛出异常瞬间截获现场
extern "C" __attribute__((visibility("default")))
void __cxa_throw(void *thrown_exception,
                 std::type_info *tinfo,
                 void (*dest)(void *)) {
    // 此时栈还没有展开，在此处抓取调用栈
    zwMobileGuardRecordThrowState(thrown_exception, tinfo);

    // 动态寻找系统的原始 __cxa_throw
    typedef void (*CxaThrowType)(void *, std::type_info *, void (*)(void *));
    static CxaThrowType original_cxa_throw = nullptr;
    if (original_cxa_throw == nullptr) {
        original_cxa_throw = (CxaThrowType)dlsym(RTLD_NEXT, "__cxa_throw");
    }
    
    // 转发异常
    original_cxa_throw(thrown_exception, tinfo, dest);
    __builtin_unreachable(); // 标记为不可达，因为 __cxa_throw 是 noreturn 属性
}

// 强行覆盖 libc++abi 的 weak 符号 __cxa_rethrow，以拦截异常重抛 (throw;)
extern "C" __attribute__((visibility("default")))
void __cxa_rethrow() {
    typedef void (*CxaRethrowType)();
    static CxaRethrowType original_cxa_rethrow = nullptr;
    if (original_cxa_rethrow == nullptr) {
        original_cxa_rethrow = (CxaRethrowType)dlsym(RTLD_NEXT, "__cxa_rethrow");
    }
    
    // 转发重抛
    original_cxa_rethrow();
    __builtin_unreachable(); // noreturn 属性
}
