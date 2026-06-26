#include "dwgGuardState.h"

// 使用 thread_local 保存每个线程的最新 throw 堆栈
thread_local ThreadExceptionInfo g_threadException = {{0}, 0, {0}, false};

// 环形存储辅助数据结构
BreadCrumbStore g_breadCrumbs = {{{0}}, 0, 0, PTHREAD_MUTEX_INITIALIZER};

// 图纸信息
ActiveDrawingState g_activeDrawing = {{0}, {0}, 0, {0}, {0}, {0}, {0}, false, PTHREAD_MUTEX_INITIALIZER};

// SDK全局配置
SDKConfig g_sdkConfig = {{0}, nullptr, {{0}}, false};
