//
//  HPControlledPrintManagerUtilTest.m
//  HPControlledPrint
//
//  Created by Fredy on 11/5/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>

#import "HPControlledPrintManagerUtil.h"

#include "../HPControlledPrint/Includes/mwp_secure_asset_printing_api.hpp"

@interface HPControlledPrintManagerUtilTest : XCTestCase

@end

@implementation HPControlledPrintManagerUtilTest

static net_mobilewebprint::secure_asset_printing_api_t *secureAssetPrinter;

- (void)setUp {
    [super setUp];
    secureAssetPrinter = net_mobilewebprint::sap_api();
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void)testPrepareToken {
    //test a token without the Qples token prefix
    NSString *token = @"QPLESLIKETOKEN-2321321312321321321";
    NSString *preparedToken = [HPControlledPrintManagerUtil prepareToken:token];
    XCTAssertTrue([token isEqualToString:preparedToken]);
    
    //test a token with the Qples token prefix
    NSString *tokenWithPrefix = [NSString stringWithFormat:@"%@%@", [HPControlledPrintManagerUtil qplesPrefix], token];
    preparedToken = [HPControlledPrintManagerUtil prepareToken:tokenWithPrefix];
    XCTAssertTrue([token isEqualToString:preparedToken]); //the prefix should have been removed
}

- (void)testParseDiscoveryData {
    NSArray* hosts = [NSArray arrayWithObjects: @"hqdev", @"dev", @"dev", nil];
    NSDictionary *dict = [[NSDictionary alloc] initWithObjectsAndKeys:
                          @"https://cayman-dev-02.cloudpublish.com", @"caymanrooturl",
                          @"/coupons/job", @"couponsjobasyncurl",
                          @"/coupons/job/synchronous", @"couponsjoburl",
                          @"/createJob", @"createjob",
                          @"/coupons/deferedprint", @"deferedprint",
                          @"/getJob", @"getjob",
                          @"/getJobsOfDevice", @"getjobsdevice",
                          @"/metrics/m", @"metricsurl",
                          @"mobiledevprint.net", @"printserverdomain",
                          hosts, @"printserverhosts",
                          nil];
    
    HPServices *services = [HPControlledPrintManagerUtil parseDiscoveryData:dict secureAssetPrint:secureAssetPrinter caymanRootUrl:@"https://cayman-dev-02.cloudpublish.com"];
    
    // Note that the line below is using 'requestedStackName' instead of 'serverName' because
    //   Mario is changing the key.
    std::string const &server = secureAssetPrinter->get_option("requestedStackName");
    std::string const &fallbackOne = secureAssetPrinter->get_option("fallbackServiceOne");
    std::string const &fallbackTwo = secureAssetPrinter->get_option("fallbackServiceTwo");
    std::string const &domain = secureAssetPrinter->get_option("domainName");
    
    NSString *serverName = [NSString stringWithCString:server.c_str() encoding:[NSString defaultCStringEncoding]];
    NSString *fallback1 = [NSString stringWithCString:fallbackOne.c_str() encoding:[NSString defaultCStringEncoding]];
    NSString *fallback2 = [NSString stringWithCString:fallbackTwo.c_str() encoding:[NSString defaultCStringEncoding]];
    NSString *domainName = [NSString stringWithCString:domain.c_str() encoding:[NSString defaultCStringEncoding]];
    
    XCTAssertTrue([serverName isEqualToString:@"hqdev"]);
    XCTAssertTrue([fallback1 isEqualToString:@"dev"]);
    XCTAssertTrue([fallback2 isEqualToString:@"dev"]);
    XCTAssertTrue([domainName isEqualToString:@"mobiledevprint.net"]);
}

- (void)testPerformanceExample {
    // This is an example of a performance test case.
    [self measureBlock:^{
        // Put the code you want to measure the time of here.
    }];
}

@end
