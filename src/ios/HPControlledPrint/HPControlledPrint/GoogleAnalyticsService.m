//
//  GoogleAnalyticsService.m
//  HPControlledPrint
//
//  Created by Harsh Pathak on 11/24/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

#import "GoogleAnalyticsService.h"
#import "GAIDictionaryBuilder.h"

@implementation GoogleAnalyticsService

+(void)setUpGoogleAnalyticsService;
{
    NSLog(@"Setting up Analytics Service");
    [GAI sharedInstance].dispatchInterval = 20;
    
    [GAI sharedInstance].trackUncaughtExceptions = YES;
    [[GAI sharedInstance].logger setLogLevel:kGAILogLevelVerbose];

    // following needs to be assigned even tho tracker is not used.
    id<GAITracker> tracker = [[GAI sharedInstance] trackerWithTrackingId:@"UA-69772755-5"];
}

+(id<GAITracker>)getGoogleAnalyticsTracker;
{
//    id<GAITracker> tracker = [[GAI sharedInstance] trackerWithTrackingId:@"UA-69772755-5"];
    id<GAITracker> tracker = [[GAI sharedInstance] defaultTracker];
    return tracker;
}

+(void)trackEventCategory:(NSString *)category withAction:(NSString *)action andLabel:(NSString *)label;
{
    NSLog(@"Tracking Analytics Event");
    NSLog(@"category: %@", category);
    id<GAITracker> tracker = [GoogleAnalyticsService getGoogleAnalyticsTracker];
    // label is hardware_id or unique_id
    // ask Fredy how to get this id
    [tracker send:[[GAIDictionaryBuilder createEventWithCategory:category
                                                          action:action
                                                           label:label
                                                           value:nil] build]];
}

+(void)trackScreenView:(NSString *)screenName withHwId:(NSString *)hwId;
{
    NSLog(@"Tracking Analytics Screen: %@", screenName);
    //TODO: NEED TO ADD HWID AS CUSTOM DIMENSION
    id<GAITracker> tracker = [GoogleAnalyticsService getGoogleAnalyticsTracker];
    [tracker set:kGAIScreenName
           value:screenName];
    [tracker send:[[GAIDictionaryBuilder createScreenView] build]];
}

+(void)trackException:(NSException *)exception withHwId:(NSString *)hwId;
{
    NSLog(@"Tracking Exception");
    id<GAITracker> tracker = [GoogleAnalyticsService getGoogleAnalyticsTracker];
    NSString * model = [[UIDevice currentDevice] model];
    NSString * version = [[UIDevice currentDevice] systemVersion];
    NSArray * backtrace = [exception callStackSymbols];
    NSString * description = [NSString stringWithFormat:@"%@.%@.%@.Backtrace:%@",
                              model,
                              version,
                              exception.description,
                              backtrace];
    
    [tracker send:[[GAIDictionaryBuilder
                    createExceptionWithDescription:description  // Exception description. May be truncated to 100 chars.
                    withFatal:@NO] build]];
}


// create object like this
//Person *person = [[Person alloc] init];
// invoke method like this
//[talkingiPhone speak];
// set var like this
// talkingiPhone.phoneName = @"Harsh";
// read only
// @property (readonly) NSString *lastName;
// instance and not accessible outside of class objects
//@interface Coffee : NSObject {
//    NSNumber *_temperature;
//}
//@end
@end
