#include "ZWMobileGuardRawReport.h"
#include "ZWMobileGuardReportJSON.h"
#include "ZWMobileGuardState.h"

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE 1
#endif

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef __APPLE__
#include <ucontext.h>
#endif

ZWRawAppInfo g_appInfo = {{0}};
ZWRawBinaryImage *g_binaryImages = nullptr;
uint32_t g_binaryImageCount = 0;

// 参照 KSCrash 使用 gettimeofday 获取崩溃发生时间
static uint64_t currentTimestampUs(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((uint64_t)tv.tv_sec) * 1000000 + (uint64_t)tv.tv_usec;
}

static char *appendLiteral(char *cursor, const char *end, const char *text) {
    if (cursor == nullptr || end == nullptr || text == nullptr) {
        return cursor;
    }
    while (cursor < end && *text != '\0') {
        *cursor++ = *text++;
    }
    return cursor;
}

static char *appendUint64Dec(char *cursor, const char *end, uint64_t value) {
    if (cursor == nullptr || end == nullptr) {
        return cursor;
    }

    char temp[32];
    int count = 0;
    if (value == 0) {
        temp[count++] = '0';
    } else {
        while (value > 0 && count < (int)sizeof(temp)) {
            temp[count++] = (char)('0' + (value % 10));
            value /= 10;
        }
    }

    while (count > 0 && cursor < end) {
        *cursor++ = temp[--count];
    }
    return cursor;
}

// 根据时间戳和pid,创建 crash 文件路径
static void buildRawCrashFilePath(char *buffer, size_t bufferSize, uint64_t timestampUs, int pid) {
    if (buffer == nullptr || bufferSize <= 0) {
        return;
    }

    const char *end = buffer + bufferSize - 1;
    buffer = appendLiteral(buffer, end, g_sdkConfig.logDirectory);
    buffer = appendLiteral(buffer, end, "/crash_");
    buffer = appendUint64Dec(buffer, end, timestampUs);
    buffer = appendLiteral(buffer, end, "_");
    buffer = appendUint64Dec(buffer, end, (uint64_t)(uint32_t)pid);
    buffer = appendLiteral(buffer, end, ".json");
    *buffer = '\0';
}

static void captureRegisters(void *context, ZWRawArm64Registers *registers) {
    if (context == nullptr || registers == nullptr) {
        return;
    }
    memset(registers, 0, sizeof(*registers));

#if defined(__arm64__)
    ucontext_t *userContext = (ucontext_t *)context;
    auto machineContext = userContext->uc_mcontext;
    if (machineContext == nullptr) {
        return;
    }
    for (int i = 0; i < 29; ++i) {
        registers->x[i] = (uint64_t)machineContext->__ss.__x[i];
    }
    registers->fp = (uint64_t)machineContext->__ss.__fp;
    registers->lr = (uint64_t)machineContext->__ss.__lr;
    registers->sp = (uint64_t)machineContext->__ss.__sp;
    registers->pc = (uint64_t)machineContext->__ss.__pc;
    registers->cpsr = (uint64_t)machineContext->__ss.__cpsr;
    registers->far = (uint64_t)machineContext->__es.__far;
    registers->esr = (uint64_t)machineContext->__es.__esr;
#elif defined(__x86_64__)
    ucontext_t *userContext = (ucontext_t *)context;
    auto machineContext = userContext->uc_mcontext;
    if (machineContext == nullptr) {
        return;
    }
    registers->pc = (uint64_t)machineContext->__ss.__rip;
    registers->fp = (uint64_t)machineContext->__ss.__rbp;
    registers->sp = (uint64_t)machineContext->__ss.__rsp;
#endif
}
// 初始化基础 App 信息
void zwMobileGuardRawReportInit(const char *processName, const char *processPath, const char *bundleId, const char *appVersion, const char *buildVersion, const char *osVersion, const char *deviceModel) {
    // 获取进程id
    g_appInfo.pid = getpid();
    // 使用原生层传入的值填充元数据
    snprintf(g_appInfo.processName, sizeof(g_appInfo.processName), "%s", processName ? processName : "");
    snprintf(g_appInfo.processPath, sizeof(g_appInfo.processPath), "%s", processPath ? processPath : "");
    snprintf(g_appInfo.bundleId, sizeof(g_appInfo.bundleId), "%s", bundleId ? bundleId : "");
    snprintf(g_appInfo.appVersion, sizeof(g_appInfo.appVersion), "%s", appVersion ? appVersion : "");
    snprintf(g_appInfo.buildVersion, sizeof(g_appInfo.buildVersion), "%s", buildVersion ? buildVersion : "");
    snprintf(g_appInfo.osVersion, sizeof(g_appInfo.osVersion), "%s", osVersion ? osVersion : "");
    snprintf(g_appInfo.deviceModel, sizeof(g_appInfo.deviceModel), "%s", deviceModel ? deviceModel : "");

#if defined(__arm64__)
    snprintf(g_appInfo.codeType, sizeof(g_appInfo.codeType), "%s", "ARM-64");
#elif defined(__x86_64__)
    snprintf(g_appInfo.codeType, sizeof(g_appInfo.codeType), "%s", "X86-64");
#else
    snprintf(g_appInfo.codeType, sizeof(g_appInfo.codeType), "%s", "Unknown");
#endif
}

