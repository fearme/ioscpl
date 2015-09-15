//
//  Printer.h
//  MarioIOSTestBench
//
//  Created by Brian C Sparks on 3/16/15.
//  Copyright (c) 2015 HP, inc. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface Printer : NSObject

@property NSString *displayName;
@property NSString *ipAddress;

@property NSString *_1284DeviceId;

@property NSMutableDictionary *otherAttributes;

- (void)setKey:(NSString*)key andValue:(NSString*)value;

@end
