#include "ZWMobileGuardExceptionRecord.h"
#include "ZWMobileGuardBacktrace.h"
#include "ZWMobileGuardInternal.h"
#include "ZWMobileGuardReport.h"
#include <stdio.h>

// 模拟崩溃写入测试
extern "C" void zwMobileGuardSimulateCrashDump(const char *message) {
    void *crashFrames[MAX_STACK_FRAMES];
    // 获取调用栈
    int crashFramesCount = zwMobileGuardCaptureBacktrace(crashFrames, MAX_STACK_FRAMES);
    dumpToFile("Simulated Uncaught Exception", message, crashFrames, crashFramesCount);
}

// 记录OC异常到本地
extern "C" void zwMobileGuardRecordObjCCrash(const char *name, const char *reason, void **frames, int count) {
    char finalReason[512];
    snprintf(finalReason, sizeof(finalReason), "Name: %s, Reason: %s", name ? name : "NULL", reason ? reason : "NULL");
    dumpToFile("OC Uncaught Exception", finalReason, frames, count);
}

// 记录 Java 层未捕获异常
extern "C" void zwMobileGuardRecordManagedException(const char *language, const char *name, const char *reason) {
    char finalReason[2048];
    snprintf(finalReason, sizeof(finalReason), "Language: %s, Reason: %s, Reason: %s", language ? language : "JAVA", name ? name : "NULL", reason ? reason : "NULL");
    // 托管异常(非原生异常)
    dumpToFile("Managed Uncaught Exception", finalReason, nullptr, 0);
}
