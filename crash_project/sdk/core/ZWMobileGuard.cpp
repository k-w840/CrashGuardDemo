#include "ZWMobileGuard.h"
#include "ZWMobileGuardState.h"
#include "ZWMobileGuardPOSIX.h"
#include "ZWMobileGuardThrow.h"
#include "ZWMobileGuardRawReport.h"
#include <string.h>
#include <sys/stat.h>

// 初始化
extern "C" int zwMobileGuardInit(const char *logDir,
                                  const char* processName,
                                  const char* processPath,
                                  const char* bundleId,
                                  const char* appVersion,
                                  const char* buildVersion,
                                  const char* osVersion,
                                  const char* deviceModel) {
    if (g_sdkConfig.isInitialized) {
        return 0;
    }
    
    if (logDir == nullptr || strlen(logDir) <= 0) {
        return -1;
    }
    
    strncpy(g_sdkConfig.logDirectory, logDir, sizeof(g_sdkConfig.logDirectory) - 1);
    // 创建日志目录
    mkdir(g_sdkConfig.logDirectory, 0755);

    // 初始化基础 App 信息
    zwMobileGuardRawReportInit(processName, processPath, bundleId, appVersion, buildVersion, osVersion, deviceModel);
    // 刷新动态库信息
    zwMobileGuardRefreshBinaryImages();
    
    // 初始化注册 throw/terminate
    zwMobileThrowInit();
    // 初始化注册信号量
    zwMobilePosixInit();
    
    g_sdkConfig.isInitialized = true;
    return 0;
}
