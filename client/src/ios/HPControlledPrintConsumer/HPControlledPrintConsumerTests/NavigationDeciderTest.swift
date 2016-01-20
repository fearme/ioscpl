//
//  InitialNavigationTest.swift
//  HPControlledPrintConsumer
//
//  Created by Fredy on 12/8/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

import UIKit
import XCTest

class NavigationDeciderTest: XCTestCase {

    let mainNavigationContollerName = "MainNavigationController"
    let printNavicationConrollerName = "PrintNavigationController"
    
    override func setUp() {
        super.setUp()
        // Put setup code here. This method is called before the invocation of each test method in the class.
    }
    
    override func tearDown() {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
        super.tearDown()
    }

    func testNavigation() {
        let navigationDecider = NavigationDecider();
        let parameters1 = ["url" : "printit://clickit.cloudpublish.com/social/print/v2/QPLES/asset-qples-blah"]
        var navigationController : UIViewController = navigationDecider.decide(parameters1)
        XCTAssertTrue(navigationController.restorationIdentifier == printNavicationConrollerName, "Then name " + printNavicationConrollerName + " should be returned.")

        let parameters2 = NSDictionary()
        navigationController = navigationDecider.decide(parameters2)
        XCTAssertTrue(navigationController.restorationIdentifier == mainNavigationContollerName, "The name" + mainNavigationContollerName + " should be returned.");
    }

    func testPerformanceExample() {
        // This is an example of a performance test case.
        self.measureBlock() {
            // Put the code you want to measure the time of here.
        }
    }

}
