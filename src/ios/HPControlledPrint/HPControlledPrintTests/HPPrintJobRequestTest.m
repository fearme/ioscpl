//
//  HPPrintJobRequestTest.m
//  HPControlledPrint
//
//  Created by Fredy on 9/16/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>
#import "HPPrintJobRequest.h"

@interface HPPrintJobRequestTest : XCTestCase

@end

@implementation HPPrintJobRequestTest

HPPrintJobRequest *printJobRequest;

- (void)setUp {
    [super setUp];
    printJobRequest = [[HPPrintJobRequest alloc] init];
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void)testProviderAsString {
    printJobRequest.providerId = ProviderQples;
    XCTAssertTrue([[printJobRequest providerAsString] isEqualToString:@"QPLES"], "ProviderQples should equal 'QPLES'");
}

@end
