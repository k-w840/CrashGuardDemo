#ifndef ZWMOBILE_GUARD_REPORT_H
#define ZWMOBILE_GUARD_REPORT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 生成崩溃报告文件名并写入
void dumpToFile(const char *crashType, const char *reason, void **crashFrames, int crashFramesCount);

#ifdef __cplusplus
}
#endif

#endif // ZWMOBILE_GUARD_REPORT_H
