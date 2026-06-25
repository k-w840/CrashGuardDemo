#include "dwg_guard.h"
#include "dwg_guard_backtrace.h"
#include <ctime>
#include <dlfcn.h>
#include <exception>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <typeinfo>
#include <unistd.h>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

// 最大堆栈深度
#define MAX_STACK_FRAMES 64
// 操作路径最大数量
#define MAX_BREADCRUMBS 50

// --- 线程局部变量，保存最近一次 throw 的异常现场 ---
struct ThreadExceptionInfo {
    void *backtrace_buffer[MAX_STACK_FRAMES];
    int frames_count;
    char exception_type[128];
    bool has_exception;
};

// 使用 thread_local 保存每个线程的最新 throw 堆栈
static thread_local ThreadExceptionInfo g_thread_exception = {
    {0}, 0, {0}, false};

// 外部声明，用于在 hook 中填充 TLS 栈
extern "C" void ZWCADGuard_RecordThrowState(void *thrown_exception,
                                            std::type_info *tinfo) {
    g_thread_exception.frames_count = ZWCADGuard_CaptureBacktrace(
                                                                  g_thread_exception.backtrace_buffer, MAX_STACK_FRAMES);
    g_thread_exception.has_exception = true;
    if (tinfo && tinfo->name()) {
        char *demangled = ZWCADGuard_Demangle(tinfo->name());
        if (demangled) {
            strncpy(g_thread_exception.exception_type, demangled,
                    sizeof(g_thread_exception.exception_type) - 1);
            free(demangled);
        } else {
            strncpy(g_thread_exception.exception_type, tinfo->name(),
                    sizeof(g_thread_exception.exception_type) - 1);
        }
    } else {
        strcpy(g_thread_exception.exception_type, "Unknown Type");
    }
}

// --- 操作路径缓冲区 ---
struct BreadCrumb {
    char time_str[32];  // 时间
    char category[64];  // 事件类别
    char action[128];   // 具体操作
    char details[256];  // 详情
};

static struct {
    BreadCrumb items[MAX_BREADCRUMBS];
    int head;
    int count;
    pthread_mutex_t mutex;
} g_breadCrumbs = {{{0}}, 0, 0, PTHREAD_MUTEX_INITIALIZER};

// --- 活跃图纸元数据 ---
static struct {
    char name[128];
    char path[256];
    long size;
    char hash[64];
    char file_id[64];
    char project_id[64];
    char project_name[128];
    bool is_active;
    pthread_mutex_t mutex;
} g_active_drawing = {
    {0}, {0}, 0, {0}, {0}, {0}, {0}, false, PTHREAD_MUTEX_INITIALIZER};

// --- 全局配置 ---
static struct {
    char log_directory[512];
    std::terminate_handler original_terminate_handler;
    struct sigaction original_sigactions[NSIG];
    bool is_initialized;
} g_sdk_config = {{0}, nullptr, {{0}}, false};

// --- 异步信号安全辅助函数 ---
// (在信号处理函数中不能使用 printf/sprintf/malloc，只能使用这些底层安全调用)

static void safe_write_str(int fd, const char *str) {
    if (str) {
        write(fd, str, strlen(str));
    }
}

static void safe_write_uint(int fd, uintptr_t val, int base) {
    char buf[32];
    char temp[32];
    int i = 0;
    if (val == 0) {
        temp[i++] = '0';
    } else {
        while (val > 0) {
            int rem = val % base;
            temp[i++] = (rem < 10) ? (rem + '0') : (rem - 10 + 'a');
            val /= base;
        }
    }
    int j = 0;
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    buf[j] = '\0';
    write(fd, buf, j);
}

static void safe_write_hex(int fd, uintptr_t val) {
    safe_write_str(fd, "0x");
    safe_write_uint(fd, val, 16);
}

static void safe_write_dec(int fd, long val) {
    if (val < 0) {
        safe_write_str(fd, "-");
        val = -val;
    }
    safe_write_uint(fd, (uintptr_t)val, 10);
}

