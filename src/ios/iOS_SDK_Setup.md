# HPControlledPrint
# iOS SDK Setup


## Overview

This document describes how to setup a new XCode project to leverage the HPControlledPrint framework.

The iOS SDK follows the same main functionality and print flow as the Android SDK.

Android:

1. Instantiate the module
2. Initialize it.
3. Perform getPrinterList().
4. Perform print().
5. Perform exit().

iOS:

1. Instantiate the module
2. Initialize it.
3. Perform scanForPrinters().
4. Perform print().
5. Perform exit().

Two other iOS methods are provided for setting the proxy and setting the print scan interval.

The framework is already capable of sending print metrics data to the metrics database of the desired server stack.

The iOS SDK Framework is available in Artifactory:
http://pso-artifactory.hp10.us/artifactory/simple/mario-local/ios/simulator/debug/HPControlledPrint.framework_0.2/

There's a valuable sample application in the mario repo in github.com, in the project HPControlledPrintConsumer. Use it to give you an idea of how to leverage the iOS SDK.
See the branch 'feature/cpp' in GitHub:
https://github.com/IPGPTP/mario/tree/feature/cpp/client/src/ios.


## Example

See the following example code from PrintListViewController.swift of the HPControlledPrintConsumer project in order to start using the iOS library. 
A discussion follows of each step.

```swift
    override func viewDidLoad() {
        super.viewDidLoad()
        self.cpl = ControlledPrint(controlledPrintDelegate: self)
        self.cpl?.printerListUpdateInterval(3)
        self.cpl?.setProxy("proxy.atlanta.hp.com", port: "8080")
        self.cpl?.initialize(selectedServerStack!)

        showSpinner()
        
        self.cpl!.scanForPrinters()
    }
```
    
1. Create a ControlledPrint object and set its delegate.
     > self.cpl = ControlledPrint(controlledPrintDelegate: self)
2. Set the time interval to refresh the printer list in seconds.     
     > self.cpl?.printerListUpdateInterval(3)
3. If behind a proxy, set the proxy host and port.
     > self.cpl?.setProxy("proxy.atlanta.hp.com", port: "8080")
3. Initialize ControlledPrint with the desired server stack.
     > self.cpl?.initialize(selectedServerStack!)
4. Scan for printers.
     > self.cpl!.scanForPrinters()
5. When ready to print, send ControlledPrint the URL/token to print and the IP of the printer to use. (Not shown in above example code.)
     > self.cpl?.print(printJob!.imageUrl, printerIp: printJob!.printerIp)
6. It is not necessary to call ControlledPrint's exit() method since it does nothing. But you may want to do it for completeness.

The ControlledPrint swift class is the entry point to the iOS library's main class, HPControlledPrintManager. Browse the ControlledPrint class to see the supported methods. It is important to note that it enforces a singleton of the HPControlledPrintManager class.



## Setting Up The Framework In Your Project

The iOS SDK Framework is available in Artifactory:
http://pso-artifactory.hp10.us/artifactory/simple/mario-local/ios/simulator/debug/HPControlledPrint.framework_0.2/

Download the framework to your file system.

Two things are need to be done to set up the framework to work in your project (or HPControlledPrintConsumer sample project):

1. Setup the Bridging header file
   a. Open your project's target. Under Build Settings, search for "bridging"
   b. Under Objective-C Bridging Header, type: 
      > $(PROJECT_DIR)/HPControlledPrintApp/HPControlledPrint-Bridging-Header.h
      
2. Pulling in the HPControlledPrint.framework
   a. Drag HPControlledPrint.framework from your file system into the project.
   b. In project properties, under General / Embedded Binaries, click the + button to add HPControlledPrint.framework. 
      Alternately, instead of clicking +, you can simply drag the HPControlledPrint.framework folder from you project structure into the Embedded Binaries area.
      Remove one of the duplicate items of HPControlledPrint.framework under Linked Frameworks and Libraries (underneath the Embedded Binaries area).
      

## Compiling/Building Your Project

You'll need to select either iPhone 5S, 6, or 6 Plus in order to have successful build.
See the section below "To Be Implemented" for more details.


## To Be Implemented

The underlying Mario C++ libraries are currently only built to run in the x86_64 (64 bit) architecture and not the i386 (32 bit) architecture, and only for the simulators.

Thus, they can only be built and run in the simulator, in particular, the 64 bit simulators like iPhone 5s, iPhone 6, and 6 Plus .

Other functionality currently in the Android SDK that are not stated above are in the pipeline to be implemented.


