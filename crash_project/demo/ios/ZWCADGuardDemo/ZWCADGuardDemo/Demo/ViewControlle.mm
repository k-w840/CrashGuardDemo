#import "ViewController.h"
#import "ZWMobileGuard.h"
#import <Masonry/Masonry.h>
#include <signal.h>
#include <stdexcept>
#import "CrashDemoUtil.h"
#include <string>

#define CUSTOM_CELL_IDENTIFY @"customCellIdentify"

// --- C++ 异常测试用例的类定义 ---
class CustomCPPException {
public:
    std::string message;
    explicit CustomCPPException(const std::string& msg) : message(msg) {}
};

// 触发 C++ 析构函数双重抛出导致 terminate
struct DestructorDoubleThrower {
    ~DestructorDoubleThrower() noexcept(false) {
        throw std::runtime_error("Double throw inside destructor triggers std::terminate");
    }
};

@interface ViewController () <UITableViewDelegate, UITableViewDataSource>

@property (nonatomic, strong) UITableView *tableView;
@property (nonatomic, strong) NSArray<NSDictionary *> *pageDataArr;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    self.title = @"崩溃验证";
    self.pageDataArr = [CrashDemoUtil demoTestData];
    [self.view addSubview:self.tableView];
    [self.tableView mas_makeConstraints:^(MASConstraintMaker *make) {
        make.top.left.right.equalTo(self.view);
        make.bottom.equalTo(self.view).offset(-84.0);
    }];
}

#pragma mark - UI

- (void)showFirewallAlert:(NSString *)message {
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"安全防火墙拦截"
                                                                   message:message
                                                            preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"确定" style:UIAlertActionStyleDefault handler:nil]];
    [self presentViewController:alert animated:YES completion:nil];
}

#pragma mark - 模拟用户操作与面包屑添加

- (void)showDrawingActionAlert:(NSString *)message {
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"模拟图纸操作"
                                                                   message:message
                                                            preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"确定" style:UIAlertActionStyleDefault handler:nil]];
    [self presentViewController:alert animated:YES completion:nil];
}

#pragma mark - CellAction 埋点操作
- (void)actionOpenDrawing {
    NSString *drawingName = @"building_structure_layout.dwg";
    NSString *drawingPath = @"/Documents/ZWCAD/Projects/BuildingDesign/building_structure_layout.dwg";
    long fileSize = 4598102;
    NSString *fileHash = @"2a6b98e1f0e2d3c456897bc8a9f0e1d2";
    NSString *fileID = @"dwg_file_1024";
    NSString *projectID = @"project_365_a01";
    NSString *projectName = @"BuildingDesign";

    [[ZWMobileGuard sharedInstance] setActiveDrawingName:drawingName
                                                 path:drawingPath
                                                 size:fileSize
                                                 hash:fileHash
                                               fileID:fileID
                                            projectID:projectID
                                          projectName:projectName];

    [[ZWMobileGuard sharedInstance] addBreadcrumbCategory:@"Drawing"
                                                action:@"OpenDWG"
                                               details:[NSString stringWithFormat:@"name=%@, size=%.2fMB", drawingName, fileSize / 1024.0 / 1024.0]];

    g_currentDrawingStatus = [NSString stringWithFormat:@"图纸状态: %@ 已关联\n大小: %.2fMB  MD5: %@",
                                    drawingName,
                                    fileSize / 1024.0 / 1024.0,
                                    [fileHash substringToIndex:8]];
    [self showDrawingActionAlert:@"[埋点成功] 关联活跃图纸：building_structure_layout.dwg\n并且已向环形面包屑缓冲区写入 [Drawing][OpenDWG] 操作！"];
}

