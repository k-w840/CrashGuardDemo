#include "ZWMobileGuard.h"
#include "ZWMobileGuardState.h"
#include "ZWMobileGuardPOSIX.h"
#include "ZWMobileGuardThrow.h"
#include <string.h>
#include <sys/stat.h>

// 初始化
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
