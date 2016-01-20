//
//  HPTokenValidator.h
//  HPControlledPrint
//
//  Created by Fredy on 8/24/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef void (^HttpGetCompletion)(BOOL success);

@interface HPTokenValidator : NSObject

- (void)validate:(NSString *)token withServiceUrl:(NSString *)serviceUrl withCompletion:(HttpGetCompletion)completion;

@end
