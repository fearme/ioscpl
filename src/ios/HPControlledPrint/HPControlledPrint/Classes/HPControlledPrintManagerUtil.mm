//
//  HPControlledPrintManagerUtil.m
//  HPControlledPrint
//
//  Created by Fredy on 11/5/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

#import <CommonCrypto/CommonHMAC.h>

#import "HPControlledPrintManagerUtil.h"
#import "HPServices.h"

#include "../Includes/mwp_secure_asset_printing_api.hpp"


@implementation HPControlledPrintManagerUtil


+ (HPServices *)parseDiscoveryData:(NSDictionary *)dict secureAssetPrint:(net_mobilewebprint::secure_asset_printing_api_t *)sap caymanRootUrl:(NSString *)caymanRootUrl
{
    NSLog(@"json: %@", dict);
    
    HPServices *services = [[HPServices alloc] init];
    
    // jobUrl = discoverProperties.getCaymanRootUrl() + discoverProperties.getCouponsJobAsyncUrl();
    services.couponsJobUrl = [NSString stringWithFormat:@"%@%@", caymanRootUrl, [dict objectForKey:@"couponsjobasyncurl"]];
    NSLog(@"couponsJob: %@", services.couponsJobUrl);
    
    // metricsPostURL = discoverProperties.getCaymanRootUrl() + discoverProperties.getMetricsUrl();
    services.postMetricsUrl = [NSString stringWithFormat:@"%@%@", caymanRootUrl, [dict objectForKey:@"metricsurl"]];
    NSLog(@"postsMetrics: %@", services.postMetricsUrl);
    
    // getDeviceUrl = discoverProperties.getCaymanRootUrl() + discoverProperties.getDeferedPrint() + "/getDevice" + "?deviceID=";
    services.getDeferredPrintForDevice = [NSString stringWithFormat:@"%@%@/getDevice?deviceID=", caymanRootUrl, [dict objectForKey:@"deferedprint"]];
    NSLog(@"getDeferredPrintForDevice: %@", services.getDeferredPrintForDevice);
    
    // createJobUrl = discoverProperties.getCaymanRootUrl() + discoverProperties.getDeferedPrint() + discoverProperties.getCreateJob();
    services.createDeferredPrintJob = [NSString stringWithFormat:@"%@%@%@", caymanRootUrl, [dict objectForKey:@"deferedprint"], [dict objectForKey:@"createjob"]];
    NSLog(@"createDeferredPrintJob: %@", services.createDeferredPrintJob);
    
    // getDeferedJobsUrl = discoverProperties.getCaymanRootUrl() + discoverProperties.getDeferedPrint() + discoverProperties.getGetJobsDevice() + "?deviceID=";
    services.getDeferredPrintJobsForDevice = [NSString stringWithFormat:@"%@%@%@?deviceID=", caymanRootUrl, [dict objectForKey:@"deferedprint"], [dict objectForKey:@"getjobsdevice"]];
    NSLog(@"getDeferredPrintJobsForDevice: %@", services.getDeferredPrintJobsForDevice);
    
    // saveDeviceUrl = discoverProperties.getCaymanRootUrl() + discoverProperties.getDeferedPrint() + "/saveDevice";
    services.setDeferredPrintDevice = [NSString stringWithFormat:@"%@%@/saveDevice", caymanRootUrl, [dict objectForKey:@"deferedprint"]];
    NSLog(@"setDeferredPrintDevice: %@", services.setDeferredPrintDevice);
    
    // notifyQplesUrl = discoverProperties.getCaymanRootUrl() + "/coupons/qples/notify";
    services.notifyQples = [NSString stringWithFormat:@"%@/coupons/qples/notify", caymanRootUrl];
    NSLog(@"notifyQples: %@", services.notifyQples);
    
    // validateTokenUrl = discoverProperties.getCaymanRootUrl() + "/coupons/qples/validate";
    services.validateQplesToken = [NSString stringWithFormat:@"%@/coupons/qples/validate", caymanRootUrl];
    NSLog(@"validateQplesToken: %@", services.validateQplesToken);
    
    NSArray *printServerHosts   = [dict objectForKey:@"printserverhosts"];
    NSString *printServerDomain = [dict objectForKey:@"printserverdomain"];
    
    const char *serverName         = [printServerHosts[0] cStringUsingEncoding:NSASCIIStringEncoding];
    const char *fallbackServiceOne = [printServerHosts[1] cStringUsingEncoding:NSASCIIStringEncoding];
    const char *fallbackServiceTwo = [printServerHosts[2] cStringUsingEncoding:NSASCIIStringEncoding];
    const char *domainName         = [printServerDomain cStringUsingEncoding:NSASCIIStringEncoding];
    
    sap->set_option("serverName", serverName);
    sap->set_option("fallbackServiceOne", fallbackServiceOne);
    sap->set_option("fallbackServiceTwo", fallbackServiceTwo);
    sap->set_option("domainName", domainName);
    
    NSLog(@"serverName: %s", serverName);
    NSLog(@"fallbackServiceOne: %s", fallbackServiceOne);
    NSLog(@"fallbackServiceTwo: %s", fallbackServiceTwo);
    NSLog(@"domainName: %s", domainName);
    
    return services;
}

+ (NSString *) hash:(NSString *)source withSalt:(NSString *)salt
{
    const char *cKey  = [salt cStringUsingEncoding:NSUTF8StringEncoding];
    const char *cData = [source cStringUsingEncoding:NSUTF8StringEncoding];
    unsigned char cHMAC[CC_SHA256_DIGEST_LENGTH];
    CCHmac(kCCHmacAlgSHA256, cKey, strlen(cKey), cData, strlen(cData), cHMAC);
    
    NSString *hash;
    
    NSMutableString* output = [NSMutableString stringWithCapacity:CC_SHA256_DIGEST_LENGTH * 2];
    
    for(int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++)
        [output appendFormat:@"%02x", cHMAC[i]];
    hash = output;
    return hash;
}


+ (NSString *) salt:(int)length
{    
    NSString *letters = @"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    NSMutableString *randomString = [NSMutableString stringWithCapacity: length];
    
    for (int i=0; i < length; i++) {
        [randomString appendFormat: @"%C", [letters characterAtIndex: arc4random_uniform([letters length])]];
    }
    
    return randomString;
}

@end