// --- 崩溃日志落盘核心实现 ---
static void write_crash_report(int fd, const char *crash_type,
                               const char *reason, void **crash_frames,
                               int crash_frames_count) {
    safe_write_str(fd, "========================================\n");
    safe_write_str(fd, "          ZWCADGuard CRASH REPORT       \n");
    safe_write_str(fd, "========================================\n\n");
    
    // 崩溃类型与原因
    safe_write_str(fd, "[Crash Type]: ");
    safe_write_str(fd, crash_type);
    safe_write_str(fd, "\n");
    
    safe_write_str(fd, "[Reason]: ");
    safe_write_str(fd, reason ? reason : "N/A");
    safe_write_str(fd, "\n\n");
    
    // 图纸元数据 (仅记录脱敏后的标识，符合合规红线)
    pthread_mutex_lock(&g_active_drawing.mutex);
    safe_write_str(fd, "[Active Drawing Information]:\n");
    if (g_active_drawing.is_active) {
        safe_write_str(fd, "  File Name: ");
        safe_write_str(fd, g_active_drawing.name);
        safe_write_str(fd, "\n  File ID: ");
        safe_write_str(fd, strlen(g_active_drawing.file_id) > 0
                       ? g_active_drawing.file_id
                       : "N/A");
        safe_write_str(fd, "\n  Project ID: ");
        safe_write_str(fd, strlen(g_active_drawing.project_id) > 0
                       ? g_active_drawing.project_id
                       : "N/A");
        safe_write_str(fd, "\n  Project Name: ");
        safe_write_str(fd, strlen(g_active_drawing.project_name) > 0
                       ? g_active_drawing.project_name
                       : "N/A");
        safe_write_str(fd, "\n  File Size: ");
        safe_write_dec(fd, g_active_drawing.size);
        safe_write_str(fd, " bytes\n  File MD5/Hash: ");
        safe_write_str(fd, g_active_drawing.hash);
        safe_write_str(fd, "\n  File Path (Raw): ");
        safe_write_str(fd, g_active_drawing.path);
        safe_write_str(fd, "\n");
    } else {
        safe_write_str(fd, "  No active drawing opened during crash.\n");
    }
    pthread_mutex_unlock(&g_active_drawing.mutex);
    safe_write_str(fd, "\n");
    
    // 1. 原始 throw 堆栈（如果有）
    if (g_thread_exception.has_exception) {
        safe_write_str(fd,
                       "[Original Throw Stack Trace] (Captured at __cxa_throw):\n");
        safe_write_str(fd, "  Exception Class: ");
        safe_write_str(fd, g_thread_exception.exception_type);
        safe_write_str(fd, "\n");
        
        char stack_desc[2048];
        ZWCADGuard_FormatBacktrace(g_thread_exception.backtrace_buffer,
                                   g_thread_exception.frames_count, stack_desc,
                                   sizeof(stack_desc));
        safe_write_str(fd, stack_desc);
        safe_write_str(fd, "\n");
    }
    
    // 2. 当前崩溃现场堆栈
    if (crash_frames && crash_frames_count > 0) {
        safe_write_str(fd, "[Termination Stack Trace] (At crash/signal site):\n");
        char stack_desc[2048];
        ZWCADGuard_FormatBacktrace(crash_frames, crash_frames_count, stack_desc,
                                   sizeof(stack_desc));
        safe_write_str(fd, stack_desc);
        safe_write_str(fd, "\n");
    }
    
    // 3. 用户操作路径 (Breadcrumbs)
    pthread_mutex_lock(&g_breadCrumbs.mutex);
    safe_write_str(fd, "[User Path Breadcrumbs] (Last 50 operations):\n");
    if (g_breadCrumbs.count == 0) {
        safe_write_str(fd, "  No operations recorded.\n");
    } else {
        int idx = g_breadCrumbs.head;
        for (int i = 0; i < g_breadCrumbs.count; ++i) {
            BreadCrumb *b = &g_breadCrumbs.items[idx];
            safe_write_str(fd, "  [");
            safe_write_str(fd, b->time_str);
            safe_write_str(fd, "] [");
            safe_write_str(fd, b->category);
            safe_write_str(fd, "] ");
            safe_write_str(fd, b->action);
            if (strlen(b->details) > 0) {
                safe_write_str(fd, " (");
                safe_write_str(fd, b->details);
                safe_write_str(fd, ")");
            }
            safe_write_str(fd, "\n");
            idx = (idx + 1) % MAX_BREADCRUMBS;
        }
    }
    pthread_mutex_unlock(&g_breadCrumbs.mutex);
    safe_write_str(fd, "\n");
    
    safe_write_str(fd, "========================================\n");
}

