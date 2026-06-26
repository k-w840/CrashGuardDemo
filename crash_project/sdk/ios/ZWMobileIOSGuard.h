#import <Foundation/Foundation.h>

@interface ZWMobileIOSGuard : NSObject
// 崩溃日志文件路径
@property(nonatomic, copy, readonly) NSString *logDir;

/**
 * 获取 ZWMobileIOSGuard 单例
 */
+ (instancetype)sharedInstance;

/**
 * 初始化 ZWMobileIOSGuard，自动捕获 C++ 未捕获异常、POSIX 信号、 OC 未捕获异常
 */
- (BOOL)initializeSDK;


/**
 * 关联活跃图纸信息
 * 操作路径可能过长，导致图纸信息被覆盖，因此图纸信息单独关联
 *
 * @param name 图纸文件名
 * @param path 绝对路径
 * @param size 文件大小
 * @param hash MD5 校验码
 * @param fileID 文件ID
 * @param projectID 项目ID
 * @param projectName 项目名
 */
- (void)setActiveDrawingName:(NSString *)name
                        path:(NSString *)path
                        size:(long)size
                        hash:(NSString *)hash
                      fileID:(NSString *)fileID
                   projectID:(NSString *)projectID
                 projectName:(NSString *)projectName;

/**
 * 清除关联的活跃图纸信息
 */
- (void)clearActiveDrawing;

/**
 * 记录用户操作埋点
 *
 * @param category 事件类别 (如 UI, Draw, View)
 * @param action 具体操作 (如 ClickButton, Zoom, DrawCircle)
 * @param details 详情/附加参数 (如 zoom_scale=1.2)
 */
- (void)addBreadcrumbCategory:(NSString *)category
                       action:(NSString *)action
                      details:(NSString *)details;

/**
 * 获取本地所有崩溃日志文件路径列表
 */
- (NSArray<NSString *> *)getCrashLogPaths;

/**
 * 读取特定崩溃日志文件的文本内容
 * 
 * @param path 崩溃日志文件的绝对路径
 */
- (NSString *)readCrashLogContentAtPath:(NSString *)path;

/**
 * 模拟上传崩溃日志与图纸本体
 * 
 * @param logPath 崩溃日志路径
 * @param uploadDrawing 是否随同上传图纸本体
 * @param completion 模拟上传完成回调 (success=是否成功, message=状态描述)
 */
- (void)uploadCrashLogAtPath:(NSString *)logPath
               uploadDrawing:(BOOL)uploadDrawing
                  completion:(void (^)(BOOL success, NSString *message))completion;

/**
 * 删除本地崩溃日志文件
 */
- (BOOL)deleteCrashLogAtPath:(NSString *)path;

@end
