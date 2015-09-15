

#import "HPPrintJobRequest.h"

@implementation HPPrintJobRequest

- (instancetype)init
{
    self = [super init];
    if (self){
        self.applyExclusion = NO;
    }
    return self;
}

- (NSString *)providerAsString
{
    NSString *provider;
    if (self.providerId == ProviderQples) {
        provider = @"QPLES";
    } else {
        provider = @"";
    }
    
    return provider;
}

@end