// 生成崩溃报告文件名并写入目录
static void dump_to_file(const char *crash_type, const char *reason,
                         void **crash_frames, int crash_frames_count) {
    if (strlen(g_sdk_config.log_directory) == 0) {
        return;
    }
    
    char file_path[768];
    snprintf(file_path, sizeof(file_path), "%s/crash_%ld.log",
             g_sdk_config.log_directory, (long)time(NULL));
    
    int fd = open(file_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd >= 0) {
        write_crash_report(fd, crash_type, reason, crash_frames,
                           crash_frames_count);
        close(fd);
    }
}

// 动态链接符号声明，用于获取当前抛出异常的类型
extern "C" std::type_info *__cxa_current_exception_type();

// --- Terminate 拦截器 (C++ 级未捕获异常句柄) ---
static void ZWCADGuard_TerminateHandler(void) {
    void *crash_frames[MAX_STACK_FRAMES];
    int crash_frames_count =
    ZWCADGuard_CaptureBacktrace(crash_frames, MAX_STACK_FRAMES);
    
    char reason_buf[256] = {0};
    const char *exception_type_name = "Unknown C++ Exception";
    
    // 利用 __cxa_current_exception_type() 动态获取异常类型
    std::type_info *tinfo = __cxa_current_exception_type();
    if (tinfo && tinfo->name()) {
        char *demangled = ZWCADGuard_Demangle(tinfo->name());
        if (demangled) {
            strncpy(reason_buf, demangled, sizeof(reason_buf) - 1);
            free(demangled);
        } else {
            strncpy(reason_buf, tinfo->name(), sizeof(reason_buf) - 1);
        }
        exception_type_name = reason_buf;
    }
    
    // 尝试重抛异常以截获 what() 信息
    char message_buf[512] = {0};
    try {
        if (std::current_exception()) {
            std::rethrow_exception(std::current_exception());
        }
    } catch (const std::exception &e) {
        strncpy(message_buf, e.what(), sizeof(message_buf) - 1);
    } catch (const char *str) {
        strncpy(message_buf, str, sizeof(message_buf) - 1);
    } catch (...) {
        strcpy(message_buf, "Non-standard C++ exception thrown");
    }
    
    // 拼接最终的原因
    char final_reason[1024];
    snprintf(final_reason, sizeof(final_reason), "Type: %s, Message: %s",
             exception_type_name, strlen(message_buf) > 0 ? message_buf : "N/A");
    
    // 落盘写入
    dump_to_file("C++ Uncaught Exception (std::terminate)", final_reason,
                 crash_frames, crash_frames_count);
    
    // 转发给原本的 terminate 处理器
    if (g_sdk_config.original_terminate_handler) {
        g_sdk_config.original_terminate_handler();
    } else {
        abort();
    }
}

