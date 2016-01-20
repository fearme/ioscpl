

#import "HPPrinterAttributes.h"

@implementation HPPrinterAttributes

- (instancetype)init
{
    self = [super init];
    if (self){
        self.isSupported = NO;
        self._supportedFlagIsSet = NO;
    }
    
    return self;
}

@end
