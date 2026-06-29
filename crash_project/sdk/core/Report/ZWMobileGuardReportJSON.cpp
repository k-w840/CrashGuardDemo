#include "ZWMobileGuardReportJSON.h"
#include "KSFileUtils.h"
#include "KSJSONCodec.h"

#ifdef __APPLE__
#include <uuid/uuid.h>
#endif

#include <stdio.h>
#include <string.h>

static void safeFormatUuid(const unsigned char *uuid, char *outStr) {
    static const char hexChars[] = "0123456789ABCDEF";
    int dest = 0;
    for (int i = 0; i < 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            outStr[dest++] = '-';
        }
        outStr[dest++] = hexChars[(uuid[i] >> 4) & 0x0F];
        outStr[dest++] = hexChars[uuid[i] & 0x0F];
    }
    outStr[dest] = '\0';
}

// 每次写入数据回掉的函数
static int addJSONDataToBufferedWriter(const char *data, int length, void *userData) {
    KSBufferedWriter *writer = (KSBufferedWriter *)userData;
    if (writer == nullptr) {
        return KSJSON_ERROR_CANNOT_ADD_DATA;
    }
    return ksfu_writeBufferedWriter(writer, data, length) ? KSJSON_OK : KSJSON_ERROR_CANNOT_ADD_DATA;
}

#pragma mark - 格式化 carsh 日志
static void addReportMetadata(KSJSONEncodeContext *encoderContext, const ZWRawCrashReport *report, const char *reportIdStr) {
    ksjson_beginObject(encoderContext, "report");
    ksjson_addStringElement(encoderContext, "version", "3.3.0", KSJSON_SIZE_AUTOMATIC);
    ksjson_addStringElement(encoderContext, "id", reportIdStr, KSJSON_SIZE_AUTOMATIC);
    ksjson_addStringElement(encoderContext, "process_name", report->appInfo.processName, KSJSON_SIZE_AUTOMATIC);
    ksjson_addUIntegerElement(encoderContext, "timestamp", report->header.crashTimestamp / 1000000ULL);
    ksjson_addStringElement(encoderContext, "type", "standard", KSJSON_SIZE_AUTOMATIC);
    ksjson_endContainer(encoderContext);
}

static void addBinaryImages(KSJSONEncodeContext *encoderContext) {
    ksjson_beginArray(encoderContext, "binary_images");
    for (uint32_t i = 0; i < g_binaryImageCount; ++i) {
        const ZWRawBinaryImage *img = &g_binaryImages[i];
        ksjson_beginObject(encoderContext, nullptr);
        ksjson_addUIntegerElement(encoderContext, "image_addr", img->base);
        ksjson_addUIntegerElement(encoderContext, "image_size", img->size);
        ksjson_addStringElement(encoderContext, "uuid", img->uuid, KSJSON_SIZE_AUTOMATIC);
        ksjson_addStringElement(encoderContext, "name", img->name, KSJSON_SIZE_AUTOMATIC);
        ksjson_addStringElement(encoderContext, "path", img->path, KSJSON_SIZE_AUTOMATIC);
        ksjson_addUIntegerElement(encoderContext, "image_vmaddr", img->base);
        ksjson_endContainer(encoderContext);
    }
    ksjson_endContainer(encoderContext);
}

static void addSystemInfo(KSJSONEncodeContext *encoderContext, const ZWRawCrashReport *report) {
    ksjson_beginObject(encoderContext, "system");
    ksjson_addStringElement(encoderContext, "CFBundleIdentifier", report->appInfo.bundleId, KSJSON_SIZE_AUTOMATIC);
    ksjson_addStringElement(encoderContext, "CFBundleShortVersionString", report->appInfo.appVersion, KSJSON_SIZE_AUTOMATIC);
    ksjson_addStringElement(encoderContext, "CFBundleVersion", report->appInfo.buildVersion, KSJSON_SIZE_AUTOMATIC);
    ksjson_addStringElement(encoderContext, "os_version", report->appInfo.osVersion, KSJSON_SIZE_AUTOMATIC);
    ksjson_addStringElement(encoderContext, "model", report->appInfo.deviceModel, KSJSON_SIZE_AUTOMATIC);
    ksjson_addStringElement(encoderContext, "cpu_arch", report->appInfo.codeType, KSJSON_SIZE_AUTOMATIC);
    ksjson_addStringElement(encoderContext, "process_name", report->appInfo.processName, KSJSON_SIZE_AUTOMATIC);
    ksjson_addIntegerElement(encoderContext, "process_id", report->appInfo.pid);
    ksjson_endContainer(encoderContext);
}

