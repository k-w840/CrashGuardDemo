#include "ZWMobileGuardBacktrace.h"
#include <dlfcn.h>
#include <cxxabi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE 1
#endif

#ifdef __APPLE__
#include <execinfo.h>
#if __has_include(<sys/_types/_ucontext64.h>)
#include <sys/_types/_ucontext64.h>
#endif
#include <ucontext.h>
#endif

#ifdef __ANDROID__
#include <unwind.h>
#endif

#ifdef __APPLE__
#ifdef __arm64__
#define ZW_MOBILE_GUARD_CONTEXT_64
#endif

#ifdef ZW_MOBILE_GUARD_CONTEXT_64
#define ZW_MOBILE_GUARD_UC_MCONTEXT uc_mcontext64
typedef ucontext64_t ZWMobileGuardSignalUserContext;
#undef ZW_MOBILE_GUARD_CONTEXT_64
#else
#define ZW_MOBILE_GUARD_UC_MCONTEXT uc_mcontext
typedef ucontext_t ZWMobileGuardSignalUserContext;
#endif

struct ZWMobileGuardFrameEntry {
    ZWMobileGuardFrameEntry* previous;
    uintptr_t returnAddress;
};

// 从 signal 上下文中提取 machine context
static _STRUCT_MCONTEXT* machineContextFromSignalContext(void* signalUserContext) {
    if (signalUserContext == nullptr) {
        return nullptr;
    }
    return ((ZWMobileGuardSignalUserContext*)signalUserContext)->ZW_MOBILE_GUARD_UC_MCONTEXT;
}

// 规范化指令地址
static uintptr_t normalizeInstructionPointer(uintptr_t instructionPointer) {
#if defined(__arm64__)
    // 参照 KSCrash，去除 arm64e PAC 高位，避免符号化时地址异常
    return instructionPointer & 0x0000000fffffffff;
#else
    return instructionPointer;
#endif
}

// 获取指令地址
static uintptr_t instructionAddressFromMachineContext(_STRUCT_MCONTEXT* machineContext) {
    if (machineContext == nullptr) {
        return 0;
    }
#if defined(__arm64__)
    return (uintptr_t)machineContext->__ss.__pc;
#elif defined(__x86_64__)
    return (uintptr_t)machineContext->__ss.__rip;
#else
    return 0;
#endif
}

// 获取栈帧指针地址
static uintptr_t framePointerFromMachineContext(_STRUCT_MCONTEXT* machineContext) {
    if (machineContext == nullptr) {
        return 0;
    }
#if defined(__arm64__)
    return (uintptr_t)machineContext->__ss.__fp;
#elif defined(__x86_64__)
    return (uintptr_t)machineContext->__ss.__rbp;
#else
    return 0;
#endif
}

// 获取链接注册地址
static uintptr_t linkRegisterFromMachineContext(_STRUCT_MCONTEXT* machineContext) {
    if (machineContext == nullptr) {
        return 0;
    }
#if defined(__arm64__)
    return (uintptr_t)machineContext->__ss.__lr;
#else
    return 0;
#endif
}

#endif

// Android 栈展开辅助结构
#ifdef __ANDROID__
struct AndroidUnwindState {
    void** buffer;
    int maxFrames;
    int count;
};

// 安卓栈回朔调用函数
static _Unwind_Reason_Code androidUnwindCallback(struct _Unwind_Context* context, void* arg) {
    AndroidUnwindState* state = (AndroidUnwindState*)arg;
    if (state->count >= state->max_frames) {
        return _URC_END_OF_STACK;
    }
    // 获取当前栈帧的指令指针 (IP)
    uintptr_t ip = _Unwind_GetIP(context);
    if (ip != 0) {
        state->buffer[state->count++] = (void*)ip;
    }
    return _URC_NO_REASON;
}
#endif

#pragma mark -栈回朔对外 API
// 回朔调用栈
extern "C" int zwMobileGuardCaptureBacktrace(void** buffer, int maxFrames) {
    if (maxFrames <= 0 || buffer == NULL) {
        return 0;
    }
    
#ifdef __APPLE__
    // iOS 平台使用系统的 backtrace
    return backtrace(buffer, maxFrames);
#elif defined(__ANDROID__)
    // Android 平台使用 _Unwind_Backtrace 进行栈回溯
    AndroidUnwindState state = { buffer, maxFrames, 0 };
    _Unwind_Backtrace(androidUnwindCallback, &state);
    return state.count;
#else
    // 其它平台占位实现
    return 0;
#endif
}

