#ifndef ZWCAD_GUARD_H
#define ZWCAD_GUARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * @brief 初始化 ZWCADGuard SDK
 *
 * @param log_dir 崩溃日志落盘的绝对路径
 * @return int 0表示成功，非0表示失败
 */
int ZWCADGuard_Init(const char* log_dir);

/**
 * @brief 设置当前处于活跃状态的图纸元数据（用于在崩溃时进行图纸关联）
 *
 * @param name 图纸文件名 (例如 "floor_plan.dwg")
 * @param path 图纸绝对路径
 * @param size 图纸文件大小 (字节)
 * @param hash 图纸文件MD5哈希或唯一ID
 */
void ZWCADGuard_SetActiveDrawing(const char* name, const char* path, long size, const char* hash);

/**
 * @brief 设置当前活跃图纸及所属项目上下文
 *
 * 该接口用于贴近 zwProject / 外网365 的真实业务场景，允许在崩溃报告中同时保留
 * 文件与项目维度的上下文信息。
 *
 * @param name 图纸文件名
 * @param path 图纸绝对路径
 * @param size 图纸文件大小 (字节)
 * @param hash 图纸文件哈希或唯一指纹
 * @param file_id 文件唯一ID，可为空
 * @param project_id 项目唯一ID，可为空
 * @param project_name 项目名称，可为空
 */
void ZWCADGuard_SetActiveDrawingContext(const char* name,
                                        const char* path,
                                        long size,
                                        const char* hash,
                                        const char* file_id,
                                        const char* project_id,
                                        const char* project_name);

/**
 * @brief 清除当前活跃状态的图纸（在关闭图纸时调用）
 */
void ZWCADGuard_ClearActiveDrawing(void);

/**
 * @brief 添加一条用户操作面包屑路径（记录在内存环形队列中，仅在崩溃时随报告一起落盘）
 *
 * @param category 分类 (例如 "UI", "Draw", "View")
 * @param action 操作名称 (例如 "ClickButton", "Zoom", "DrawCircle")
 * @param details 详情/附带属性 (例如 "zoom_scale=1.5" 或 "button_id=btn_save")
 */
void ZWCADGuard_AddBreadcrumb(const char* category, const char* action, const char* details);

/**
 * @brief 还原C++符号混淆（Demangle）
 *
 * @param mangled_name 混淆过的符号名 (例如 "_ZNSt12length_errorD1Ev")
 * @return char* 还原后的符号名。注意：必须调用 free() 释放返回的内存。
 */
char* ZWCADGuard_Demangle(const char* mangled_name);

/**
 * @brief 手动触发模拟崩溃写入（用于测试落盘和回溯功能）
 *
 * @param message 崩溃附加文本信息
 */
void ZWCADGuard_SimulateCrashDump(const char* message);

/**
 * @brief 手动记录 Objective-C 等上层捕获到的未捕获异常
 */




/// 记录OC异常
/// @param name 异常名称
/// @param reason 异常原因
/// @param frames 调用栈帧
/// @param count 栈帧数
void ZWCADGuard_RecordObjCCrash(const char* name, const char* reason, void** frames, int count);

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
void ZWCADGuard_RecordManagedException(const char* language, const char* name, const char* reason);

#ifdef __cplusplus
}
#endif

#endif // ZWCAD_GUARD_H
