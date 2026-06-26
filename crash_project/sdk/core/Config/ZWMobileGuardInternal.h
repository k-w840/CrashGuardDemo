#ifndef ZWMOBILE_GUARD_INTRERNAL_H
#define ZWMOBILE_GUARD_INTRERNAL_H

#include <stddef.h>

// 最大堆栈深度
#define MAX_STACK_FRAMES 64
// 操作路径最大数量
#define MAX_BREADCRUMBS 50

// 线程局部变量，保存最近一次 throw 的异常现场
struct ThreadExceptionInfo {
    void *backtraceBuffer[MAX_STACK_FRAMES];
    int framesCount;
    char exceptionType[256];
    bool hasException;
};

// 操作路径缓存数据
struct BreadCrumb {
    char time_str[32]; // 时间
    char category[64]; // 事件类别
    char action[128];  // 具体操作
    char details[256]; // 详情
};


#endif // ZWMOBILE_GUARD_INTRERNAL_H
