//
//  HPMetricsSender.h
//  HPControlledPrint
//
//  Created by Fredy on 7/8/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "HPPrintJobRequest.h"

@interface HPMetricsSender : NSObject

- (void)send:(NSString *)url withPrintJobRequest:(HPPrintJobRequest *)printJobRequest forOperation:(NSString *)operation metricsType:(NSString *)metricsType;

@end
