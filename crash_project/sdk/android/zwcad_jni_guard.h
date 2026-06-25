#ifndef ZWCAD_JNI_GUARD_H
#define ZWCAD_JNI_GUARD_H

#include <jni.h>
#include <exception>

static inline void ZWCADGuard_ThrowJavaRuntimeException(JNIEnv* env, const char* message) {
    if (env == nullptr) {
        return;
    }

    jclass ex_class = env->FindClass("java/lang/RuntimeException");
    if (ex_class != nullptr) {
        env->ThrowNew(ex_class, message ? message : "Unknown C++ exception crossed JNI boundary");
    }
}

/*
 * 正式方案下，Android 侧建议统一通过 JNI 边界包装做异常防火墙，而不是依赖
 * 全局 RegisterNatives Hook。业务 native 导出函数可以直接复用下面两组宏。
 */
#define ZWCAD_JNI_GUARD_BEGIN try {

#define ZWCAD_JNI_GUARD_END_VOID(env) \
    } catch (const std::exception& e) { \
        ZWCADGuard_ThrowJavaRuntimeException(env, e.what()); \
        return; \
    } catch (...) { \
        ZWCADGuard_ThrowJavaRuntimeException(env, "Unknown C++ exception crossed JNI boundary"); \
        return; \
    }

#define ZWCAD_JNI_GUARD_END_RETURN(env, fallback_value) \
    } catch (const std::exception& e) { \
        ZWCADGuard_ThrowJavaRuntimeException(env, e.what()); \
        return fallback_value; \
    } catch (...) { \
        ZWCADGuard_ThrowJavaRuntimeException(env, "Unknown C++ exception crossed JNI boundary"); \
        return fallback_value; \
    }

#endif // ZWCAD_JNI_GUARD_H