// --- POSIX 信号拦截器 (操作系统底层硬件/系统级崩溃) ---
static void ZWCADGuard_SignalHandler(int sig, siginfo_t *info, void *context) {
    void *crash_frames[MAX_STACK_FRAMES];
    int crash_frames_count =
    ZWCADGuard_CaptureBacktrace(crash_frames, MAX_STACK_FRAMES);
    
    // 构造崩溃原因
    char reason[128];
    snprintf(reason, sizeof(reason), "Signal %d (%s) at address %p", sig,
             strsignal(sig), info ? info->si_addr : nullptr);
    
    // 写入文件 (注意：在信号处理函数里调用 dump_to_file 属于安全子集)
    dump_to_file("OS POSIX Signal Crash", reason, crash_frames,
                 crash_frames_count);
    
    // 恢复系统默认或原本的信号处理
    struct sigaction original_action = g_sdk_config.original_sigactions[sig];
    if (original_action.sa_handler == SIG_DFL) {
        signal(sig, SIG_DFL);
        raise(sig);
    } else if (original_action.sa_handler != SIG_IGN) {
        if (original_action.sa_flags & SA_SIGINFO) {
            original_action.sa_sigaction(sig, info, context);
        } else {
            original_action.sa_handler(sig);
        }
    }
}

// --- SDK 对外 C API 实现 ---

extern "C" int ZWCADGuard_Init(const char *log_dir) {
    if (g_sdk_config.is_initialized) {
        return 0;
    }
    
    if (log_dir == nullptr || strlen(log_dir) == 0) {
        return -1;
    }
    
    strncpy(g_sdk_config.log_directory, log_dir,
            sizeof(g_sdk_config.log_directory) - 1);
    
    // 创建日志目录
#ifdef __APPLE__
    mkdir(g_sdk_config.log_directory, 0755);
#else
    mkdir(g_sdk_config.log_directory, 0755);
#endif
    
    // 1. 设置 C++ std::terminate 拦截器
    g_sdk_config.original_terminate_handler =
    std::set_terminate(ZWCADGuard_TerminateHandler);
    
    // 2. 注册 POSIX 信号处理器，对 SIGSEGV, SIGABRT 等硬件/物理崩溃进行兜底
    int signals_to_catch[] = {SIGSEGV, SIGABRT, SIGILL, SIGFPE, SIGBUS};
    int num_signals = sizeof(signals_to_catch) / sizeof(signals_to_catch[0]);
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = ZWCADGuard_SignalHandler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK; // 使用备用信号栈防栈溢出
    sigemptyset(&sa.sa_mask);
    
    for (int i = 0; i < num_signals; ++i) {
        int sig = signals_to_catch[i];
        sigaction(sig, &sa, &g_sdk_config.original_sigactions[sig]);
    }
    
    g_sdk_config.is_initialized = true;
    return 0;
}

extern "C" void ZWCADGuard_SetActiveDrawing(const char *name, const char *path,
                                            long size, const char *hash) {
    ZWCADGuard_SetActiveDrawingContext(name, path, size, hash, nullptr, nullptr,
                                       nullptr);
}

extern "C" void ZWCADGuard_SetActiveDrawingContext(
                                                   const char *name, const char *path, long size, const char *hash,
                                                   const char *file_id, const char *project_id, const char *project_name) {
    pthread_mutex_lock(&g_active_drawing.mutex);
    memset(g_active_drawing.name, 0, sizeof(g_active_drawing.name));
    memset(g_active_drawing.path, 0, sizeof(g_active_drawing.path));
    memset(g_active_drawing.hash, 0, sizeof(g_active_drawing.hash));
    memset(g_active_drawing.file_id, 0, sizeof(g_active_drawing.file_id));
    memset(g_active_drawing.project_id, 0, sizeof(g_active_drawing.project_id));
    memset(g_active_drawing.project_name, 0,
           sizeof(g_active_drawing.project_name));
    if (name)
        strncpy(g_active_drawing.name, name, sizeof(g_active_drawing.name) - 1);
    if (path)
        strncpy(g_active_drawing.path, path, sizeof(g_active_drawing.path) - 1);
    g_active_drawing.size = size;
    if (hash)
        strncpy(g_active_drawing.hash, hash, sizeof(g_active_drawing.hash) - 1);
    if (file_id)
        strncpy(g_active_drawing.file_id, file_id,
                sizeof(g_active_drawing.file_id) - 1);
    if (project_id)
        strncpy(g_active_drawing.project_id, project_id,
                sizeof(g_active_drawing.project_id) - 1);
    if (project_name)
        strncpy(g_active_drawing.project_name, project_name,
                sizeof(g_active_drawing.project_name) - 1);
    g_active_drawing.is_active = true;
    pthread_mutex_unlock(&g_active_drawing.mutex);
}

