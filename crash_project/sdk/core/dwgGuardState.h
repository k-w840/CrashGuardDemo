#ifndef ZWMOBILE_GUARD_STATE_H
#define ZWMOBILE_GUARD_STATE_H

#include <stddef.h>
#include <exception>
#include <pthread.h>
#include <signal.h>

#include "dwgGuardInternal.h"

struct BreadCrumbStore {
    BreadCrumb items[MAX_BREADCRUMBS];
    int head;
    int count;
    pthread_mutex_t mutex;
};

struct ActiveDrawingState {
    char name[256];
    char path[512];
    long size;
    char hash[64];
    char fileId[64];
    char projectId[64];
    char projectName[256];
    bool isActive;
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

extern thread_local ThreadExceptionInfo g_threadException;
extern BreadCrumbStore g_breadCrumbs;
extern ActiveDrawingState g_activeDrawing;
extern SDKConfig g_sdkConfig;

#endif // ZWMOBILE_GUARD_STATE_H
