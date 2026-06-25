#include "dwg_guard.h"
#include "dwg_guard_backtrace.h"
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
static thread_local ThreadExceptionInfo g_thread_exception = {{0}, 0, {0}, false};

// 外部声明，用于在 hook 中填充 TLS 栈
extern "C" void zwMobileGuardRecordThrowState(void *thrown_exception, std::type_info *tinfo) {
    g_thread_exception.frames_count = zwMobileGuardCaptureBacktrace(g_thread_exception.backtrace_buffer, MAX_STACK_FRAMES);
    g_thread_exception.has_exception = true;
    if (tinfo && tinfo->name()) {
        char *demangled = zwMobileGuardDemangle(tinfo->name());
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

// 操作路径缓存数据
struct BreadCrumb {
    char time_str[32]; // 时间
    char category[64]; // 事件类别
    char action[128];  // 具体操作
    char details[256]; // 详情
};

static struct {
    BreadCrumb items[MAX_BREADCRUMBS];
    int head;
    int count;
    pthread_mutex_t mutex;
} g_breadCrumbs = {{{0}}, 0, 0, PTHREAD_MUTEX_INITIALIZER};

// 图纸信息
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
} g_activeDrawing = {{0}, {0}, 0, {0}, {0}, {0}, {0}, false, PTHREAD_MUTEX_INITIALIZER};

#pragma mark - SDK全局配置
static struct {
    // 崩溃日志存储路径
    char logDirectory[512];
    // 自定义 std::set_terminate 处理
    std::terminate_handler originalTerminateHandler;
    // 系统原信号量处理
    struct sigaction originalSigactions[NSIG];
    // 标记SDK是否已初始化
    bool isInitialized;
} g_sdkConfig = {{0}, nullptr, {{0}}, false};

#pragma mark - 辅助处理函数
// 信号处理函数中不能使用 printf/sprintf/malloc，要使用底层安全调用
// 向打开的文件描述符中写入指定数据
static void safeWriteStr(int fd, const char *str) {
    if (str) {
        write(fd, str, strlen(str));
    }
}

// 10/16进制数字转为字符存入
static void safeWriteUint(int fd, uintptr_t val, int base) {
    // 记录数字
    char buf[32];
    char temp[32];
    int i = 0;
    if (val == 0) {
        temp[i++] = '0';
    } else {
        while (val > 0) {
            // 取余逆序获取数字值
            int rem = val % base;
            // 数字根据ASCII表转为字符（16进制有a、b、c）
            temp[i++] = (rem < 10) ? (rem + '0') : (rem - 10 + 'a');
            val /= base;
        }
    }
    // 转为正序
    int j = 0;
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    // 数字字符串结尾补\0
    buf[j] = '\0';
    write(fd, buf, j);
}

//写入16进制数字
static void safeWriteHex(int fd, uintptr_t val) {
    safeWriteStr(fd, "0x");
    safeWriteUint(fd, val, 16);
}

// 写入10进制数字
static void safeWriteDec(int fd, long val) {
    // 负数先写入符号
    if (val < 0) {
        safeWriteStr(fd, "-");
        val = -val;
    }
    // 转为无符号类型的正数写入
    safeWriteUint(fd, (uintptr_t)val, 10);
}

// 清空活跃图纸数据
static void clear_active_drawing_internal(void) {
    memset(g_activeDrawing.name, 0, sizeof(g_activeDrawing.name));
    memset(g_activeDrawing.path, 0, sizeof(g_activeDrawing.path));
    memset(g_activeDrawing.hash, 0, sizeof(g_activeDrawing.hash));
    memset(g_activeDrawing.file_id, 0, sizeof(g_activeDrawing.file_id));
    memset(g_activeDrawing.project_id, 0, sizeof(g_activeDrawing.project_id));
    memset(g_activeDrawing.project_name, 0,
           sizeof(g_activeDrawing.project_name));
    g_activeDrawing.size = 0;
    g_activeDrawing.is_active = false;
}

#pragma mark - 崩溃日志落盘
static void writeCrashReport(int fd, const char *crash_type, const char *reason, void **crash_frames, int crash_frames_count) {
    safeWriteStr(fd, "========================================\n");
    safeWriteStr(fd, "          ZWMOBILE CRASH REPORT       \n");
    safeWriteStr(fd, "========================================\n\n");
    
    // 崩溃类型与原因
    safeWriteStr(fd, "[Crash Type]: ");
    safeWriteStr(fd, crash_type);
    safeWriteStr(fd, "\n");
    
    safeWriteStr(fd, "[Reason]: ");
    safeWriteStr(fd, reason ? reason : "N/A");
    safeWriteStr(fd, "\n\n");
    
    // 图纸元数据 (仅记录脱敏后的标识，符合合规红线)
    pthread_mutex_lock(&g_activeDrawing.mutex);
    safeWriteStr(fd, "[Active Drawing Information]:\n");
    if (g_activeDrawing.is_active) {
        safeWriteStr(fd, "  File Name: ");
        safeWriteStr(fd, g_activeDrawing.name);
        safeWriteStr(fd, "\n  File ID: ");
        safeWriteStr(fd, strlen(g_activeDrawing.file_id) > 0
                       ? g_activeDrawing.file_id
                       : "N/A");
        safeWriteStr(fd, "\n  Project ID: ");
        safeWriteStr(fd, strlen(g_activeDrawing.project_id) > 0
                       ? g_activeDrawing.project_id
                       : "N/A");
        safeWriteStr(fd, "\n  Project Name: ");
        safeWriteStr(fd, strlen(g_activeDrawing.project_name) > 0
                       ? g_activeDrawing.project_name
                       : "N/A");
        safeWriteStr(fd, "\n  File Size: ");
        safeWriteDec(fd, g_activeDrawing.size);
        safeWriteStr(fd, " bytes\n  File MD5/Hash: ");
        safeWriteStr(fd, g_activeDrawing.hash);
        safeWriteStr(fd, "\n  File Path (Raw): ");
        safeWriteStr(fd, g_activeDrawing.path);
        safeWriteStr(fd, "\n");
    } else {
        safeWriteStr(fd, "  No active drawing opened during crash.\n");
    }
    pthread_mutex_unlock(&g_activeDrawing.mutex);
    safeWriteStr(fd, "\n");
    
    // 1. 原始 throw 堆栈（如果有）
    if (g_thread_exception.has_exception) {
        safeWriteStr(fd,
                       "[Original Throw Stack Trace] (Captured at __cxa_throw):\n");
        safeWriteStr(fd, "  Exception Class: ");
        safeWriteStr(fd, g_thread_exception.exception_type);
        safeWriteStr(fd, "\n");
        
        char stack_desc[2048];
        zwMobileGuardFormatBacktrace(g_thread_exception.backtrace_buffer,
                                     g_thread_exception.frames_count, stack_desc,
                                     sizeof(stack_desc));
        safeWriteStr(fd, stack_desc);
        safeWriteStr(fd, "\n");
    }
    
    // 2. 当前崩溃现场堆栈
    if (crash_frames && crash_frames_count > 0) {
        safeWriteStr(fd, "[Termination Stack Trace] (At crash/signal site):\n");
        char stack_desc[2048];
        zwMobileGuardFormatBacktrace(crash_frames, crash_frames_count, stack_desc,
                                     sizeof(stack_desc));
        safeWriteStr(fd, stack_desc);
        safeWriteStr(fd, "\n");
    }
    
    // 3. 用户操作路径 (Breadcrumbs)
    pthread_mutex_lock(&g_breadCrumbs.mutex);
    safeWriteStr(fd, "[User Path Breadcrumbs] (Last 50 operations):\n");
    if (g_breadCrumbs.count == 0) {
        safeWriteStr(fd, "  No operations recorded.\n");
    } else {
        int idx = g_breadCrumbs.head;
        for (int i = 0; i < g_breadCrumbs.count; ++i) {
            BreadCrumb *b = &g_breadCrumbs.items[idx];
            safeWriteStr(fd, "  [");
            safeWriteStr(fd, b->time_str);
            safeWriteStr(fd, "] [");
            safeWriteStr(fd, b->category);
            safeWriteStr(fd, "] ");
            safeWriteStr(fd, b->action);
            if (strlen(b->details) > 0) {
                safeWriteStr(fd, " (");
                safeWriteStr(fd, b->details);
                safeWriteStr(fd, ")");
            }
            safeWriteStr(fd, "\n");
            idx = (idx + 1) % MAX_BREADCRUMBS;
        }
    }
    pthread_mutex_unlock(&g_breadCrumbs.mutex);
    safeWriteStr(fd, "\n");
    
    safeWriteStr(fd, "========================================\n");
}

// 生成崩溃报告文件名 && 写入
static void dumpToFile(const char *crashType, const char *reason, void **crashFrames, int crashFramesCount) {
    if (strlen(g_sdkConfig.logDirectory) <= 0) {
        return;
    }
    
    // 生成崩溃报告文件名，以时间戳为文件名一部分
    char filePath[768];
    snprintf(filePath, sizeof(filePath), "%s/crash_%ld.log", g_sdkConfig.logDirectory, (long)time(NULL));
    
    // 写入文件
    int fd = open(filePath, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd >= 0) {
        writeCrashReport(fd, crashType, reason, crashFrames, crashFramesCount);
        close(fd);
    }
}

#pragma mark - 自定义崩溃捕获拦截处理
// 动态链接符号声明，用于获取当前抛出异常的类型
extern "C" std::type_info *__cxa_current_exception_type();

// Terminate 崩溃捕获拦截处理
static void zwMobileGuardTerminateHandler(void) {
    void *crash_frames[MAX_STACK_FRAMES];
    int crash_frames_count =
    zwMobileGuardCaptureBacktrace(crash_frames, MAX_STACK_FRAMES);
    
    char reason_buf[256] = {0};
    const char *exception_type_name = "Unknown C++ Exception";
    
    // 利用 __cxa_current_exception_type() 动态获取异常类型
    std::type_info *tinfo = __cxa_current_exception_type();
    if (tinfo && tinfo->name()) {
        char *demangled = zwMobileGuardDemangle(tinfo->name());
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
    dumpToFile("C++ Uncaught Exception (std::terminate)", final_reason,
                 crash_frames, crash_frames_count);
    
    // 转发给原本的 terminate 处理器
    if (g_sdkConfig.originalTerminateHandler) {
        g_sdkConfig.originalTerminateHandler();
    } else {
        abort();
    }
}

// POSIX 崩溃捕获拦截处理
static void zwMobileGuardSignalHandler(int sig, siginfo_t *info, void *context) {
    // 获取堆栈
    void *crashFrames[MAX_STACK_FRAMES];
    int crashFramesCount = zwMobileGuardCaptureBacktrace(crashFrames, MAX_STACK_FRAMES);
    
    // 构造崩溃原因
    char reason[128];
    snprintf(reason, sizeof(reason), "Signal %d (%s) at address %p", sig,
             strsignal(sig), info ? info->si_addr : nullptr);
    
    // 写入文件
    dumpToFile("OS POSIX Signal Crash", reason, crashFrames, crashFramesCount);
    
    // 恢复原本的信号处理
    struct sigaction originalAction = g_sdkConfig.originalSigactions[sig];
    // 默认信号处理
    if (originalAction.sa_handler == SIG_DFL) {
        signal(sig, SIG_DFL);
        raise(sig);
    } else if (originalAction.sa_handler != SIG_IGN) {
        // 有自定义处理，根据参数类型 返回自定义处理
        if (originalAction.sa_flags & SA_SIGINFO) {
            originalAction.sa_sigaction(sig, info, context);
        } else {
            originalAction.sa_handler(sig);
        }
    }
}

#pragma mark - 线程备用信号栈
static pthread_key_t g_alternateStackKey;
static pthread_once_t g_alternateStackKeyOnce = PTHREAD_ONCE_INIT;

// 线程退出时，释放备用栈
static void destroyAlternateStack(void *ptr) {
    if (ptr) {
        stack_t altStack;
        altStack.ss_sp = nullptr;
        altStack.ss_size = 0;
        altStack.ss_flags = SS_DISABLE;
        sigaltstack(&altStack, nullptr);
        free(ptr);
    }
}

// 备用栈初始化回掉函数
static void alternateStackInitFunc() {
    // 线程安全执行析构
    pthread_key_create(&g_alternateStackKey, destroyAlternateStack);
}

#pragma mark - 对外 C API 实现
// 初始化 注册 std::terminate、POSIX
extern "C" int zwMobileGuardInit(const char *logDir) {
    if (g_sdkConfig.isInitialized) {
        return 0;
    }
    
    if (logDir == nullptr || strlen(logDir) <= 0) {
        return -1;
    }
    
    strncpy(g_sdkConfig.logDirectory, logDir, sizeof(g_sdkConfig.logDirectory) - 1);
    // 创建日志目录
    mkdir(g_sdkConfig.logDirectory, 0755);
    
    // 设置 std::terminate 拦截器
    g_sdkConfig.originalTerminateHandler = std::set_terminate(zwMobileGuardTerminateHandler);
    
    // 注册 POSIX 信号处理器
    // 只处理这五个即可，其余信号为系统层面不能捕获，或者是用户主动操作的正常信号
    int signalsToCatch[] = {SIGSEGV, SIGABRT, SIGILL, SIGFPE, SIGBUS};
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    // 设置信号崩溃时处理函数
    sa.sa_sigaction = zwMobileGuardSignalHandler;
    // 使用三参数崩溃信号函数，以便能获取物理地址
    // 使用备用信号栈防栈溢出
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    
    for (int i = 0; i < 5; ++i) {
        int sig = signalsToCatch[i];
        // 设置崩溃信号自定义处理器，并存储原处理
        sigaction(sig, &sa, &g_sdkConfig.originalSigactions[sig]);
    }
    
    // 默认为主线程/当前线程注册备用信号栈
    zwMobileGuardRegisterThreadSignalStack();
    
    g_sdkConfig.isInitialized = true;
    return 0;
}

// 注册备用信号栈
extern "C" void zwMobileGuardRegisterThreadSignalStack(void) {
    // 线程安全防护 && 只执行一次
    pthread_once(&g_alternateStackKeyOnce, alternateStackInitFunc);
    // g_alternateStackKey 保证线程安全，每个线程只有一个备用栈
    if (pthread_getspecific(g_alternateStackKey)) {
        return;
    }
    
    // 备用栈大小为默认大小
    void *stackBase = malloc(SIGSTKSZ);
    if (!stackBase) {
        return;
    }
    
    stack_t alternateSingleStack;
    // 配置备用栈信息
    alternateSingleStack.ss_sp = stackBase;
    alternateSingleStack.ss_size = SIGSTKSZ;
    alternateSingleStack.ss_flags = 0;
    // 创建备用信号栈，当线程因栈溢出崩溃时，能捕获崩溃
    if (sigaltstack(&alternateSingleStack, nullptr) == 0) {
        pthread_setspecific(g_alternateStackKey, stackBase);
    } else {
        free(stackBase);
    }
}

// 关联当前活跃图纸
extern "C" void zwMobileGuardSetActiveDrawingContext(
    const char *name, const char *path, long size, const char *hash,
    const char *file_id, const char *project_id, const char *project_name) {
    pthread_mutex_lock(&g_activeDrawing.mutex);
    // 绑定图纸时，清空历史数据
    clear_active_drawing_internal();
    if (name)
    {
        strncpy(g_activeDrawing.name, name, sizeof(g_activeDrawing.name) - 1);
    }
    if (path)
    {
        strncpy(g_activeDrawing.path, path, sizeof(g_activeDrawing.path) - 1);
    }
    if (hash)
    {
        strncpy(g_activeDrawing.hash, hash, sizeof(g_activeDrawing.hash) - 1);
    }
    if (file_id)
    {
        strncpy(g_activeDrawing.file_id, file_id, sizeof(g_activeDrawing.file_id) - 1);
    }
    if (project_id)
    {
        strncpy(g_activeDrawing.project_id, project_id, sizeof(g_activeDrawing.project_id) - 1);
    }
    if (project_name)
    {
        strncpy(g_activeDrawing.project_name, project_name, sizeof(g_activeDrawing.project_name) - 1);
    }
    g_activeDrawing.size = size;
    g_activeDrawing.is_active = true;
    pthread_mutex_unlock(&g_activeDrawing.mutex);
}

// 清空/解除当前活跃图纸关联
extern "C" void zwMobileGuardClearActiveDrawing(void) {
  pthread_mutex_lock(&g_activeDrawing.mutex);
  clear_active_drawing_internal();
  pthread_mutex_unlock(&g_activeDrawing.mutex);
}

// 添加用户操作路径
extern "C" void zwMobileGuardAddBreadcrumb(const char *category, const char *action, const char *details) {
    pthread_mutex_lock(&g_breadCrumbs.mutex);
    // 环形缓存操作路径
    // 从头开始存储，存满时覆盖起始位置，head记录最老路径的位置
    int index = (g_breadCrumbs.head + g_breadCrumbs.count) % MAX_BREADCRUMBS;
    if (g_breadCrumbs.count == MAX_BREADCRUMBS) {
        // 缓存已满，覆盖老数据，head指针前移
        g_breadCrumbs.head = (g_breadCrumbs.head + 1) % MAX_BREADCRUMBS;
    } else {
        // 存储没满，直接增加计数
        g_breadCrumbs.count++;
    }
    // 取出当前要写入位置的结构体
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

// 模拟崩溃写入测试
extern "C" void zwMobileGuardSimulateCrashDump(const char *message) {
    void *crashFrames[MAX_STACK_FRAMES];
    // 获取调用栈
    int crashFramesCount = zwMobileGuardCaptureBacktrace(crashFrames, MAX_STACK_FRAMES);
    dumpToFile("Simulated Uncaught Exception", message, crashFrames, crashFramesCount);
}

// 记录OC异常到本地
extern "C" void zwMobileGuardRecordObjCCrash(const char *name, const char *reason, void **frames, int count) {
    char finalReason[512];
    snprintf(finalReason, sizeof(finalReason), "异常名称: %s, 异常原因: %s", name ? name : "NULL", reason ? reason : "NULL");
    dumpToFile("OC Uncaught Exception", finalReason, frames, count);
}

// 记录 Java 层未捕获异常
extern "C" void zwMobileGuardRecordManagedException(const char *language, const char *name, const char *reason) {
    char finalReason[2048];
    snprintf(finalReason, sizeof(finalReason), "语言类别: %s, 异常类名: %s, 异常描述或堆栈文本: %s", language ? language : "JAVA", name ? name : "NULL", reason ? reason : "NULL");
    // 托管异常(非原生异常)
    dumpToFile("Managed Uncaught Exception", finalReason, nullptr, 0);
}
