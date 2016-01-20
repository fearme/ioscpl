//
//  Printer.m
//  MarioIOSTestBench
//
//  Created by Brian C Sparks on 3/16/15.
//  Copyright (c) 2015 HP, inc. All rights reserved.
//

#import "Printer.h"

@implementation Printer

- (void)setKey:(NSString*)key andValue:(NSString*)value {

  if ([key isEqualToString:@"name"]) {
    _displayName = value;
  } else if ([key isEqualToString:@"1284_device_id"]) {
    __1284DeviceId = value;
  } else if ([key isEqualToString:@"ip"]){
    _ipAddress = value;
  }
}

@end

