#ifndef ZWMOBILE_GUARD_BACKTRACE_H
#define ZWMOBILE_GUARD_BACKTRACE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 抓取当前调用栈
 * 
 * @param buffer 存放调用栈帧地址指针的数组
 * @param max_frames 最大抓取帧数
 * @return int 实际抓取的帧数
 */
int zwMobileGuardCaptureBacktrace(void** buffer, int max_frames);

/**
 * @brief 格式化堆栈为可读的文本，若支持，会解析出 Mach-O 模块名与偏移量
 * 
 * @param buffer 调用栈帧指针数组
 * @param frames 帧数
 * @param output_buf 输出缓冲区
 * @param output_size 输出缓冲区最大容量
 */
void zwMobileGuardFormatBacktrace(void** buffer, int frames, char* output_buf, size_t output_size);

#ifdef __cplusplus
}
#endif

#endif // ZWMOBILE_GUARD_BACKTRACE_H
