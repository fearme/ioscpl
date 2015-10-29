//
//  HPProviderNotifier.m
//  HPControlledPrint
//
//  Created by Fredy on 10/27/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

#import "HPProviderNotifier.h"


@implementation HPProviderNotifier

- (void)sendPrintStatus:(NSString *)status notifyUrl:(NSString *)url tokenId:(NSString *)tokenId serverStack:(NSString *)stack
{
    NSString *statusXml = [NSString stringWithFormat:@"<coup:qplesNotification xmlns:coup=\"http://hp.com/sips/services/xml/coupons\"><printTokenType><printToken>%@</printToken><status>%@</status><stack>%@</stack></printTokenType></coup:qplesNotification>", tokenId, status, stack];
    NSLog(@"statusXml: %@ ^^^^^^^^^^^^^^^^^^^^^^^^^", statusXml);
    
    // Create the request.
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:url]];
    request.HTTPMethod = @"POST";
    [request setValue:@"application/xml; charset=utf-8" forHTTPHeaderField:@"Content-Type"];
    //[request setValue:@"application/xml; charset=utf-8" forHTTPHeaderField:@"Accept"];
    
    NSData *requestBodyData = [statusXml dataUsingEncoding:NSUTF8StringEncoding];
    request.HTTPBody = requestBodyData;
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{    NSURLResponse *response = nil;
        NSError *connectionError = nil;
        NSData *responseData = [NSURLConnection sendSynchronousRequest:request returningResponse:&response error:&connectionError];
        
        if (connectionError == nil) {
            if ([response isKindOfClass:[NSHTTPURLResponse class]]) {
                //NSLog(@"Response description: %@", [(NSHTTPURLResponse *)response description]);
                NSInteger statusCode = [(NSHTTPURLResponse *)response statusCode];
                if (statusCode != 200) {
                    NSLog(@"HPProviderNotifier: Response code = %ld", (long)statusCode);
                    return;
                } else {
                    NSLog(@"HPProviderNotifier: Prodider was successfully notified.");
                }
            }
        } else {
            NSLog(@"HPProviderNotifier: Unable to notify Provider of print status. Connection error = %@", connectionError);
        }
    });
}

@end
