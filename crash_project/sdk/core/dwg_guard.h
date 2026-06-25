#ifndef ZWMOBILE_GUARD_H
#define ZWMOBILE_GUARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * @brief 初始化 ZWMobileGuard SDK
 *
 * @param log_dir 崩溃日志落盘的绝对路径
 * @return int 0表示成功，非0表示失败
 */
int zwMobileGuardInit(const char* log_dir);


/// 设置活跃图纸信息
/// * @param name 图纸文件名
/// * @param path 图纸绝对路径
/// * @param size 图纸文件大小
/// * @param hash MD5 校验码
/// * @param file_id 文件ID
/// * @param project_id 项目ID
/// * @param project_name 项目名
void zwMobileGuardSetActiveDrawingContext(const char* name,
                                        const char* path,
                                        long size,
                                        const char* hash,
                                        const char* file_id,
                                        const char* project_id,
                                        const char* project_name);

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
 * @param mangled_name 混淆过的符号名 (例如 "_ZNSt12length_errorD1Ev")
 * @return char* 还原后的符号名。注意：必须调用 free() 释放返回的内存。
 */
char* zwMobileGuardDemangle(const char* mangled_name);

/**
 * @brief 手动触发模拟崩溃写入（用于测试落盘和回溯功能）
 *
 * @param message 崩溃附加文本信息
 */
void zwMobileGuardSimulateCrashDump(const char* message);

/**
 * @brief 手动记录 Objective-C 等上层捕获到的未捕获异常
 */




/// 记录OC异常
/// @param name 异常名称
/// @param reason 异常原因
/// @param frames 调用栈帧
/// @param count 栈帧数
void zwMobileGuardRecordObjCCrash(const char* name, const char* reason, void** frames, int count);

/**
 * @brief 手动记录 Java / Kotlin 等托管层未捕获异常
 *
 * Android 侧默认未捕获异常处理器可调用该接口，将 Java 异常类型、消息和格式化后的
 * 调用栈文本落盘，和 native/C++ 崩溃报告走同一套本地文件管理链路。
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
