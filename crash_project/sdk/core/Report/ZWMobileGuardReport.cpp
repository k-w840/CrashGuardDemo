#include "ZWMobileGuardReport.h"
#include "ZWMobileGuardBacktrace.h"
#include "ZWMobileGuardState.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#pragma mark - 辅助处理函数
// 信号处理函数中不能使用 printf/sprintf/malloc，要使用底层安全调用
// 向打开的文件描述符中写入指定数据
static void safeWriteStr(int fd, const char *str) {
    if (str) {
        write(fd, str, strlen(str));
    }
}

// 10/16进制数字转为字符存入
static void safeWriteUint(int fd, uintptr_t val, int base) {
    // 记录数字
    char buf[32];
    char temp[32];
    int i = 0;
    if (val == 0) {
        temp[i++] = '0';
    } else {
        while (val > 0) {
            // 取余逆序获取数字值
            int rem = val % base;
            // 数字根据ASCII表转为字符（16进制有a、b、c）
            temp[i++] = (rem < 10) ? (rem + '0') : (rem - 10 + 'a');
            val /= base;
        }
    }
    // 转为正序
    int j = 0;
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    // 数字字符串结尾补\0
    buf[j] = '\0';
    write(fd, buf, j);
}

//写入16进制数字
static void safeWriteHex(int fd, uintptr_t val) {
    safeWriteStr(fd, "0x");
    safeWriteUint(fd, val, 16);
}

// 写入10进制数字
static void safeWriteDec(int fd, long val) {
    // 负数先写入符号
    if (val < 0) {
        safeWriteStr(fd, "-");
        val = -val;
    }
    // 转为无符号类型的正数写入
    safeWriteUint(fd, (uintptr_t)val, 10);
}

#pragma mark - 崩溃日志落盘
static void writeCrashReport(int fd, const char *crash_type, const char *reason, void **crashFrames, int crashFramesCount) {
    safeWriteStr(fd, "========================================\n");
    safeWriteStr(fd, "          ZWMOBILE CRASH REPORT       \n");
    safeWriteStr(fd, "========================================\n\n");
    
    // 崩溃类型与原因
    safeWriteStr(fd, "[Crash Type]: ");
    safeWriteStr(fd, crash_type);
    safeWriteStr(fd, "\n");
    
    safeWriteStr(fd, "[Reason]: ");
    safeWriteStr(fd, reason ? reason : "N/A");
    safeWriteStr(fd, "\n\n");
    
    // 图纸元数据 (仅记录脱敏后的标识，符合合规红线)
    pthread_mutex_lock(&g_activeDrawing.mutex);
    safeWriteStr(fd, "[Active Drawing Information]:\n");
    if (g_activeDrawing.isActive) {
        safeWriteStr(fd, "  File Name: ");
        safeWriteStr(fd, g_activeDrawing.name);
        safeWriteStr(fd, "\n  File ID: ");
        safeWriteStr(fd, strlen(g_activeDrawing.fileId) > 0
                       ? g_activeDrawing.fileId
                       : "N/A");
        safeWriteStr(fd, "\n  Project ID: ");
        safeWriteStr(fd, strlen(g_activeDrawing.projectId) > 0
                       ? g_activeDrawing.projectId
                       : "N/A");
        safeWriteStr(fd, "\n  Project Name: ");
        safeWriteStr(fd, strlen(g_activeDrawing.projectName) > 0
                       ? g_activeDrawing.projectName
                       : "N/A");
        safeWriteStr(fd, "\n  File Size: ");
        safeWriteDec(fd, g_activeDrawing.size);
        safeWriteStr(fd, " bytes\n  File MD5/Hash: ");
        safeWriteStr(fd, g_activeDrawing.hash);
        safeWriteStr(fd, "\n  File Path (Raw): ");
        safeWriteStr(fd, g_activeDrawing.path);
        safeWriteStr(fd, "\n");
    } else {
        safeWriteStr(fd, "  No active drawing opened during crash.\n");
    }
    pthread_mutex_unlock(&g_activeDrawing.mutex);
    safeWriteStr(fd, "\n");
    
    // 1. 原始 throw 堆栈（如果有）
    if (g_threadException.hasException) {
        safeWriteStr(fd,
                       "[Original Throw Stack Trace] (Captured at __cxa_throw):\n");
        safeWriteStr(fd, "  Exception Class: ");
        safeWriteStr(fd, g_threadException.exceptionType);
        safeWriteStr(fd, "\n");
        
        char stackDesc[2048];
        zwMobileGuardFormatBacktrace(g_threadException.backtraceBuffer,
                                     g_threadException.framesCount, stackDesc,
                                     sizeof(stackDesc));
        safeWriteStr(fd, stackDesc);
        safeWriteStr(fd, "\n");
    }
    
    // 2. 当前崩溃现场堆栈
    if (crashFrames && crashFramesCount > 0) {
        safeWriteStr(fd, "[Termination Stack Trace] (At crash/signal site):\n");
        char stackDesc[2048];
        zwMobileGuardFormatBacktrace(crashFrames, crashFramesCount, stackDesc, sizeof(stackDesc));
        safeWriteStr(fd, stackDesc);
        safeWriteStr(fd, "\n");
    }
    
    // 3. 用户操作路径 (Breadcrumbs)
    pthread_mutex_lock(&g_breadCrumbs.mutex);
    safeWriteStr(fd, "[User Path Breadcrumbs] (Last 50 operations):\n");
    if (g_breadCrumbs.count == 0) {
        safeWriteStr(fd, "  No operations recorded.\n");
    } else {
        int idx = g_breadCrumbs.head;
        for (int i = 0; i < g_breadCrumbs.count; ++i) {
            BreadCrumb *b = &g_breadCrumbs.items[idx];
            safeWriteStr(fd, "  [");
            safeWriteStr(fd, b->time_str);
            safeWriteStr(fd, "] [");
            safeWriteStr(fd, b->category);
            safeWriteStr(fd, "] ");
            safeWriteStr(fd, b->action);
            if (strlen(b->details) > 0) {
                safeWriteStr(fd, " (");
                safeWriteStr(fd, b->details);
                safeWriteStr(fd, ")");
            }
            safeWriteStr(fd, "\n");
            idx = (idx + 1) % MAX_BREADCRUMBS;
        }
    }
    pthread_mutex_unlock(&g_breadCrumbs.mutex);
    safeWriteStr(fd, "\n");
    
    safeWriteStr(fd, "========================================\n");
}

// 生成崩溃报告文件名 && 写入
void dumpToFile(const char *crashType, const char *reason, void **crashFrames, int crashFramesCount) {
    if (strlen(g_sdkConfig.logDirectory) <= 0) {
        return;
    }
    
    // 生成崩溃报告文件名，以时间戳为文件名一部分
    char filePath[768];
    snprintf(filePath, sizeof(filePath), "%s/crash_%ld.log", g_sdkConfig.logDirectory, (long)time(NULL));
    
    // 写入文件
    int fd = open(filePath, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd >= 0) {
        writeCrashReport(fd, crashType, reason, crashFrames, crashFramesCount);
        close(fd);
    }
}
