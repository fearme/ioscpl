//
//  HPProviderNotifier.h
//  HPControlledPrint
//
//  Created by Fredy on 10/27/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface HPProviderNotifier : NSObject

- (void)sendPrintStatus:(NSString *)status notifyUrl:(NSString *)url tokenId:(NSString *)tokenId serverStack:(NSString *)stack;

@end
