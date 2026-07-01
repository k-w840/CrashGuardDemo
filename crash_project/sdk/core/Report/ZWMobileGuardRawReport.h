#ifndef ZWMOBILE_GUARD_RAW_REPORT_H
#define ZWMOBILE_GUARD_RAW_REPORT_H

#include <stddef.h>
#include <stdint.h>
#include <signal.h>

#include "ZWMobileGuardInternal.h"
#include "ZWMobileGuardState.h"

#define ZW_RAW_MAX_BINARY_IMAGES 128

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ZWRawCrashType {
    ZWRawCrashTypeSignal = 1,// POSIX signal
    ZWRawCrashTypeCPP = 2,// C++ 未捕获异常
    ZWRawCrashTypeObjC = 3,// OC 未捕获异常
    ZWRawCrashTypeManaged = 4,// java层未捕获异常
} ZWRawCrashType;

// app信息，用于关联版本、设备和包信息
struct ZWRawAppInfo {
    char processName[128];// 进程名
    char processPath[512];// 绝对路径
    char bundleId[128];// Bundle Identifier
    char appVersion[64];// 对外版本号
    char buildVersion[64];// 构建版本号
    char osVersion[128];// 系统版本
    char deviceModel[64];// 设备机型，例如 iPhone16,2
    char codeType[32];// 架构，如 ARM-64 / X86-64
    int pid;// 进程 pid
};

// 崩溃类型、数据元素个数
struct ZWRawCrashHeader {
    int crashType;// 崩溃类型
    uint64_t crashTimestamp;// 崩溃发生时的时间戳
    int signalNumber;//如 SIGSEGV / SIGABRT
    int signalCode;// 具体的异常原因（si_code）
    uint64_t faultAddress;// 崩溃地址
    uint32_t frameCount;// 栈帧数
    uint32_t imageCount;//  动态库数量
    uint32_t breadcrumbCount;// 操作路径条数
};

// 崩溃寄存器快照，(参照kscrash 报告格式，辅助分析崩溃上下文)
struct ZWRawArm64Registers {
    uint64_t x[29];// 通用寄存器 x0 ~ x28
    uint64_t fp;// Frame Pointer
    uint64_t lr;// Link Register / 返回地址寄存器
    uint64_t sp;// Stack Pointer
    uint64_t pc;// Program Counter，当前执行指令地址
    uint64_t cpsr;// Current Program Status Register
    uint64_t esr;// Exception Syndrome Register
    uint64_t far;// Fault Address Register
};

// 动态库信息，据此匹配 dSYM 符号文件还原堆栈
struct ZWRawBinaryImage {
    uint64_t base;// 基地址
    uint64_t size;
    char uuid[64];// UUID，匹配 dSYM 符号表的标识符
    char name[128];// 名称（如 studyGL）
    char path[512];// 路径后两级
    char arch[32];// 架构，例如 arm64 / x86_64
};

// 崩溃落盘数据
struct ZWRawCrashReport {
    ZWRawCrashHeader header;// 崩溃类型、数据元素个数
    ZWRawAppInfo appInfo;// app基础信息
    ZWRawArm64Registers registers;// 崩溃寄存器快照
    uint64_t frames[MAX_STACK_FRAMES];// 崩溃堆栈
    BreadCrumb breadcrumbs[MAX_BREADCRUMBS];// 操作路径
    ActiveDrawingInfo activeDrawing;// 关联的图纸信息
    char exceptionClass[256];// 异常类型名
    char exceptionReason[2048];// 异常详细原因
};

// 初始化基础 App 信息
void zwMobileGuardRawReportInit(const char *processName, const char *processPath, const char *bundleId, const char *appVersion, const char *buildVersion, const char *osVersion, const char *deviceModel);

// 刷新动态库信息，崩溃现场只读取
void zwMobileGuardRefreshBinaryImages(void);

// crash 报告构建、落盘接口
void zwMobileGuardWriteReportInternal(int crashType, int sig, siginfo_t *sigInfo, void *sigContext, const char *exceptionClass, const char *exceptionReason, void **frames, int frameCount);

#ifdef __cplusplus
}
#endif

#endif // ZWMOBILE_GUARD_RAW_REPORT_H
