
#import "HPDiscoveredPrinters.h"

@implementation HPDiscoveredPrinters


- (instancetype)init
{
    self = [super init];
    if (self){
        self.printers = [[NSMutableDictionary alloc] init];
    }
    return self;
}


@end
