#import "AppDelegate.h"
#import "ZWMobileIOSGuard.h"
#import "AppTabControllerFactory.h"
#import "StaticTabBarController.h"

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
    self.window.backgroundColor = [UIColor blackColor];
    // 设置一个静态TabBar，方便切换
    StaticTabBarController *tabBarController = [[StaticTabBarController alloc] init];
    tabBarController.view.backgroundColor = [UIColor blackColor];
    tabBarController.viewControllers = [AppTabControllerFactory buildTabNavigationControllers];
    self.window.rootViewController = tabBarController;
    [self.window makeKeyAndVisible];
    // 初始化 SDK
    [[ZWMobileIOSGuard sharedInstance] initializeSDK];
    
    // 初始化一些基础面包屑，记录启动事件
    [[ZWMobileIOSGuard sharedInstance] addBreadcrumbCategory:@"App" action:@"Launch" details:@"didFinishLaunchingWithOptions"];
    return YES;
}

@end
