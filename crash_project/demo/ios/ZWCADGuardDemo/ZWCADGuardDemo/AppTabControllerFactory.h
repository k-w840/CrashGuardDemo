#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface AppTabControllerFactory : NSObject

+ (NSArray<UINavigationController *> *)buildTabNavigationControllers;

@end

NS_ASSUME_NONNULL_END
