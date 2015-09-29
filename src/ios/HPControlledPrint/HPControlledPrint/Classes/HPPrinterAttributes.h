

#import <Foundation/Foundation.h>
#import "HPDiscoveredPrinters.h"

@protocol HPPrinterAttributesDelegate;
@protocol HPPrinterAttributesDelegate <NSObject>
- (void)didReceivePrintJobStatus: (NSString *)string;
- (void)didReceivePrinters: (HPDiscoveredPrinters *)printers;
@end


@interface HPPrinterAttributes : NSObject

typedef enum {
    PrinterSupportedNo,
    PrinterSupportedYes,
    PrinterSupportedUnassigned
} PrinterSupported;

@property (strong, nonatomic) NSString *ip; //printer IP
@property (strong, nonatomic) NSString *name;
@property (strong, nonatomic) NSString *macAddress;
@property (strong, nonatomic) NSString *status;
@property (strong, nonatomic) NSString *manufacturer;
@property (assign) PrinterSupported isSupported;

@end