- (void)actionZoomDrawing {
    static float scale = 1.0f;
    scale *= 1.25f;
    if (scale > 10.0f) {
        scale = 1.0f;
    }

    [[ZWMobileGuard sharedInstance] addBreadcrumbCategory:@"Gesture"
                                                action:@"PinchZoom"
                                               details:[NSString stringWithFormat:@"zoom_scale=%.2f", scale]];
    [self showDrawingActionAlert:[NSString stringWithFormat:@"[埋点成功] 写入操作面包屑:\n[Gesture][PinchZoom] scale=%.2f", scale]];
}

- (void)actionDrawCircle {
    [[ZWMobileGuard sharedInstance] addBreadcrumbCategory:@"Draw"
                                                action:@"CreateCircle"
                                               details:@"center=(341.25, 592.11), radius=45.0"];
    [self showDrawingActionAlert:@"[埋点成功] 写入操作面包屑:\n[Draw][CreateCircle] center=(341.25, 592.11), radius=45.0"];
}

- (void)actionDrawLine {
    [[ZWMobileGuard sharedInstance] addBreadcrumbCategory:@"Draw"
                                                action:@"CreateLine"
                                               details:@"from=(12.0, 50.0), to=(120.5, 95.0)"];
    [self showDrawingActionAlert:@"[埋点成功] 写入操作面包屑:\n[Draw][CreateLine] from=(12,50), to=(120.5,95)"];
}

- (void)actionCloseDrawing {
    [[ZWMobileGuard sharedInstance] clearActiveDrawing];
    [[ZWMobileGuard sharedInstance] addBreadcrumbCategory:@"Drawing"
                                                action:@"CloseDWG"
                                               details:@"file=building_structure_layout.dwg"];

    g_currentDrawingStatus = @"图纸状态: 未打开任何图纸\n(崩溃日志将不携带图纸哈希)";
    [self showDrawingActionAlert:@"[埋点成功] 已清空当前关联图纸。写入操作面包屑:\n[Drawing][CloseDWG]"];
}

#pragma mark - CellAction 崩溃场景触发事件

- (void)triggerCppError {
    [[ZWMobileGuard sharedInstance] addBreadcrumbCategory:@"Test" action:@"CrashTrigger" details:@"Uncaught std::runtime_error"];
    throw std::runtime_error("Triggered uncaught std::runtime_error inside UI Thread");
}

- (void)triggerCustomCppException {
    [[ZWMobileGuard sharedInstance] addBreadcrumbCategory:@"Test" action:@"CrashTrigger" details:@"Uncaught Custom CPP Exception"];
    throw CustomCPPException("This is a custom non-standard C++ exception payload");
}

- (void)triggerPrimitiveException {
    [[ZWMobileGuard sharedInstance] addBreadcrumbCategory:@"Test" action:@"CrashTrigger" details:@"Uncaught Primitive C++ exception"];
    throw 42;
}

- (void)triggerDoubleThrowException {
    [[ZWMobileGuard sharedInstance] addBreadcrumbCategory:@"Test" action:@"CrashTrigger" details:@"Destructor double throw"];
    try {
        DestructorDoubleThrower thrower;
        throw std::runtime_error("Initial Exception in scope");
    } catch (...) {
        NSLog(@"Catching Initial Exception...");
    }
}

- (void)triggerOCFirewall {
    [[ZWMobileGuard sharedInstance] addBreadcrumbCategory:@"Test" action:@"FirewallTrigger" details:@"OC Exception Firewall"];
    @try {
        try {
            throw std::out_of_range("ZWCAD C++ Vector indexing out of range");
        } catch (const std::exception& e) {
            NSString *reason = [NSString stringWithUTF8String:e.what()];
            @throw [NSException exceptionWithName:@"zwMobileGuardBorderFirewallException" reason:reason userInfo:nil];
        }
    } @catch (NSException *exception) {
        [self showFirewallAlert:[NSString stringWithFormat:@"[OC防火墙拦截成功] 防火墙已捕获底层逃逸异常，并将其安全桥接给 OC 运行时进行业务降级处理。应用未崩溃！\n异常原因: %@", exception.reason]];
    }
}

