#ifndef ZWMOBILE_GUARD_BACKTRACE_H
#define ZWMOBILE_GUARD_BACKTRACE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 回朔调用栈
 *
 * @param buffer 调用栈帧地址的指针数组
 * @param maxFrames 最大抓取帧数
 * @return int 实际抓取的帧数
 */
int zwMobileGuardCaptureBacktrace(void** buffer, int maxFrames);

/**
 * 从 signal 的上下文中回朔调用栈
 *
 * @param signalUserContext signal handler 回调传入的上下文
 * @param buffer 调用栈帧地址的指针数组
 * @param maxFrames 最大抓取帧数
 * @return int 实际抓取的帧数
 */
int zwMobileGuardCaptureSignalBacktrace(void* signalUserContext,
                                        void** buffer,
                                        int maxFrames);

/**
 * @brief 格式化堆栈为可读的文本，若支持，会解析出 Mach-O 模块名与偏移量
 * 
 * @param buffer 调用栈帧指针数组
 * @param frames 帧数
 * @param outputBuf 输出缓冲区
 * @param outputSize 输出缓冲区最大容量
 */
void zwMobileGuardFormatBacktrace(void** buffer, int frames, char* outputBuf, size_t outputSize);

#ifdef __cplusplus
}
#endif

#endif // ZWMOBILE_GUARD_BACKTRACE_H
