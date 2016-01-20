

#import <Foundation/Foundation.h>

@interface HPServices : NSObject

@property (strong, nonatomic) NSString *couponsJobUrl;
@property (strong, nonatomic) NSString *postMetricsUrl;

@property (strong, nonatomic) NSString *getDeferredPrintForDevice;
@property (strong, nonatomic) NSString *setDeferredPrintDevice;

@property (strong, nonatomic) NSString *createDeferredPrintJob;
@property (strong, nonatomic) NSString *getDeferredPrintJobsForDevice;

@property (strong, nonatomic) NSString *notifyQples;
@property (strong, nonatomic) NSString *validateQplesToken;

@end