- (void)triggerJniFirewallMock {
    [[ZWMobileGuard sharedInstance] addBreadcrumbCategory:@"Test" action:@"FirewallTrigger" details:@"JNI Firewall Mock"];
    try {
        throw std::invalid_argument("Invalid drawing coordinates (NAN detected)");
    } catch (const std::exception& e) {
        NSString *logMsg = [NSString stringWithFormat:@"[JNI异常防火墙拦截模拟]\n"
                            "1. 拦截底层 C++ 异常: std::invalid_argument\n"
                            "2. 模拟调用 JNIEnv* env->ThrowNew(\"com/zwsoft/zwcad/DWGParseException\", \"%s\")\n"
                            "3. 拦截成功，已将本地异常安全桥接给 Java JVM 进行 catch 处理！进程免于崩溃。", e.what()];
        [self showFirewallAlert:logMsg];
    }
}

- (void)triggerOCBoundsException {
    [[ZWMobileGuard sharedInstance] addBreadcrumbCategory:@"Test" action:@"CrashTrigger" details:@"OC Array Index Bounds"];
    NSArray *arr = @[];
    id obj = arr[0];
    NSLog(@"%@", obj);
}

- (void)triggerOCSelectorException {
    [[ZWMobileGuard sharedInstance] addBreadcrumbCategory:@"Test" action:@"CrashTrigger" details:@"OC Unrecognized Selector"];
    [self performSelector:@selector(unimplementedSelectorInThisController)];
}

- (void)triggerSigSegv {
    [[ZWMobileGuard sharedInstance] addBreadcrumbCategory:@"Test" action:@"CrashTrigger" details:@"SIGSEGV"];
    volatile int *ptr = nullptr;
    *ptr = 0xdead;
}

- (void)triggerSigAbrt {
    [[ZWMobileGuard sharedInstance] addBreadcrumbCategory:@"Test" action:@"CrashTrigger" details:@"SIGABRT"];
    abort();
}

- (void)triggerSigIll {
    [[ZWMobileGuard sharedInstance] addBreadcrumbCategory:@"Test" action:@"CrashTrigger" details:@"SIGILL"];
    __builtin_trap();
}

- (void)triggerSigFpe {
    [[ZWMobileGuard sharedInstance] addBreadcrumbCategory:@"Test" action:@"CrashTrigger" details:@"SIGFPE"];
    raise(SIGFPE);
}

- (void)triggerStackOverflow {
    [[ZWMobileGuard sharedInstance] addBreadcrumbCategory:@"Test" action:@"CrashTrigger" details:@"Stack Overflow"];
    [self recursiveOverflow:0];
}

- (void)recursiveOverflow:(int)depth {
    volatile char buffer[8192];
    memset((void *)buffer, 0, sizeof(buffer));
    [self recursiveOverflow:depth + 1];
}

- (void)clickedActionWithType:(ActionType)actionType {
    switch (actionType) {
        case ActionTypeOpenDWG:
            [self actionOpenDrawing];
            break;
        case ActionTypeDoubleFinger:
            [self actionZoomDrawing];
            break;
        case ActionTypeDrawCircle:
            [self actionDrawCircle];
            break;
        case ActionTypeDrawLine:
            [self actionDrawLine];
            break;
        case ActionTypeCloseDWG:
            [self actionCloseDrawing];
            break;
        case ActionTypeUncatchRuntime:
            [self triggerCppError];
            break;
        case ActionTypeUncatchCustom:
            [self triggerCustomCppException];
            break;
        case ActionTypeThrowBaseData:
            [self triggerPrimitiveException];
            break;
        case ActionTypeDestructor:
            [self triggerDoubleThrowException];
            break;
        case ActionTypeFireWallCatch:
            [self triggerOCFirewall];
            break;
        case ActionTypeJNIExceptionSafeConv:
            [self triggerJniFirewallMock];
            break;
        case ActionTypeCrossing:
            [self triggerOCBoundsException];
            break;
        case ActionTypeUnrecognizedSelector:
            [self triggerOCSelectorException];
            break;
        case ActionTypeWildOrNullPointer:
            [self triggerSigSegv];
            break;
        case ActionTypeSIGABRT:
            [self triggerSigAbrt];
            break;
        case ActionTypeSIGILL:
            [self triggerSigIll];
            break;
        case ActionTypeSIGFPE:
            [self triggerSigFpe];
            break;
        case ActionTypeStackOverflow:
            [self triggerStackOverflow];
            break;
            
        default:
            break;
    }
}

