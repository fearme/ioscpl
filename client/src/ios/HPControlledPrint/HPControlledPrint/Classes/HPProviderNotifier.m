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
                    NSLog(@"\n\nHPProviderNotifier: Response code = %ld\n\n", (long)statusCode);
                    return;
                } else {
                    NSLog(@"\n\nHPProviderNotifier: Provider was successfully notified.\n\n");
                }
            }
        } else {
            NSLog(@"\n\nHPProviderNotifier: Unable to notify Provider of print status. Connection error = %@\n\n", connectionError);
        }
    });
}

@end
