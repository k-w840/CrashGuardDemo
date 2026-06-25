package com.zwsoft.zwcadguard.demo;

public final class NativeBridge {
    private NativeBridge() {
    }

    public static native void triggerUnhandledCppException();

    public static native void triggerSignalCrash();

    public static native int callGuardedCppException();
}