#pragma mark - UITableViewDelegate && UITableViewDataSource
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    NSDictionary *sectionDic = [self.pageDataArr objectAtIndex:indexPath.section];
    NSArray *typeArr = [sectionDic objectForKey:DataTypeId];
    NSNumber *actionType = [typeArr objectAtIndex:indexPath.row];
    [self clickedActionWithType:(ActionType)actionType.intValue];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    NSDictionary *cellDic = [self.pageDataArr objectAtIndex:section];
    NSArray *cellDataArr = [cellDic objectForKey:DataValuesID];
    return cellDataArr.count;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return self.pageDataArr.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CUSTOM_CELL_IDENTIFY forIndexPath:indexPath];
    NSDictionary *sectionDic = [self.pageDataArr objectAtIndex:indexPath.section];
    NSArray *cellDataArr = [sectionDic objectForKey:DataValuesID];
    if (@available(iOS 14.0, *)) {
        UIListContentConfiguration *content = [UIListContentConfiguration subtitleCellConfiguration];
        content.text = [cellDataArr objectAtIndex:indexPath.row];
        content.textProperties.color = UIColor.blackColor;
        cell.contentConfiguration = content;
    } else {
        cell.textLabel.text = [cellDataArr objectAtIndex:indexPath.row];
        cell.textLabel.textColor = UIColor.blackColor;
    }
    cell.backgroundColor = [UIColor whiteColor];

    return cell;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
    static NSString *headerID = @"CustomHeaderView";
    UITableViewHeaderFooterView *header = [tableView dequeueReusableHeaderFooterViewWithIdentifier:headerID];
    if (!header) {
        header = [[UITableViewHeaderFooterView alloc] initWithReuseIdentifier:headerID];
        UIView *backgroundView = [[UIView alloc] init];
        backgroundView.backgroundColor = [UIColor lightGrayColor];
        header.backgroundView = backgroundView;
    }
    
    NSDictionary *sectionDic = [self.pageDataArr objectAtIndex:section];
    NSString *title = [sectionDic objectForKey:DataTitleID];
    
    if (@available(iOS 14.0, *)) {
        UIListContentConfiguration *config = [header defaultContentConfiguration];
        config.text = title;
        config.textProperties.color = [UIColor blackColor];
        config.textProperties.font = [UIFont boldSystemFontOfSize:16.0];
        header.contentConfiguration = config;
    } else {
        header.textLabel.text = title;
        header.textLabel.textColor = [UIColor blackColor];
        header.textLabel.font = [UIFont boldSystemFontOfSize:16.0];
    }
    
    return header;
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
    return 44.0;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    return 48.0;
}

#pragma mark - Lazy

- (UITableView *)tableView {
    if (!_tableView) {
        _tableView = [[UITableView alloc] initWithFrame:CGRectZero style:UITableViewStylePlain];
        _tableView.backgroundColor = [UIColor clearColor];
        _tableView.separatorColor = [UIColor colorWithWhite:0.0 alpha:0.12];
        if (@available(iOS 15.0, *)) {
            _tableView.sectionHeaderTopPadding = 0.0;
        }
        _tableView.dataSource = self;
        _tableView.delegate = self;
        [_tableView registerClass:UITableViewCell.class forCellReuseIdentifier:CUSTOM_CELL_IDENTIFY];
    }
    return _tableView;
}

@end