static void addCrashError(KSJSONEncodeContext *encoderContext, const ZWRawCrashReport *report) {
    ksjson_beginObject(encoderContext, "error");
    ksjson_addUIntegerElement(encoderContext, "address", report->header.faultAddress);
    if (report->header.crashType == ZWRawCrashTypeCPP) {
        ksjson_addStringElement(encoderContext, "type", "cpp_exception", KSJSON_SIZE_AUTOMATIC);
        ksjson_beginObject(encoderContext, "cpp_exception");
        ksjson_addStringElement(encoderContext, "class", report->exceptionClass, KSJSON_SIZE_AUTOMATIC);
        ksjson_addStringElement(encoderContext, "name", report->exceptionClass, KSJSON_SIZE_AUTOMATIC);
        ksjson_addStringElement(encoderContext, "reason", report->exceptionReason, KSJSON_SIZE_AUTOMATIC);
        ksjson_endContainer(encoderContext);
    } else if (report->header.crashType == ZWRawCrashTypeObjC || report->header.crashType == ZWRawCrashTypeManaged) {
        ksjson_addStringElement(encoderContext, "type", "nsexception", KSJSON_SIZE_AUTOMATIC);
        ksjson_beginObject(encoderContext, "nsexception");
        ksjson_addStringElement(encoderContext, "name", report->exceptionClass, KSJSON_SIZE_AUTOMATIC);
        ksjson_addStringElement(encoderContext, "reason", report->exceptionReason, KSJSON_SIZE_AUTOMATIC);
        ksjson_endContainer(encoderContext);
    } else {
        ksjson_addStringElement(encoderContext, "type", "signal", KSJSON_SIZE_AUTOMATIC);
        ksjson_beginObject(encoderContext, "signal");
        ksjson_addIntegerElement(encoderContext, "signal", report->header.signalNumber);
        ksjson_addIntegerElement(encoderContext, "code", report->header.signalCode);
        ksjson_endContainer(encoderContext);
    }
    ksjson_endContainer(encoderContext);
}

static void addThreadRegisters(KSJSONEncodeContext *encoderContext, const ZWRawCrashReport *report) {
    ksjson_beginObject(encoderContext, "registers");
    ksjson_beginObject(encoderContext, "basic");
    ksjson_addUIntegerElement(encoderContext, "pc", report->registers.pc);
    ksjson_addUIntegerElement(encoderContext, "sp", report->registers.sp);
    ksjson_addUIntegerElement(encoderContext, "lr", report->registers.lr);
    ksjson_addUIntegerElement(encoderContext, "fp", report->registers.fp);
    ksjson_addUIntegerElement(encoderContext, "cpsr", report->registers.cpsr);
#if defined(__arm64__) || defined(__aarch64__)
    for (int i = 0; i < 29; ++i) {
        char regName[8];
        snprintf(regName, sizeof(regName), "x%d", i);
        ksjson_addUIntegerElement(encoderContext, regName, report->registers.x[i]);
    }
#endif
    ksjson_endContainer(encoderContext);
    ksjson_beginObject(encoderContext, "exception");
    ksjson_addUIntegerElement(encoderContext, "esr", report->registers.esr);
    ksjson_addUIntegerElement(encoderContext, "far", report->registers.far);
    ksjson_endContainer(encoderContext);
    ksjson_endContainer(encoderContext);
}

static void addCrashedThread(KSJSONEncodeContext *encoderContext, const ZWRawCrashReport *report) {
    ksjson_beginArray(encoderContext, "threads");
    ksjson_beginObject(encoderContext, nullptr);
    ksjson_addBooleanElement(encoderContext, "crashed", true);
    ksjson_addBooleanElement(encoderContext, "current_thread", true);
    ksjson_addIntegerElement(encoderContext, "index", 0);
    ksjson_beginObject(encoderContext, "backtrace");
    ksjson_beginArray(encoderContext, "contents");
    for (uint32_t i = 0; i < report->header.frameCount; ++i) {
        ksjson_beginObject(encoderContext, nullptr);
        ksjson_addUIntegerElement(encoderContext, "instruction_addr", report->frames[i]);
        ksjson_endContainer(encoderContext);
    }
    ksjson_endContainer(encoderContext);
    ksjson_endContainer(encoderContext);
    // 寄存器快照
    addThreadRegisters(encoderContext, report);
    ksjson_endContainer(encoderContext);
    ksjson_endContainer(encoderContext);
}

