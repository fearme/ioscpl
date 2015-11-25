//
//  GoogleAnalyticsModel.h
//  HPControlledPrint
//
//  Created by Harsh Pathak on 11/24/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface GoogleAnalyticsModel : NSObject
@property NSString *eventCategory;
@property NSString *eventAction;
@property NSString *screenName;
@property NSException *exception;
//@property NSString *hwId;
@end
