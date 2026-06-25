#include <cxxabi.h>
#include <dlfcn.h>
#include <typeinfo>

// Android 侧与 iOS 侧保持一致：在 throw 发生的第一时间记录原始堆栈，
// 后续即使异常跨越 JNI 边界或最终进入 terminate，也能尽量保留最初现场。
extern "C" void ZWCADGuard_RecordThrowState(void* thrown_exception, std::type_info* tinfo);

extern "C" __attribute__((visibility("default")))
void __cxa_throw(void* thrown_exception,
                 std::type_info* tinfo,
                 void (*dest)(void*)) {
    ZWCADGuard_RecordThrowState(thrown_exception, tinfo);

    typedef void (*CxaThrowType)(void*, std::type_info*, void (*)(void*));
    static CxaThrowType original_cxa_throw = nullptr;
    if (original_cxa_throw == nullptr) {
        original_cxa_throw = (CxaThrowType)dlsym(RTLD_NEXT, "__cxa_throw");
    }

    original_cxa_throw(thrown_exception, tinfo, dest);
    __builtin_unreachable();
}

extern "C" __attribute__((visibility("default")))
void __cxa_rethrow() {
    typedef void (*CxaRethrowType)();
    static CxaRethrowType original_cxa_rethrow = nullptr;
    if (original_cxa_rethrow == nullptr) {
        original_cxa_rethrow = (CxaRethrowType)dlsym(RTLD_NEXT, "__cxa_rethrow");
    }

    original_cxa_rethrow();
    __builtin_unreachable();
}
