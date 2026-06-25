#import "AppTabControllerFactory.h"

#import "ViewController.h"
#import "LogReportViewController.h"

@implementation AppTabControllerFactory

+ (NSArray<UINavigationController *> *)buildTabNavigationControllers {
    ViewController *demoController = [[ViewController alloc] init];
    demoController.navigationItem.largeTitleDisplayMode = UINavigationItemLargeTitleDisplayModeNever;
    demoController.tabBarItem = [[UITabBarItem alloc] initWithTitle:@"崩溃验证"
                                                              image:[UIImage systemImageNamed:@"play.rectangle"]
                                                      selectedImage:[UIImage systemImageNamed:@"play.rectangle.fill"]];

    UINavigationController *demoNavigationController = [[UINavigationController alloc] initWithRootViewController:demoController];
    [self applyNavigationAppearanceToNavigationController:demoNavigationController];

    LogReportViewController *reportController = [[LogReportViewController alloc] init];
    reportController.navigationItem.largeTitleDisplayMode = UINavigationItemLargeTitleDisplayModeNever;
    reportController.tabBarItem = [[UITabBarItem alloc] initWithTitle:@"日志上报"
                                                                image:[UIImage systemImageNamed:@"doc.text"]
                                                        selectedImage:[UIImage systemImageNamed:@"doc.text.fill"]];

    UINavigationController *reportNavigationController = [[UINavigationController alloc] initWithRootViewController:reportController];
    [self applyNavigationAppearanceToNavigationController:reportNavigationController];

    return @[
        demoNavigationController,
        reportNavigationController
    ];
}

+ (void)applyNavigationAppearanceToNavigationController:(UINavigationController *)navigationController {
    UINavigationBarAppearance *appearance = [[UINavigationBarAppearance alloc] init];
    [appearance configureWithOpaqueBackground];
    appearance.backgroundColor = [UIColor colorWithRed:45.0 / 255.0 green:45.0 / 255.0 blue:45.0 / 255.0 alpha:1.0];
    appearance.titleTextAttributes = @{ NSForegroundColorAttributeName: [UIColor whiteColor] };
    appearance.largeTitleTextAttributes = @{ NSForegroundColorAttributeName: [UIColor whiteColor] };

    navigationController.navigationBar.standardAppearance = appearance;
    navigationController.navigationBar.scrollEdgeAppearance = appearance;
    navigationController.navigationBar.compactAppearance = appearance;
    if (@available(iOS 15.0, *)) {
        navigationController.navigationBar.compactScrollEdgeAppearance = appearance;
    } else {
        // Fallback on earlier versions
    }
    navigationController.navigationBar.tintColor = [UIColor whiteColor];
    navigationController.navigationBar.translucent = NO;
    navigationController.navigationBar.prefersLargeTitles = NO;
    navigationController.view.backgroundColor = [UIColor blackColor];
}

@end
