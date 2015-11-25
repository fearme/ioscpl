//
//  GoogleAnalyticsService.h
//  HPControlledPrint
//
//  Created by Harsh Pathak on 11/24/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface GoogleAnalyticsService : NSObject
+(void) setUpGoogleAnalyticsService;
+(id<GAITracker>) getGoogleAnalyticsTracker;
+(void) trackEventCategory:(NSString *)category withAction:(NSString *)action andLabel:(NSString *)label;
+(void) trackScreenView:(NSString *)screenName withHwId:(NSString *)hwId;
+(void) trackException:(NSException *)exception withHwId:(NSString *)hwId;
@end
