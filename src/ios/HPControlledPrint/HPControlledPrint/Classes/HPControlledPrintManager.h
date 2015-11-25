
#import <Foundation/Foundation.h>
#import "HPPrinterAttributes.h"
#import "HPPrintJobRequest.h"
#import "HPPrinter.h"
#import "GoogleAnalyticsModel.h"

@interface HPControlledPrintManager : NSObject

typedef enum {
    ServerStackDevelopment,
    ServerStackQa,
    ServerStackStaging,
    ServerStackProduction
} ServerStack;

typedef enum {
    InitStatusServerStackNotAvailable,
    InitStatusServerStackAvailable
} InitStatus;

typedef enum {
    Event,
    Crash,
    Screen
} GoogleAnalyticsType;


@property (strong, nonatomic) id <HPPrinterAttributesDelegate> printerAttributesDelegate;

- (void)setScanIntervalSeconds:(int)seconds;
- (int)getScanIntervalSeconds;

- (void)proxy:(NSString *)host onPort:(NSString *)port;

- (void)initialize: (ServerStack)stack withCompletion:(void (^)(InitStatus status))completion;

- (void)validateToken:(NSString *)token withCompletion:(void (^)(BOOL valid))completion;

- (BOOL)scanForPrinters;

- (BOOL)print:(HPPrinter *)selectedPrinter withJobRequest:(HPPrintJobRequest *)printJobRequest;

- (void)postGoogleAnalyticsMetrics: (GoogleAnalyticsType)analyticsType withParams:(GoogleAnalyticsModel *)analyticsModel;

- (BOOL)exit;

@end
