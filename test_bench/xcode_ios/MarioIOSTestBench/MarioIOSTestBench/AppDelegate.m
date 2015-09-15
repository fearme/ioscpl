//
//  AppDelegate.m
//  MarioIOSTestBench
//
//  Created by Brian C Sparks on 3/10/15.
//  Copyright (c) 2015 HP, inc. All rights reserved.
//

#import "AppDelegate.h"

#include "hp_sap.h"
#import "Printer.h"


@interface AppDelegate ()

- (int)dispatch:(char const *)message identifiedBy:(int)identifier forTransaction:(int)transaction_id withP1:(uint8 const *)p1 andParams:(sap_params const *)params;
- (int)afterMwpStart;

@end

int sap_app_callback(void * app_data, char const * msg, int ident, int32 transaction_id, uint8 const * p1, sap_params const * params)
{
  AppDelegate * delegate = (__bridge AppDelegate *)(app_data);
    
  return [delegate dispatch:msg identifiedBy:ident forTransaction:transaction_id withP1:p1 andParams:params];
}

@implementation AppDelegate


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
  // Override point for customization after application launch.

  hp_sap_register_handler("app", (__bridge void *)(self), sap_app_callback);
  hp_sap_set_flag("quiet", true);

  // Set these as desired, or leave commented out for normal behavior
  //hp_sap_set_flag("vvverbose", true);
//  hp_sap_set_flag("quiet", false);
  hp_sap_set_flag("fast-fail", true);

  hp_sap_set_option("http_proxy_name", "proxy.atlanta.hp.com");
  hp_sap_set_option("http_proxy_port", "8080");

//  hp_sap_set_flag("log_api", true);

  hp_sap_start();
  hp_sap_set_timeout("afterMwpStart", 500);
  return YES;
}

-(int) dispatch:(char const *)message identifiedBy:(int)identifier forTransaction:(int)transaction_id withP1:(uint8 const *)p1 andParams:(sap_params const *)params {
  
//  char const * p2 = params ? params->p2 ? (char const *)params->p2 : "" : "";
//  char const * p3 = params ? params->p3 ? (char const *)params->p3 : "" : "";
  
  if (strcmp(message, "log_d") == 0) {
    printf("D:MWP-core: %s\n", p1 ? (char const *)p1 : "");
    return 1;
  } else if (strcmp(message, "log_v") == 0) {
    printf("V:MWP-core: %s\n", p1 ? (char const *)p1 : "");
    return 1;
  } else {
    //printf("Message: %s %d %d %s\n", message, identifier, transaction_id, p1 != NULL ? p1 : "");
    if (strcmp(message, "afterMwpStart") == 0) {
      return [self afterMwpStart];
    }
  }
  
  return 1;
}

- (int)afterMwpStart {

//  hp_sap_set_option("add_printer", "192.168.11.108;9100;photosmart 7520 series");
  return 1;
}

- (void) printerChosen:(NSString *)ip_address {
  
//  hp_sap_send_job("http://qa1.hpsavingscenter.com/filestore/files/11911/data", [ip_address UTF8String]);
  hp_sap_send_job("http://cayman-dev-02.cloudpublish.com/filestore/files/4/data", [ip_address UTF8String]);

}

- (void)applicationWillResignActive:(UIApplication *)application {
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillTerminate:(UIApplication *)application {
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

@end