static void addUserSection(KSJSONEncodeContext *encoderContext, const ZWRawCrashReport *report) {
    const ActiveDrawingInfo *draw = &report->activeDrawing;
    ksjson_beginObject(encoderContext, "user");
    ksjson_beginArray(encoderContext, "breadcrumbs");
    // 操作路径
    for (uint32_t i = 0; i < report->header.breadcrumbCount; ++i) {
        const BreadCrumb *bc = &report->breadcrumbs[i];
        ksjson_beginObject(encoderContext, nullptr);
        ksjson_addStringElement(encoderContext, "time", bc->time_str, KSJSON_SIZE_AUTOMATIC);
        ksjson_addStringElement(encoderContext, "category", bc->category, KSJSON_SIZE_AUTOMATIC);
        ksjson_addStringElement(encoderContext, "action", bc->action, KSJSON_SIZE_AUTOMATIC);
        ksjson_addStringElement(encoderContext, "details", bc->details, KSJSON_SIZE_AUTOMATIC);
        ksjson_endContainer(encoderContext);
    }
    // 关联图纸
    ksjson_endContainer(encoderContext);
    ksjson_beginObject(encoderContext, "active_drawing");
    ksjson_addStringElement(encoderContext, "name", draw->name, KSJSON_SIZE_AUTOMATIC);
    ksjson_addStringElement(encoderContext, "path", draw->path, KSJSON_SIZE_AUTOMATIC);
    ksjson_addIntegerElement(encoderContext, "size", draw->size);
    ksjson_addStringElement(encoderContext, "hash", draw->hash, KSJSON_SIZE_AUTOMATIC);
    ksjson_addStringElement(encoderContext, "file_id", draw->fileId, KSJSON_SIZE_AUTOMATIC);
    ksjson_addStringElement(encoderContext, "project_id", draw->projectId, KSJSON_SIZE_AUTOMATIC);
    ksjson_addStringElement(encoderContext, "project_name", draw->projectName, KSJSON_SIZE_AUTOMATIC);
    ksjson_addBooleanElement(encoderContext, "is_active", draw->isActive);
    ksjson_endContainer(encoderContext);
    ksjson_endContainer(encoderContext);
}

static void encodeCrashReportJSON(KSJSONEncodeContext *encoderContext, const ZWRawCrashReport *report, const char *reportIdStr) {
    ksjson_beginObject(encoderContext, nullptr);
    ksjson_beginObject(encoderContext, "report");
    // report 数据
    addReportMetadata(encoderContext, report, reportIdStr);
    // 动态库列表
    addBinaryImages(encoderContext);
    // app 信息
    addSystemInfo(encoderContext, report);
    ksjson_beginObject(encoderContext, "crash");
    // crash 异常错误分类
    addCrashError(encoderContext, report);
    // crash 线程调用栈
    addCrashedThread(encoderContext, report);
    ksjson_endContainer(encoderContext);
    // 用户行为信息
    addUserSection(encoderContext, report);
    ksjson_endContainer(encoderContext);
    ksjson_endContainer(encoderContext);
    ksjson_endEncode(encoderContext);
}

void zwMobileGuardWriteJSONReport(const ZWRawCrashReport *report, const char *filePath) {
    char writeBuffer[4096];
    KSBufferedWriter writer;
    if (!ksfu_openBufferedWriter(&writer, filePath, writeBuffer, sizeof(writeBuffer))) {
        return;
    }

    // 构造符合 KSCrash 规范 of Report ID
    uuid_t reportId;
    memset(reportId, 0, sizeof(reportId));
    uint64_t ts = report->header.crashTimestamp;
    int pidVal = report->appInfo.pid;
    memcpy(reportId, &ts, sizeof(ts));
    memcpy(&reportId[8], &pidVal, sizeof(pidVal));
    char reportIdStr[64];
    safeFormatUuid(reportId, reportIdStr);

    KSJSONEncodeContext encoderContext;
    // 开始写入准备,配置写入上下文
    ksjson_beginEncode(&encoderContext, true, addJSONDataToBufferedWriter, &writer);
    // 开始写入
    encodeCrashReportJSON(&encoderContext, report, reportIdStr);
    // 写入结束
    ksfu_closeBufferedWriter(&writer);
}
