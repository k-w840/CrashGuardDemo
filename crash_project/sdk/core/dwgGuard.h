#ifndef ZWMOBILE_GUARD_H
#define ZWMOBILE_GUARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * @brief 初始化 ZWMobileGuard SDK
 *
 * @param logDir 崩溃日志落盘的绝对路径
 * @return int 0表示成功，非0表示失败
 */
int zwMobileGuardInit(const char* logDir);

/**
 *  注册当前线程的备用信号栈以处理拦截栈溢出崩溃
 *  默认会为主线程/当前线程注册。对于多线程项目，可在子线程入口函数中调用。
 */
void zwMobileGuardRegisterThreadSignalStack(void);


/// 设置活跃图纸信息
/// * @param name 图纸文件名
/// * @param path 图纸绝对路径
/// * @param size 图纸文件大小
/// * @param hash MD5 校验码
/// * @param fileId 文件ID
/// * @param projectId 项目ID
/// * @param projectName 项目名
void zwMobileGuardSetActiveDrawingContext(const char* name,
                                        const char* path,
                                        long size,
                                        const char* hash,
                                        const char* fileId,
                                        const char* projectId,
                                        const char* projectName);

/**
 * 清除关联的活跃图纸信息
 */
void zwMobileGuardClearActiveDrawing(void);

/**
 * 添加用户操作路径
 *
 * @param category 分类 (例如 "UI", "Draw", "View")
 * @param action 操作名称 (例如 "ClickButton", "Zoom", "DrawCircle")
 * @param details 详情/附带属性 (例如 "zoom_scale=1.5" 或 "button_id=btn_save")
 */
void zwMobileGuardAddBreadcrumb(const char* category, const char* action, const char* details);

/**
 * @brief 还原C++符号混淆（Demangle）
 *
 * @param mangledName 混淆过的符号名 (例如 "_ZNSt12length_errorD1Ev")
 * @return char* 还原后的符号名。注意：必须调用 free() 释放返回的内存。
 */
char* zwMobileGuardDemangle(const char* mangledName);

/**
 * 模拟崩溃写入测试
 *
 * @param message 自定义崩溃信息(异常原因)
 */
void zwMobileGuardSimulateCrashDump(const char* message);

/// 记录OC异常到本地
/// @param name 异常名称
/// @param reason 异常原因
/// @param frames 调用栈帧
/// @param count 栈帧数
void zwMobileGuardRecordObjCCrash(const char* name, const char* reason, void** frames, int count);

/**
 * 记录 Java/Kotlin 层未捕获异常
 * Android 侧默认未捕获异常处理器可调用该接口
 *
 * @param language 语言来源，例如 "Java" 或 "Kotlin"
 * @param name 异常类名
 * @param reason 异常描述或堆栈文本
 */
void zwMobileGuardRecordManagedException(const char* language, const char* name, const char* reason);

#ifdef __cplusplus
}
#endif

#endif // ZWMOBILE_GUARD_H
