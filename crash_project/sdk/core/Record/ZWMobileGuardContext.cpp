#include "ZWMobileGuardContext.h"
#include "ZWMobileGuard.h"
#include "ZWMobileGuardInternal.h"
#include "ZWMobileGuardState.h"
#include <pthread.h>
#include <string.h>
#include <time.h>

#pragma mark - 图纸相关
// 关联当前活跃图纸
extern "C" void zwMobileGuardSetActiveDrawingContext(
    const char *name, const char *path, long size, const char *hash,
    const char *fileId, const char *projectId, const char *projectName) {
    pthread_mutex_lock(&g_activeDrawing.mutex);
    // 绑定图纸时，清空历史数据
    memset(&g_activeDrawing.info, 0, sizeof(g_activeDrawing.info));
    if (name)
    {
        strncpy(g_activeDrawing.info.name, name, sizeof(g_activeDrawing.info.name) - 1);
    }
    if (path)
    {
        strncpy(g_activeDrawing.info.path, path, sizeof(g_activeDrawing.info.path) - 1);
    }
    if (hash)
    {
        strncpy(g_activeDrawing.info.hash, hash, sizeof(g_activeDrawing.info.hash) - 1);
    }
    if (fileId)
    {
        strncpy(g_activeDrawing.info.fileId, fileId, sizeof(g_activeDrawing.info.fileId) - 1);
    }
    if (projectId)
    {
        strncpy(g_activeDrawing.info.projectId, projectId, sizeof(g_activeDrawing.info.projectId) - 1);
    }
    if (projectName)
    {
        strncpy(g_activeDrawing.info.projectName, projectName, sizeof(g_activeDrawing.info.projectName) - 1);
    }
    g_activeDrawing.info.size = size;
    g_activeDrawing.info.isActive = true;
    // 同步快照数据
    g_activeDrawingSnapshot = g_activeDrawing.info;
    pthread_mutex_unlock(&g_activeDrawing.mutex);
}

// 清空/解除当前活跃图纸关联
extern "C" void zwMobileGuardClearActiveDrawing(void) {
  pthread_mutex_lock(&g_activeDrawing.mutex);
    // 清空活跃图纸数据
    memset(&g_activeDrawing.info, 0, sizeof(g_activeDrawing.info));
    // 同步快照数据
    g_activeDrawingSnapshot = g_activeDrawing.info;
  pthread_mutex_unlock(&g_activeDrawing.mutex);
}

#pragma mark - 操作路径
// 添加用户操作路径
extern "C" void zwMobileGuardAddBreadcrumb(const char *category, const char *action, const char *details) {
    pthread_mutex_lock(&g_breadCrumbs.mutex);
    // 环形缓存操作路径
    // 从头开始存储，存满时覆盖起始位置，head记录最老路径的位置
    BreadCrumbStore *store = &g_breadCrumbs.store;
    int index = (store->head + store->count) % MAX_BREADCRUMBS;
    if (store->count == MAX_BREADCRUMBS) {
        // 缓存已满，覆盖老数据，head指针前移
        store->head = (store->head + 1) % MAX_BREADCRUMBS;
    } else {
        // 存储没满，直接增加计数
        store->count++;
    }
    // 取出当前要写入位置的结构体
    BreadCrumb *b = &store->items[index];
    
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
    // 同步操作路径快照
    g_breadCrumbSnapshot = g_breadCrumbs.store;
    
    pthread_mutex_unlock(&g_breadCrumbs.mutex);
}
