#include "dwgGuard.h"
#include "dwgGuardState.h"
#include "dwgGuardPOSIX.h"
#include "dwgGuardThrow.h"
#include <string.h>
#include <sys/stat.h>

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
    
    // 初始化注册 throw/terminate
    zwMobileThrowInit();
    // 初始化注册信号量
    zwMobilePosixInit();
    
    g_sdkConfig.isInitialized = true;
    return 0;
}
