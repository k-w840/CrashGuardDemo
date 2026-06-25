#include <jni.h>
#include <stdexcept>

#include "zwcad_jni_guard.h"

extern "C" JNIEXPORT void JNICALL
Java_com_zwsoft_zwcadguard_demo_NativeBridge_triggerUnhandledCppException(JNIEnv* env, jclass clazz) {
    (void)env;
    (void)clazz;
    throw std::runtime_error("Unhandled C++ exception from Android demo");
}

extern "C" JNIEXPORT void JNICALL
Java_com_zwsoft_zwcadguard_demo_NativeBridge_triggerSignalCrash(JNIEnv* env, jclass clazz) {
    (void)env;
    (void)clazz;
    volatile int* crash_ptr = nullptr;
    *crash_ptr = 7;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_zwsoft_zwcadguard_demo_NativeBridge_callGuardedCppException(JNIEnv* env, jclass clazz) {
    (void)clazz;
    ZWCAD_JNI_GUARD_BEGIN

    throw std::runtime_error("Guarded JNI C++ exception from Android demo");

    return 0;
    ZWCAD_JNI_GUARD_END_RETURN(env, -1)
}
