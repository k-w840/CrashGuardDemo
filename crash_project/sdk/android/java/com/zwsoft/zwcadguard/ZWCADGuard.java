package com.zwsoft.zwcadguard;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Build;

import java.io.PrintWriter;
import java.io.StringWriter;

public final class ZWCADGuard {
    private static volatile boolean initialized = false;
    private static Thread.UncaughtExceptionHandler previousHandler;

    private ZWCADGuard() {
    }

    public static synchronized boolean initialize(Context context, String logDir) {
        if (initialized) {
            return true;
        }

        Context appContext = context.getApplicationContext();
        String packageName = appContext.getPackageName();
        String processName = appContext.getApplicationInfo().processName;
        String processPath = appContext.getApplicationInfo().sourceDir;
        String versionName = "";
        String versionCode = "";

        try {
            PackageInfo packageInfo = appContext.getPackageManager().getPackageInfo(packageName, 0);
            versionName = packageInfo.versionName != null ? packageInfo.versionName : "";
            versionCode = String.valueOf(packageInfo.getLongVersionCode());
        } catch (PackageManager.NameNotFoundException ignored) {
        }

        nativeInit(logDir,
                processName,
                processPath,
                packageName,
                versionName,
                versionCode,
                Build.VERSION.RELEASE,
                Build.MANUFACTURER + " " + Build.MODEL);
        installJavaExceptionHandler();
        addBreadcrumb("App", "Launch", "Android SDK initialized");
        initialized = true;
        return true;
    }

    public static void setActiveDrawing(String name, String path, long size, String hash) {
        nativeSetActiveDrawing(name, path, size, hash);
    }

    public static void setActiveDrawingContext(String name,
                                               String path,
                                               long size,
                                               String hash,
                                               String fileId,
                                               String projectId,
                                               String projectName) {
        nativeSetActiveDrawingContext(name, path, size, hash, fileId, projectId, projectName);
    }

    public static void clearActiveDrawing() {
        nativeClearActiveDrawing();
    }

    public static void addBreadcrumb(String category, String action, String details) {
        nativeAddBreadcrumb(category, action, details);
    }

    public static void simulateCrash(String message) {
        nativeSimulateCrash(message);
    }

    private static void installJavaExceptionHandler() {
        previousHandler = Thread.getDefaultUncaughtExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler((thread, throwable) -> {
            recordJavaException(throwable);
            if (previousHandler != null) {
                previousHandler.uncaughtException(thread, throwable);
            }
        });
    }

    private static void recordJavaException(Throwable throwable) {
        if (throwable == null) {
            return;
        }

        StringWriter stringWriter = new StringWriter();
        PrintWriter printWriter = new PrintWriter(stringWriter);
        throwable.printStackTrace(printWriter);
        printWriter.flush();

        nativeRecordJavaException(throwable.getClass().getName(),
                throwable.getMessage(),
                stringWriter.toString());
    }

    private static native int nativeInit(String logDir,
                                         String processName,
                                         String processPath,
                                         String bundleId,
                                         String appVersion,
                                         String buildVersion,
                                         String osVersion,
                                         String deviceModel);

    private static native void nativeSetActiveDrawing(String name, String path, long size, String hash);

    private static native void nativeSetActiveDrawingContext(String name,
                                                             String path,
                                                             long size,
                                                             String hash,
                                                             String fileId,
                                                             String projectId,
                                                             String projectName);

    private static native void nativeClearActiveDrawing();

    private static native void nativeAddBreadcrumb(String category, String action, String details);

    private static native void nativeSimulateCrash(String message);

    private static native void nativeRecordJavaException(String exceptionName, String exceptionMessage, String stacktrace);
}
