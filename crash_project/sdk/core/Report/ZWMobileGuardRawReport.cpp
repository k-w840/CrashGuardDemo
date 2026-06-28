#include "ZWMobileGuardRawReport.h"
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
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach/machine.h>
#include <ucontext.h>
#include <uuid/uuid.h>
#include <mach/mach.h>
#include <sys/mount.h>
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

// 根据时间戳和pid，创建 crash 文件路径
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

#ifdef __APPLE__
static uintptr_t firstCmdAfterHeader(const struct mach_header *const header) {
    switch (header->magic) {
        case MH_MAGIC:
        case MH_CIGAM:
            return (uintptr_t)(header + 1);
        case MH_MAGIC_64:
        case MH_CIGAM_64:
            return (uintptr_t)(((struct mach_header_64 *)header) + 1);
        default:
            return 0;
    }
}

static void parseMachHeader(const struct mach_header *header, intptr_t slide, ZWRawBinaryImage *image) {
    if (header == nullptr || image == nullptr) {
        return;
    }

    uintptr_t cmdPtr = firstCmdAfterHeader(header);
    if (cmdPtr == 0) {
        return;
    }

    uint64_t imageSize = 0;

    for (uint32_t i = 0; i < header->ncmds; ++i) {
        struct load_command *command = (struct load_command *)cmdPtr;
        // 设置uuid
        if (command->cmd == LC_UUID) {
            struct uuid_command *uuidCommand = (struct uuid_command *)cmdPtr;
            uuid_unparse_upper(uuidCommand->uuid, image->uuid);
        } else if (command->cmd == LC_SEGMENT_64) {
            struct segment_command_64 *segment = (struct segment_command_64 *)cmdPtr;
            // 参照 KSCrash 的做法：仅以 __TEXT 段大小作为二进制文件的 size
            if (strcmp(segment->segname, SEG_TEXT) == 0) {
                imageSize = segment->vmsize;
            }
        } else if (command->cmd == LC_SEGMENT) {
            struct segment_command *segment = (struct segment_command *)cmdPtr;
            if (strcmp(segment->segname, SEG_TEXT) == 0) {
                imageSize = segment->vmsize;
            }
        }
        if (command->cmdsize == 0) {
            break;
        }
        cmdPtr += command->cmdsize;
    }

    // 参照 KSCrash 的做法：加载地址直接用 header 头部指针
    image->base = (uintptr_t)header;
    image->size = imageSize;

    // 动态库架构设置
    switch (header->cputype) {
        case CPU_TYPE_ARM64:
            snprintf(image->arch, sizeof(image->arch), "%s", "arm64");
            break;
        case CPU_TYPE_X86_64:
            snprintf(image->arch, sizeof(image->arch), "%s", "x86_64");
            break;
        default:
            snprintf(image->arch, sizeof(image->arch), "%s", "unknown");
            break;
    }
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
#endif

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

void zwMobileGuardRefreshBinaryImages(void) {
#ifdef __APPLE__

    if (g_binaryImages) {
        free(g_binaryImages);
    }
    // 获取准确的动态库总量
    uint32_t imageCount = _dyld_image_count();
    g_binaryImages = (ZWRawBinaryImage *)calloc(imageCount, sizeof(ZWRawBinaryImage));
    g_binaryImageCount = imageCount;

    for (uint32_t i = 0; i < imageCount; ++i) {
        ZWRawBinaryImage *image = &g_binaryImages[i];
        // 设置基地址、大小、uuid等信息
        parseMachHeader(_dyld_get_image_header(i), _dyld_get_image_vmaddr_slide(i), image);
        const char *path = _dyld_get_image_name(i);
        snprintf(image->path, sizeof(image->path), "%s", path ? path : "");
        // 从后向前查找/，路径中最后一个是名字
        const char *last = strrchr(path, '/');
        snprintf(image->name, sizeof(image->name), "%s", last ? last + 1 : path);
    }
#else
    g_binaryImageCount = 0;
#endif
}

#pragma mark - 写入辅助函数
// 将 reportId 转换大写字符串
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

// 向打开的文件描述符中写入指定数据
static void safeWriteStr(int fd, const char *str) {
    if (str) {
        write(fd, str, strlen(str));
    }
}

// 10/16进制数字转为字符存入
static void safeWriteUint(int fd, uintptr_t val, int base) {
    char buf[32];
    char temp[32];
    int i = 0;
    if (val == 0) {
        temp[i++] = '0';
    } else {
        while (val > 0) {
            int rem = val % base;
            temp[i++] = (rem < 10) ? (rem + '0') : (rem - 10 + 'a');
            val /= base;
        }
    }
    int j = 0;
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    buf[j] = '\0';
    write(fd, buf, j);
}

// 写入16进制数字
static void safeWriteHex(int fd, uintptr_t val) {
    safeWriteStr(fd, "0x");
    safeWriteUint(fd, val, 16);
}

// 写入10进制数字
static void safeWriteDec(int fd, long val) {
    if (val < 0) {
        safeWriteStr(fd, "-");
        val = -val;
    }
    safeWriteUint(fd, (uintptr_t)val, 10);
}

// 写入经过 JSON 字符转义的字符串，保证 JSON 语法的合法性
static void safeWriteEscaped(int fd, const char *str) {
    if (str == nullptr) return;
    while (*str != '\0') {
        char c = *str++;
        if (c == '"' || c == '\\') {
            char esc[2] = {'\\', c};
            write(fd, esc, 2);
        } else if (c == '\n') {
            write(fd, "\\n", 2);
        } else if (c == '\r') {
            write(fd, "\\r", 2);
        } else if (c == '\t') {
            write(fd, "\\t", 2);
        } else if ((unsigned char)c < 0x20) {
            write(fd, " ", 1);
        } else {
            write(fd, &c, 1);
        }
    }
}

void zwMobileGuardWriteJSONReport(const ZWRawCrashReport *report, const char *filePath) {
    int fd = open(filePath, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        return;
    }

    // 构造符合 KSCrash 规范 of Report ID
    uuid_t reportId;
    memset(reportId, 0, sizeof(reportId));
    uint64_t ts = report->header.crashTimestamp;
    int pidVal = report->appInfo.pid;
    memcpy(reportId, &ts, sizeof(ts));
    memcpy(reportId + 8, &pidVal, sizeof(pidVal));
    char reportIdStr[64];
    safeFormatUuid(reportId, reportIdStr);

    safeWriteStr(fd, "{\n  \"report\": {\n");

    // 1. report.report 元数据
    safeWriteStr(fd, "    \"report\": {\n      \"version\": \"3.3.0\",\n      \"id\": \"");
    safeWriteEscaped(fd, reportIdStr);
    safeWriteStr(fd, "\",\n      \"process_name\": \"");
    safeWriteEscaped(fd, report->appInfo.processName);
    safeWriteStr(fd, "\",\n      \"timestamp\": ");
    safeWriteDec(fd, report->header.crashTimestamp / 1000000ULL);
    safeWriteStr(fd, ",\n      \"type\": \"standard\"\n    },\n");

    // 2. report.binary_images 动态库列表
    safeWriteStr(fd, "    \"binary_images\": [\n");
    for (uint32_t i = 0; i < g_binaryImageCount; ++i) {
        const ZWRawBinaryImage *img = &g_binaryImages[i];
        safeWriteStr(fd, "      {\n        \"image_addr\": ");
        safeWriteHex(fd, img->base);
        safeWriteStr(fd, ",\n        \"image_size\": ");
        safeWriteDec(fd, img->size);
        safeWriteStr(fd, ",\n        \"uuid\": \"");
        safeWriteEscaped(fd, img->uuid);
        safeWriteStr(fd, "\",\n        \"name\": \"");
        safeWriteEscaped(fd, img->name);
        safeWriteStr(fd, "\",\n        \"image_vmaddr\": ");
        safeWriteHex(fd, img->base);
        safeWriteStr(fd, "\n      }");
        if (i + 1 < g_binaryImageCount) {
            safeWriteStr(fd, ",\n");
        } else {
            safeWriteStr(fd, "\n");
        }
    }
    safeWriteStr(fd, "    ],\n");

    // 3. report.system 系统和硬件/内存信息
    safeWriteStr(fd, "    \"system\": {\n      \"CFBundleIdentifier\": \"");
    safeWriteEscaped(fd, report->appInfo.bundleId);
    safeWriteStr(fd, "\",\n      \"CFBundleShortVersionString\": \"");
    safeWriteEscaped(fd, report->appInfo.appVersion);
    safeWriteStr(fd, "\",\n      \"CFBundleVersion\": \"");
    safeWriteEscaped(fd, report->appInfo.buildVersion);
    safeWriteStr(fd, "\",\n      \"os_version\": \"");
    safeWriteEscaped(fd, report->appInfo.osVersion);
    safeWriteStr(fd, "\",\n      \"model\": \"");
    safeWriteEscaped(fd, report->appInfo.deviceModel);
    safeWriteStr(fd, "\",\n      \"cpu_arch\": \"");
    safeWriteEscaped(fd, report->appInfo.codeType);
    safeWriteStr(fd, "\",\n      \"process_name\": \"");
    safeWriteEscaped(fd, report->appInfo.processName);
    safeWriteStr(fd, "\",\n      \"process_id\": ");
    safeWriteDec(fd, report->appInfo.pid);
    safeWriteStr(fd, "\n    },\n");

    // 4. report.crash 崩溃现场核心 (error 和 threads)
    safeWriteStr(fd, "    \"crash\": {\n");

    // 4.1 report.crash.error 异常错误分类
    safeWriteStr(fd, "      \"error\": {\n        \"address\": ");
    safeWriteHex(fd, report->header.faultAddress);
    safeWriteStr(fd, ",\n");
    if (report->header.crashType == ZWRawCrashTypeCPP) {
        safeWriteStr(fd, "        \"type\": \"cpp_exception\",\n        \"cpp_exception\": {\n          \"class\": \"");
        safeWriteEscaped(fd, report->exceptionClass);
        safeWriteStr(fd, "\",\n          \"name\": \"");
        safeWriteEscaped(fd, report->exceptionClass);
        safeWriteStr(fd, "\",\n          \"reason\": \"");
        safeWriteEscaped(fd, report->exceptionReason);
        safeWriteStr(fd, "\"\n        }\n");
    } else if (report->header.crashType == ZWRawCrashTypeObjC || report->header.crashType == ZWRawCrashTypeManaged) {
        safeWriteStr(fd, "        \"type\": \"nsexception\",\n        \"nsexception\": {\n          \"name\": \"");
        safeWriteEscaped(fd, report->exceptionClass);
        safeWriteStr(fd, "\",\n          \"reason\": \"");
        safeWriteEscaped(fd, report->exceptionReason);
        safeWriteStr(fd, "\"\n        }\n");
    } else {
        safeWriteStr(fd, "        \"type\": \"signal\",\n        \"signal\": {\n          \"signal\": ");
        safeWriteDec(fd, report->header.signalNumber);
        safeWriteStr(fd, ",\n          \"code\": ");
        safeWriteDec(fd, report->header.signalCode);
        safeWriteStr(fd, "\n        }\n");
    }
    safeWriteStr(fd, "      },\n");

    // 4.2 report.crash.threads 线程调用栈
    safeWriteStr(fd, "      \"threads\": [\n        {\n          \"crashed\": true,\n          \"current_thread\": true,\n          \"index\": 0,\n");

    // 4.2.1 调用栈帧数组内容
    safeWriteStr(fd, "          \"backtrace\": {\n            \"contents\": [\n");
    for (uint32_t i = 0; i < report->header.frameCount; ++i) {
        safeWriteStr(fd, "              {\n                \"instruction_addr\": ");
        safeWriteHex(fd, report->frames[i]);
        safeWriteStr(fd, "\n              }");
        if (i + 1 < report->header.frameCount) {
            safeWriteStr(fd, ",\n");
        } else {
            safeWriteStr(fd, "\n");
        }
    }
    safeWriteStr(fd, "            ]\n          },\n");

    // 4.2.2 崩溃现场寄存器快照
    safeWriteStr(fd, "          \"registers\": {\n            \"basic\": {\n              \"pc\": ");
    safeWriteHex(fd, report->registers.pc);
    safeWriteStr(fd, ",\n              \"sp\": ");
    safeWriteHex(fd, report->registers.sp);
    safeWriteStr(fd, ",\n              \"lr\": ");
    safeWriteHex(fd, report->registers.lr);
    safeWriteStr(fd, ",\n              \"fp\": ");
    safeWriteHex(fd, report->registers.fp);
    safeWriteStr(fd, ",\n              \"cpsr\": ");
    safeWriteHex(fd, report->registers.cpsr);

#if defined(__arm64__) || defined(__aarch64__)
    safeWriteStr(fd, ",\n");
    for (int i = 0; i < 29; ++i) {
        safeWriteStr(fd, "              \"x");
        safeWriteDec(fd, i);
        safeWriteStr(fd, "\": ");
        safeWriteHex(fd, report->registers.x[i]);
        if (i < 28) {
            safeWriteStr(fd, ",\n");
        } else {
            safeWriteStr(fd, "\n");
        }
    }
#else
    safeWriteStr(fd, "\n");
#endif
    safeWriteStr(fd, "            },\n            \"exception\": {\n              \"esr\": ");
    safeWriteHex(fd, report->registers.esr);
    safeWriteStr(fd, ",\n              \"far\": ");
    safeWriteHex(fd, report->registers.far);
    safeWriteStr(fd, "\n            }\n          }\n        }\n      ]\n    },\n");

    // 5. report.user 自定义轨迹与业务上下文
    safeWriteStr(fd, "    \"user\": {\n      \"breadcrumbs\": [\n");
    for (uint32_t i = 0; i < report->header.breadcrumbCount; ++i) {
        const BreadCrumb *bc = &report->breadcrumbs[i];
        safeWriteStr(fd, "        {\n          \"time\": \"");
        safeWriteEscaped(fd, bc->time_str);
        safeWriteStr(fd, "\",\n          \"category\": \"");
        safeWriteEscaped(fd, bc->category);
        safeWriteStr(fd, "\",\n          \"action\": \"");
        safeWriteEscaped(fd, bc->action);
        safeWriteStr(fd, "\",\n          \"details\": \"");
        safeWriteEscaped(fd, bc->details);
        safeWriteStr(fd, "\"\n        }");
        if (i + 1 < report->header.breadcrumbCount) {
            safeWriteStr(fd, ",\n");
        } else {
            safeWriteStr(fd, "\n");
        }
    }
    safeWriteStr(fd, "      ],\n      \"active_drawing\": {\n");

    // 活跃图纸信息
    const ActiveDrawingInfo *draw = &report->activeDrawing;
    safeWriteStr(fd, "        \"name\": \"");
    safeWriteEscaped(fd, draw->name);
    safeWriteStr(fd, "\",\n        \"path\": \"");
    safeWriteEscaped(fd, draw->path);
    safeWriteStr(fd, "\",\n        \"size\": ");
    safeWriteDec(fd, draw->size);
    safeWriteStr(fd, ",\n        \"hash\": \"");
    safeWriteEscaped(fd, draw->hash);
    safeWriteStr(fd, "\",\n        \"file_id\": \"");
    safeWriteEscaped(fd, draw->fileId);
    safeWriteStr(fd, "\",\n        \"project_id\": \"");
    safeWriteEscaped(fd, draw->projectId);
    safeWriteStr(fd, "\",\n        \"project_name\": \"");
    safeWriteEscaped(fd, draw->projectName);
    safeWriteStr(fd, "\",\n        \"is_active\": ");
    safeWriteStr(fd, draw->isActive ? "true" : "false");
    safeWriteStr(fd, "\n      }\n    }\n  }\n}\n");

    close(fd);
}

void zwMobileGuardWriteReportInternal(int crashType, int sig, siginfo_t *sigInfo, void *sigContext, const char *exceptionClass, const char *exceptionReason, void **frames, int frameCount) {
    // 崩溃现场使用静态存储，防止信号处理器触发栈溢出
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
    buildRawCrashFilePath(jsonCrashPath, sizeof(jsonCrashPath), report.header.crashTimestamp, report.appInfo.pid);

    // 序列化写入 JSON 文件
    zwMobileGuardWriteJSONReport(&report, jsonCrashPath);
}
