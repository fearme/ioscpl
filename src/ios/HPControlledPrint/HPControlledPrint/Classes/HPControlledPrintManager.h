
#import <Foundation/Foundation.h>
#import "HPPrinterAttributes.h"
#import "HPPrintJobRequest.h"
#import "HPPrinter.h"

@interface HPControlledPrintManager : NSObject

typedef enum {
    ServerStackDevelopment,
    ServerStackExternalTest,
    ServerStackProduction
} ServerStack;

typedef enum {
    InitStatusServerStackNotAvailable,
    InitStatusServerStackAvailable,
    InitStatusTokenInvalid,
    InitStatusTokenValid
} InitStatus;


@property (strong, nonatomic) id <HPPrinterAttributesDelegate> printerAttributesDelegate;

- (void)setScanIntervalSeconds:(int)seconds;
- (int)getScanIntervalSeconds;

- (void)proxy:(NSString *)host onPort:(NSString *)port;

- (void)initialize: (ServerStack)stack withToken:(NSString *)token withCompletion:(void (^)(InitStatus status))completion;
- (BOOL)scanForPrinters;
- (BOOL)print: (HPPrinter *)selectedPrinter withJobRequest:(HPPrintJobRequest *)printJobRequest;
- (BOOL)exit;


@end
