//
//  NavigationDecider.swift
//  HPControlledPrintConsumer
//
//  Created by Fredy on 12/8/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

import Foundation
import UIKit

class NavigationDecider {
    
    func decide(parameters : NSDictionary) -> UIViewController {

        if(parameters.objectForKey("url") != nil){
            let storyboard = UIStoryboard(name: "Main", bundle: nil)
            return storyboard.instantiateViewControllerWithIdentifier("PrintNavigationController");
            
        } else {
            let storyboard = UIStoryboard(name: "Main", bundle: nil)
            return storyboard.instantiateViewControllerWithIdentifier("MainNavigationController");
        }
    }
}