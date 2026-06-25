#ifndef ZWCAD_GUARD_JNI_H
#define ZWCAD_GUARD_JNI_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     com_zwsoft_zwcadguard_ZWCADGuard
 * Method:    nativeInit
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_zwsoft_zwcadguard_ZWCADGuard_nativeInit
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_zwsoft_zwcadguard_ZWCADGuard
 * Method:    nativeSetActiveDrawing
 * Signature: (Ljava/lang/String;Ljava/lang/String;JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_zwsoft_zwcadguard_ZWCADGuard_nativeSetActiveDrawing
  (JNIEnv *, jclass, jstring, jstring, jlong, jstring);

/*
 * Class:     com_zwsoft_zwcadguard_ZWCADGuard
 * Method:    nativeSetActiveDrawingContext
 * Signature: (Ljava/lang/String;Ljava/lang/String;JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_zwsoft_zwcadguard_ZWCADGuard_nativeSetActiveDrawingContext
  (JNIEnv *, jclass, jstring, jstring, jlong, jstring, jstring, jstring, jstring);

/*
 * Class:     com_zwsoft_zwcadguard_ZWCADGuard
 * Method:    nativeClearActiveDrawing
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_zwsoft_zwcadguard_ZWCADGuard_nativeClearActiveDrawing
  (JNIEnv *, jclass);

/*
 * Class:     com_zwsoft_zwcadguard_ZWCADGuard
 * Method:    nativeAddBreadcrumb
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_zwsoft_zwcadguard_ZWCADGuard_nativeAddBreadcrumb
  (JNIEnv *, jclass, jstring, jstring, jstring);

/*
 * Class:     com_zwsoft_zwcadguard_ZWCADGuard
 * Method:    nativeSimulateCrash
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_zwsoft_zwcadguard_ZWCADGuard_nativeSimulateCrash
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_zwsoft_zwcadguard_ZWCADGuard
 * Method:    nativeRecordJavaException
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_zwsoft_zwcadguard_ZWCADGuard_nativeRecordJavaException
  (JNIEnv *, jclass, jstring, jstring, jstring);

#ifdef __cplusplus
}
#endif

#endif // ZWCAD_GUARD_JNI_H
