#ifndef ZWMOBILE_GUARD_STATE_H
#define ZWMOBILE_GUARD_STATE_H

#include <stddef.h>
#include <exception>
#include <pthread.h>
#include <signal.h>

#include "ZWMobileGuardInternal.h"

// 环形缓冲区纯数据结构体
struct BreadCrumbStore {
    BreadCrumb items[MAX_BREADCRUMBS];
    int head;
    int count;
};

// 环形缓冲区带锁的状态结构体
struct BreadCrumbState {
    BreadCrumbStore store;
    pthread_mutex_t mutex;
};

// 图纸信息纯数据结构体
struct ActiveDrawingInfo {
    char name[256];
    char path[512];
    long size;
    char hash[64];
    char fileId[64];
    char projectId[64];
    char projectName[256];
    bool isActive;
};

// 图纸信息带锁的状态结构体
struct ActiveDrawingState {
    ActiveDrawingInfo info;
    pthread_mutex_t mutex;
};

struct SDKConfig {
    // 崩溃日志存储路径
    char logDirectory[512];
    // 原始 std::set_terminate 处理
    std::terminate_handler originalTerminateHandler;
    // 系统原信号量处理
    struct sigaction originalSigactions[NSIG];
    // 标记SDK是否已初始化
    bool isInitialized;
};

typedef enum ZWMobileCrashHandlingResult {
    ZWMobileCrashHandlingResultFirstCrash = 0,
    ZWMobileCrashHandlingResultRecrash = 1,
} ZWMobileCrashHandlingResult;

// 统一状态机处理
ZWMobileCrashHandlingResult zwMobileGuardEnterFatalCrashHandling(void);

extern thread_local ThreadExceptionInfo g_threadException;
// 操作路径缓存
extern BreadCrumbState g_breadCrumbs;
// 操作路径快照，降低脏数据概率，确保数据完整（避免在数据生成时崩溃损坏数据）
extern BreadCrumbStore g_breadCrumbSnapshot;
// 图纸信息缓存
extern ActiveDrawingState g_activeDrawing;
// 图纸信息快照
extern ActiveDrawingInfo g_activeDrawingSnapshot;
extern SDKConfig g_sdkConfig;

#endif // ZWMOBILE_GUARD_STATE_H