void zwMobileGuardWriteReportInternal(int crashType, int sig, siginfo_t *sigInfo, void *sigContext, const char *exceptionClass, const char *exceptionReason, void **frames, int frameCount) {
    // 崩溃现场使用静态存储,防止信号处理器触发栈溢出
    static ZWRawCrashReport report;
    memset(&report, 0, sizeof(report));

    report.header.crashType = crashType;
    report.header.crashTimestamp = currentTimestampUs();

    if (crashType == ZWRawCrashTypeSignal) {
        report.header.signalNumber = sig;
        report.header.signalCode = sigInfo ? sigInfo->si_code : 0;
        report.header.faultAddress = sigInfo ? (uintptr_t)sigInfo->si_addr : 0;
#ifdef __APPLE__
        captureRegisters(sigContext, &report.registers);
#endif
    } else {
        if (exceptionClass) {
            strncpy(report.exceptionClass, exceptionClass, sizeof(report.exceptionClass) - 1);
        }
        if (exceptionReason) {
            strncpy(report.exceptionReason, exceptionReason, sizeof(report.exceptionReason) - 1);
        }
    }

    if (frameCount < 0) {
        frameCount = 0;
    }
    if (frameCount > MAX_STACK_FRAMES) {
        frameCount = MAX_STACK_FRAMES;
    }
    report.header.frameCount = (uint32_t)frameCount;
    for (int i = 0; i < frameCount; ++i) {
        report.frames[i] = (uint64_t)(uintptr_t)frames[i];
    }
    // 设置崩溃 app 信息
    report.appInfo = g_appInfo;
    // 设置动态库数量
    report.header.imageCount = g_binaryImageCount;

    // 设置操作路径
    report.header.breadcrumbCount = g_breadCrumbSnapshot.count;
    int breadcrumbIndex = g_breadCrumbSnapshot.head;
    for (uint32_t i = 0; i < report.header.breadcrumbCount; ++i) {
        report.breadcrumbs[i] = g_breadCrumbSnapshot.items[breadcrumbIndex];
        breadcrumbIndex = (breadcrumbIndex + 1) % MAX_BREADCRUMBS;
    }
    
    // 读取图纸信息快照数据
    report.activeDrawing = g_activeDrawingSnapshot;

    // 构建落盘的 JSON 文件路径
    char jsonCrashPath[768];
    buildRawCrashFilePath(jsonCrashPath, sizeof(jsonCrashPath), report.header.crashTimestamp / 1000000ULL, report.appInfo.pid);

    // 序列化写入 JSON 文件
    zwMobileGuardWriteJSONReport(&report, jsonCrashPath);
}