extern "C" void ZWCADGuard_ClearActiveDrawing(void) {
    pthread_mutex_lock(&g_active_drawing.mutex);
    memset(g_active_drawing.name, 0, sizeof(g_active_drawing.name));
    memset(g_active_drawing.path, 0, sizeof(g_active_drawing.path));
    memset(g_active_drawing.hash, 0, sizeof(g_active_drawing.hash));
    memset(g_active_drawing.file_id, 0, sizeof(g_active_drawing.file_id));
    memset(g_active_drawing.project_id, 0, sizeof(g_active_drawing.project_id));
    memset(g_active_drawing.project_name, 0,
           sizeof(g_active_drawing.project_name));
    g_active_drawing.size = 0;
    g_active_drawing.is_active = false;
    pthread_mutex_unlock(&g_active_drawing.mutex);
}

extern "C" void ZWCADGuard_AddBreadcrumb(const char *category,
                                         const char *action,
                                         const char *details) {
    pthread_mutex_lock(&g_breadCrumbs.mutex);
    // 环形缓存操作路径
    // 从头开始存储，存满时覆盖起始位置，head记录最老路径的位置
    int index = (g_breadCrumbs.head + g_breadCrumbs.count) % MAX_BREADCRUMBS;
    if (g_breadCrumbs.count == MAX_BREADCRUMBS) {
        g_breadCrumbs.head = (g_breadCrumbs.head + 1) % MAX_BREADCRUMBS; // 覆盖最老记录
    } else {
        g_breadCrumbs.count++;
    }
    
    BreadCrumb *b = &g_breadCrumbs.items[index];
    
    // 获取时间戳并转换为本地时间
    time_t timeInterval;
    time(&timeInterval);
    struct tm *timeInfo = localtime(&timeInterval);
    if (timeInfo) {
        strftime(b->time_str, sizeof(b->time_str), "%Y-%m-%d %H:%M:%S", timeInfo);
    } else {
        strcpy(b->time_str, "0000-00-00 00:00:00");
    }
    // 设置类别、操作、详细信息
    strncpy(b->category, category ? category : "NULL", sizeof(b->category) - 1);
    strncpy(b->action, action ? action : "NULL", sizeof(b->action) - 1);
    strncpy(b->details, details ? details : "", sizeof(b->details) - 1);
    
    pthread_mutex_unlock(&g_breadCrumbs.mutex);
}

extern "C" void ZWCADGuard_SimulateCrashDump(const char *message) {
    void *crash_frames[MAX_STACK_FRAMES];
    int crash_frames_count =
    ZWCADGuard_CaptureBacktrace(crash_frames, MAX_STACK_FRAMES);
    dump_to_file("Manual Simulated Crash (Testing)", message, crash_frames,
                 crash_frames_count);
}

extern "C" void ZWCADGuard_RecordObjCCrash(const char *name, const char *reason,
                                           void **frames, int count) {
    char final_reason[512];
    snprintf(final_reason, sizeof(final_reason), "Name: %s, Reason: %s",
             name ? name : "N/A", reason ? reason : "N/A");
    dump_to_file("Objective-C Uncaught Exception", final_reason, frames, count);
}

extern "C" void ZWCADGuard_RecordManagedException(const char *language,
                                                  const char *name,
                                                  const char *reason) {
    char final_reason[2048];
    snprintf(final_reason, sizeof(final_reason),
             "Language: %s, Name: %s, Reason: %s",
             language ? language : "Managed", name ? name : "N/A",
             reason ? reason : "N/A");
    dump_to_file("Managed Uncaught Exception", final_reason, nullptr, 0);
}
