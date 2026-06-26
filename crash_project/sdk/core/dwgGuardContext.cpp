#include "dwgGuardContext.h"
#include "dwgGuard.h"
#include "dwgGuardInternal.h"
#include "dwgGuardState.h"
#include <pthread.h>
#include <string.h>
#include <time.h>

#pragma mark - 图纸相关
// 清空活跃图纸数据
static void clearActiveDrawingInternal(void) {
    memset(g_activeDrawing.name, 0, sizeof(g_activeDrawing.name));
    memset(g_activeDrawing.path, 0, sizeof(g_activeDrawing.path));
    memset(g_activeDrawing.hash, 0, sizeof(g_activeDrawing.hash));
    memset(g_activeDrawing.fileId, 0, sizeof(g_activeDrawing.fileId));
    memset(g_activeDrawing.projectId, 0, sizeof(g_activeDrawing.projectId));
    memset(g_activeDrawing.projectName, 0,
           sizeof(g_activeDrawing.projectName));
    g_activeDrawing.size = 0;
    g_activeDrawing.isActive = false;
}

// 关联当前活跃图纸
extern "C" void zwMobileGuardSetActiveDrawingContext(
    const char *name, const char *path, long size, const char *hash,
    const char *fileId, const char *projectId, const char *projectName) {
    pthread_mutex_lock(&g_activeDrawing.mutex);
    // 绑定图纸时，清空历史数据
    clearActiveDrawingInternal();
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
    if (fileId)
    {
        strncpy(g_activeDrawing.fileId, fileId, sizeof(g_activeDrawing.fileId) - 1);
    }
    if (projectId)
    {
        strncpy(g_activeDrawing.projectId, projectId, sizeof(g_activeDrawing.projectId) - 1);
    }
    if (projectName)
    {
        strncpy(g_activeDrawing.projectName, projectName, sizeof(g_activeDrawing.projectName) - 1);
    }
    g_activeDrawing.size = size;
    g_activeDrawing.isActive = true;
    pthread_mutex_unlock(&g_activeDrawing.mutex);
}

// 清空/解除当前活跃图纸关联
extern "C" void zwMobileGuardClearActiveDrawing(void) {
  pthread_mutex_lock(&g_activeDrawing.mutex);
    clearActiveDrawingInternal();
  pthread_mutex_unlock(&g_activeDrawing.mutex);
}

#pragma mark - 操作路径
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
