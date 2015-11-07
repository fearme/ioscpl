//
//  HPControlledPrintManagerUtil.m
//  HPControlledPrint
//
//  Created by Fredy on 11/5/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

#import "HPControlledPrintManagerUtil.h"
#import "HPServices.h"

#include "../Includes/mwp_secure_asset_printing_api.hpp"


@implementation HPControlledPrintManagerUtil

NSString *const kQplesTokenPrefix = @"asset-qples-";

+ (NSString *)qplesPrefix
{
    return kQplesTokenPrefix;
}

// This method is used to remove "asset-qples-" from the beginning of the token before
//  (1) validating the token
//  (2) notifying Qples of a print success or failure.
// It is not used when sending the print job to Mario since Mario strips off "asset-qples-".
+ (NSString *)prepareToken:(NSString *)token
{
    if( (token != nil) && [token hasPrefix:kQplesTokenPrefix]) {
        return [token substringFromIndex:12];
    } else {
        return token;
    }
}

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

@end
