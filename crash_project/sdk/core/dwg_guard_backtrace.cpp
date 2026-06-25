#include "dwg_guard_backtrace.h"
#include <dlfcn.h>
#include <cxxabi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __APPLE__
#include <execinfo.h>
#endif

#ifdef __ANDROID__
#include <unwind.h>
#endif

// Android 栈展开辅助结构与回调
#ifdef __ANDROID__
struct AndroidUnwindState {
    void** buffer;
    int maxFrames;
    int count;
};

// 栈回朔调用函数
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

extern "C" char* zwMobileGuardDemangle(const char* mangled_name) {
    if (mangled_name == NULL) {
        return NULL;
    }
    int status = 0;
    char* demangled = abi::__cxa_demangle(mangled_name, NULL, NULL, &status);
    if (status == 0 && demangled != NULL) {
        return demangled; // 调用者需 free()
    }
    // 如果还原失败，返回原名字的副本
    return strdup(mangled_name);
}

extern "C" void zwMobileGuardFormatBacktrace(void** buffer, int frames, char* output_buf, size_t output_size) {
    if (buffer == NULL || frames <= 0 || output_buf == NULL || output_size == 0) {
        return;
    }
    
    size_t written = 0;
    output_buf[0] = '\0';
    
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
                int len = snprintf(output_buf + written, output_size - written,
                                   "#%02d: %s + 0x%lx (%s + 0x%lx)\n",
                                   i, lib_name, (unsigned long)offset,
                                   demangled ? demangled : info.dli_sname,
                                   (unsigned long)((uintptr_t)addr - (uintptr_t)info.dli_saddr));
                if (demangled) {
                    free(demangled);
                }
                if (len > 0 && written + len < output_size) {
                    written += len;
                } else {
                    break;
                }
            } else {
                // 无符号名，仅输出模块偏移
                int len = snprintf(output_buf + written, output_size - written,
                                   "#%02d: %s + 0x%lx (0x%p)\n",
                                   i, lib_name, (unsigned long)offset, addr);
                if (len > 0 && written + len < output_size) {
                    written += len;
                } else {
                    break;
                }
            }
        } else {
            // 无法获取模块信息，直接输出原始指针地址
            int len = snprintf(output_buf + written, output_size - written,
                               "#%02d: Unknown Module (0x%p)\n",
                               i, addr);
            if (len > 0 && written + len < output_size) {
                written += len;
            } else {
                break;
            }
        }
    }
}
