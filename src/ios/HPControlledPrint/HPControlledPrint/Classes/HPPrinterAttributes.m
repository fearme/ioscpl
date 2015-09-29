

#import "HPPrinterAttributes.h"

@implementation HPPrinterAttributes

- (instancetype)init
{
    self = [super init];
    if (self){
        self.isSupported = PrinterSupportedUnassigned;
    }
    
    return self;
}

@end
