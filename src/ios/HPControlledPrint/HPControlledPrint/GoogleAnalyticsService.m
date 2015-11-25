//
//  GoogleAnalyticsService.m
//  HPControlledPrint
//
//  Created by Harsh Pathak on 11/24/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

#import "GoogleAnalyticsService.h"

@implementation GoogleAnalyticsService

-(void)setUpGoogleAnalyticsService;
{
    NSLog(@"Setting up Analytics Service");
}

-(id<GAITracker>)getGoogleAnalyticsTracker;
{
    id<GAITracker> tracker = [[GAI sharedInstance] trackerWithTrackingId:@"UA-69772755-5"];
    return tracker;
}

-(void)trackEventCategory:(NSString *)category withAction:(NSString *)action andLabel:(NSString *)label;
{
    NSLog(@"Tracking Analytics Event");
    NSLog(@"category: %@", category);
}
// create object like this
//Person *person = [[Person alloc] init];
// invoke method like this
//[talkingiPhone speak];
// set var like this
// talkingiPhone.phoneName = @"Harsh";
@end
