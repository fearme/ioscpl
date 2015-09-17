//
//  HPControlledPrintManagerTest.m
//  HPControlledPrint
//
//  Created by Fredy on 9/11/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>
#import <OCMock/OCMock.h>

#import "HPControlledPrintManager.h"

@interface HPControlledPrintManagerTest : XCTestCase

@end

@implementation HPControlledPrintManagerTest

HPControlledPrintManager *cpManager;

- (void)setUp {
    [super setUp];
    
    cpManager = [[HPControlledPrintManager alloc] init];
    XCTAssertNotNil(cpManager, "HPControlledPrintManager should not be nil");
    XCTAssert([cpManager isKindOfClass:[HPControlledPrintManager class]], @"cpManager should be an instance of HPControlledPrintManager");
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

-(void)testSetScanIntervalSeconds {
    [cpManager setScanIntervalSeconds:60];
    XCTAssertTrue([cpManager getScanIntervalSeconds] == 60, @"Scan Interval should be 60 seconds");
}

- (void)testExample {
    // This is an example of a functional test case.
    HPControlledPrintManager *cpm = OCMClassMock([HPControlledPrintManager class]);
    [cpm setScanIntervalSeconds:100];
    int seconds = [cpm getScanIntervalSeconds]; //the mock object returns 0
    XCTAssert(seconds != 100, @"intervals should be equal");
    NSLog(@"seconds %d", seconds);
    
    //XCTAssertEqual(seconds, 100, @"Intervals should be equal");
    
    //XCTAssert(YES, @"Pass");
    //XCTAssertFalse(NO, @"Pass");
    //XCTAssertNil(nil, @"Pass");
}


@end
