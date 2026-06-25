//
//  CrashDemoUtil.h
//  ZWCADGuardDemo
//
//  Created by kwb on 2026/6/21.
//

#import <Foundation/Foundation.h>

#define DataTitleID @"dataTitleId"
#define DataValuesID @"dataValuesId"
#define DataTypeId @"dataTypeId"

typedef enum {
    ActionTypeNone,
    
    ActionTypeOpenDWG,
    ActionTypeDoubleFinger,
    ActionTypeDrawCircle,
    ActionTypeDrawLine,
    ActionTypeCloseDWG,
    
    ActionTypeUncatchRuntime,
    ActionTypeUncatchCustom,
    ActionTypeThrowBaseData,
    ActionTypeDestructor,
    
    ActionTypeFireWallCatch,
    ActionTypeJNIExceptionSafeConv,
    
    ActionTypeCrossing,
    ActionTypeUnrecognizedSelector,
    
    ActionTypeWildOrNullPointer,
    ActionTypeSIGABRT,
    ActionTypeSIGILL,
    ActionTypeSIGFPE,
    ActionTypeStackOverflow
} ActionType;

extern NSString * _Nonnull g_currentDrawingStatus;

NS_ASSUME_NONNULL_BEGIN

@interface CrashDemoUtil : NSObject
+ (NSArray *)demoTestData;
@end

NS_ASSUME_NONNULL_END
