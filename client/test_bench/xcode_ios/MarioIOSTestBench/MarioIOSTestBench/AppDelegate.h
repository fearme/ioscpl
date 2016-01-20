//
//  AppDelegate.h
//  MarioIOSTestBench
//
//  Created by Brian C Sparks on 3/10/15.
//  Copyright (c) 2015 HP, inc. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;

-(void) printerChosen:(NSString *)ip_address;


@end