// 信号量栈回朔
extern "C" int zwMobileGuardCaptureSignalBacktrace(void* signalUserContext, void** buffer, int maxFrames) {
    if (signalUserContext == nullptr || buffer == nullptr || maxFrames <= 0) {
        return 0;
    }

#ifdef __APPLE__
    _STRUCT_MCONTEXT* machineContext = machineContextFromSignalContext(signalUserContext);
    if (machineContext == nullptr) {
        return 0;
    }
    int frameCount = 0;
    if (frameCount < maxFrames) {
        // 先拿 pc/rip 指令地址（正在执行的指令/崩溃的那一行）
        uintptr_t instructionAddress = instructionAddressFromMachineContext(machineContext);
        // 过滤明显无效的结尾地址
        if (instructionAddress > 1) {
            buffer[frameCount++] = (void*)normalizeInstructionPointer(instructionAddress);
        }
    }
    if (frameCount < maxFrames) {
        // 获取链接地址（上一级调用者的返回地址，如 a 调用 b，即为 b 返回到 a 的指令地址）
        uintptr_t linkRegister = linkRegisterFromMachineContext(machineContext);
        if (linkRegister > 1) {
            buffer[frameCount++] = (void*)normalizeInstructionPointer(linkRegister);
        }
    }

    // 获取 fp/rbp 栈帧指针地址，回朔调用链路
    uintptr_t framePointerValue = framePointerFromMachineContext(machineContext);
    // 栈帧地址存储的数据，为上一个栈帧的地址 和 当前函数的返回地址，这里用结构体来表示
    ZWMobileGuardFrameEntry* frame = (ZWMobileGuardFrameEntry*)framePointerValue;
    while (frame != nullptr && frameCount < maxFrames) {
        // 地址对齐检查
        uintptr_t pointerValue = (uintptr_t)frame;
        if ((pointerValue & (sizeof(uintptr_t) - 1)) != 0) {
            break;
        }

        ZWMobileGuardFrameEntry currentFrame = *frame;
        // 地址错误 || 上一个栈帧为自己，结束循环
        if (currentFrame.returnAddress == 0 || currentFrame.previous == frame) {
            break;
        }
        // 参照 kscrash 过滤明显无效地址
        if (currentFrame.returnAddress > 1) {
            buffer[frameCount++] = (void*)normalizeInstructionPointer(currentFrame.returnAddress);
        }
        // 无上一栈帧可结束过滤；栈是向低地址生长的，对不符合规律的过滤
        if (currentFrame.previous == nullptr || (uintptr_t)currentFrame.previous <= (uintptr_t)frame) {
            break;
        }
        frame = currentFrame.previous;
    }
    return frameCount;
#else
    return zwMobileGuardCaptureBacktrace(buffer, maxFrames);
#endif
}

// 把 C++ 的“符号修饰名”还原成函数/类型名。
extern "C" char* zwMobileGuardDemangle(const char* mangledName) {
    if (mangledName == NULL) {
        return NULL;
    }
    int status = 0;
    char* demangled = abi::__cxa_demangle(mangledName, NULL, NULL, &status);
    if (status == 0 && demangled != NULL) {
        return demangled; // 调用者需 free()
    }
    // 如果还原失败，返回原名字的副本
    return strdup(mangledName);
}

extern "C" void zwMobileGuardFormatBacktrace(void** buffer, int frames, char* outputBuf, size_t outputSize) {
    if (buffer == NULL || frames <= 0 || outputBuf == NULL || outputSize == 0) {
        return;
    }
    
    size_t written = 0;
    outputBuf[0] = '\0';
    
    for (int i = 0; i < frames; ++i) {
        void* addr = buffer[i];
        Dl_info info;
        // 使用 dladdr 获取地址对应的动态库模块信息和最接近的符号
        if (dladdr(addr, &info) && info.dli_fname != NULL) {
            // 获取库文件名短名称 (去除路径)
            const char* lib_name = strrchr(info.dli_fname, '/');
            if (lib_name != NULL) {
                lib_name++;
            } else {
                lib_name = info.dli_fname;
            }
            
            // 计算偏移量
            uintptr_t offset = (uintptr_t)addr - (uintptr_t)info.dli_fbase;
            
            if (info.dli_sname != NULL) {
                // 尝试 C++ 符号反混淆 (Demangle)
                char* demangled = zwMobileGuardDemangle(info.dli_sname);
                int len = snprintf(outputBuf + written, outputSize - written,
                                   "#%02d: %s + 0x%lx (%s + 0x%lx)\n",
                                   i, lib_name, (unsigned long)offset,
                                   demangled ? demangled : info.dli_sname,
                                   (unsigned long)((uintptr_t)addr - (uintptr_t)info.dli_saddr));
                if (demangled) {
                    free(demangled);
                }
                if (len > 0 && written + len < outputSize) {
                    written += len;
                } else {
                    break;
                }
            } else {
                // 无符号名，仅输出模块偏移
                int len = snprintf(outputBuf + written, outputSize - written,
                                   "#%02d: %s + 0x%lx (0x%p)\n",
                                   i, lib_name, (unsigned long)offset, addr);
                if (len > 0 && written + len < outputSize) {
                    written += len;
                } else {
                    break;
                }
            }
        } else {
            // 无法获取模块信息，直接输出原始指针地址
            int len = snprintf(outputBuf + written, outputSize - written, "#%02d: Unknown Module (0x%p)\n",
                               i, addr);
            if (len > 0 && written + len < outputSize) {
                written += len;
            } else {
                break;
            }
        }
    }
}
