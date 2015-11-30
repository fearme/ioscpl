//
//  GoogleAnalyticsModel.h
//  HPControlledPrint
//
//  Created by Harsh Pathak on 11/24/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

#import <Foundation/Foundation.h>

// categories
NSString *const kGAControlledPrintIosSdk     = @"ControlledPrintIosSdk";

// Actions
NSString *const kGAInitConstructor     = @"InitConstructor";
NSString *const kGANotifyProvider      = @"NotifyProvider";
NSString *const kGAInitialized         = @"Initialized";
NSString *const kGAValidate            = @"Validate";
NSString *const kGAPrint               = @"PrintIssued";
NSString *const KGAExit                = @"Exit";

@interface GoogleAnalyticsModel : NSObject
@property NSString *eventCategory;
@property NSString *eventAction;
@property NSString *screenName;
@property NSException *exception;

//@property NSString *hwId;
@end
