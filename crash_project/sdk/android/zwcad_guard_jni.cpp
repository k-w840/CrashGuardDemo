#include "zwcad_guard_jni.h"
#include "zwcad_jni_guard.h"
#include "dwg_guard.h"
#include <string.h>
#include <stdio.h>
 
// Android 正式方案不再依赖全局 RegisterNatives Hook，而是要求在 JNI 边界
// 统一使用 try/catch 防火墙。当前文件中的 SDK 对外 JNI 接口本身也遵循同一规则。

JNIEXPORT jint JNICALL Java_com_zwsoft_zwcadguard_ZWCADGuard_nativeInit(JNIEnv *env, jclass clazz, jstring logDir) {
    (void)clazz;
    ZWCAD_JNI_GUARD_BEGIN

    const char* path_str = env->GetStringUTFChars(logDir, nullptr);
    int res = ZWCADGuard_Init(path_str);
    env->ReleaseStringUTFChars(logDir, path_str);
    return res;

    ZWCAD_JNI_GUARD_END_RETURN(env, -1)
}

JNIEXPORT void JNICALL Java_com_zwsoft_zwcadguard_ZWCADGuard_nativeSetActiveDrawing
  (JNIEnv *env, jclass clazz, jstring name, jstring path, jlong size, jstring hash) {
    (void)clazz;
    ZWCAD_JNI_GUARD_BEGIN

    const char* name_str = name ? env->GetStringUTFChars(name, nullptr) : nullptr;
    const char* path_str = path ? env->GetStringUTFChars(path, nullptr) : nullptr;
    const char* hash_str = hash ? env->GetStringUTFChars(hash, nullptr) : nullptr;

    ZWCADGuard_SetActiveDrawing(name_str, path_str, (long)size, hash_str);

    if (name) env->ReleaseStringUTFChars(name, name_str);
    if (path) env->ReleaseStringUTFChars(path, path_str);
    if (hash) env->ReleaseStringUTFChars(hash, hash_str);

    ZWCAD_JNI_GUARD_END_VOID(env)
}

JNIEXPORT void JNICALL Java_com_zwsoft_zwcadguard_ZWCADGuard_nativeSetActiveDrawingContext
  (JNIEnv *env, jclass clazz, jstring name, jstring path, jlong size, jstring hash, jstring fileId, jstring projectId, jstring projectName) {
    (void)clazz;
    ZWCAD_JNI_GUARD_BEGIN

    const char* name_str = name ? env->GetStringUTFChars(name, nullptr) : nullptr;
    const char* path_str = path ? env->GetStringUTFChars(path, nullptr) : nullptr;
    const char* hash_str = hash ? env->GetStringUTFChars(hash, nullptr) : nullptr;
    const char* file_id_str = fileId ? env->GetStringUTFChars(fileId, nullptr) : nullptr;
    const char* project_id_str = projectId ? env->GetStringUTFChars(projectId, nullptr) : nullptr;
    const char* project_name_str = projectName ? env->GetStringUTFChars(projectName, nullptr) : nullptr;

    ZWCADGuard_SetActiveDrawingContext(name_str,
                                       path_str,
                                       (long)size,
                                       hash_str,
                                       file_id_str,
                                       project_id_str,
                                       project_name_str);

    if (name) env->ReleaseStringUTFChars(name, name_str);
    if (path) env->ReleaseStringUTFChars(path, path_str);
    if (hash) env->ReleaseStringUTFChars(hash, hash_str);
    if (fileId) env->ReleaseStringUTFChars(fileId, file_id_str);
    if (projectId) env->ReleaseStringUTFChars(projectId, project_id_str);
    if (projectName) env->ReleaseStringUTFChars(projectName, project_name_str);

    ZWCAD_JNI_GUARD_END_VOID(env)
}

JNIEXPORT void JNICALL Java_com_zwsoft_zwcadguard_ZWCADGuard_nativeClearActiveDrawing(JNIEnv *env, jclass clazz) {
    (void)clazz;
    ZWCAD_JNI_GUARD_BEGIN
    ZWCADGuard_ClearActiveDrawing();
    ZWCAD_JNI_GUARD_END_VOID(env)
}

JNIEXPORT void JNICALL Java_com_zwsoft_zwcadguard_ZWCADGuard_nativeAddBreadcrumb
  (JNIEnv *env, jclass clazz, jstring category, jstring action, jstring details) {
    (void)clazz;
    ZWCAD_JNI_GUARD_BEGIN

    const char* cat_str = category ? env->GetStringUTFChars(category, nullptr) : nullptr;
    const char* act_str = action ? env->GetStringUTFChars(action, nullptr) : nullptr;
    const char* det_str = details ? env->GetStringUTFChars(details, nullptr) : nullptr;

    ZWCADGuard_AddBreadcrumb(cat_str, act_str, det_str);

    if (category) env->ReleaseStringUTFChars(category, cat_str);
    if (action) env->ReleaseStringUTFChars(action, act_str);
    if (details) env->ReleaseStringUTFChars(details, det_str);

    ZWCAD_JNI_GUARD_END_VOID(env)
}

JNIEXPORT void JNICALL Java_com_zwsoft_zwcadguard_ZWCADGuard_nativeSimulateCrash(JNIEnv *env, jclass clazz, jstring message) {
    (void)clazz;
    ZWCAD_JNI_GUARD_BEGIN

    const char* msg_str = message ? env->GetStringUTFChars(message, nullptr) : nullptr;
    ZWCADGuard_SimulateCrashDump(msg_str);
    if (message) env->ReleaseStringUTFChars(message, msg_str);

    ZWCAD_JNI_GUARD_END_VOID(env)
}

JNIEXPORT void JNICALL Java_com_zwsoft_zwcadguard_ZWCADGuard_nativeRecordJavaException
  (JNIEnv *env, jclass clazz, jstring exceptionName, jstring exceptionMessage, jstring stacktrace) {
    (void)clazz;
    ZWCAD_JNI_GUARD_BEGIN

    const char* name_str = exceptionName ? env->GetStringUTFChars(exceptionName, nullptr) : nullptr;
    const char* message_str = exceptionMessage ? env->GetStringUTFChars(exceptionMessage, nullptr) : nullptr;
    const char* stack_str = stacktrace ? env->GetStringUTFChars(stacktrace, nullptr) : nullptr;

    char reason_buf[4096];
    reason_buf[0] = '\0';
    snprintf(reason_buf,
             sizeof(reason_buf),
             "Message: %s\nStacktrace:\n%s",
             message_str ? message_str : "N/A",
             stack_str ? stack_str : "N/A");
    ZWCADGuard_RecordManagedException("Java", name_str, reason_buf);

    if (exceptionName) env->ReleaseStringUTFChars(exceptionName, name_str);
    if (exceptionMessage) env->ReleaseStringUTFChars(exceptionMessage, message_str);
    if (stacktrace) env->ReleaseStringUTFChars(stacktrace, stack_str);

    ZWCAD_JNI_GUARD_END_VOID(env)
}
