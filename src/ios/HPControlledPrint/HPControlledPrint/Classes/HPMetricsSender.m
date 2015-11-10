//
//  HPMetricsSender.m
//  HPControlledPrint
//
//  Created by Fredy on 7/8/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

#import "HPMetricsSender.h"
#import "HPAsyncHttpPost.h"
#import "HPPrintJobRequest.h"


@implementation HPMetricsSender

- (void)send:(NSString *)url withPrintJobRequest:(HPPrintJobRequest *)printJobRequest forOperation:(NSString *)operation forReason:(NSString *)reason forState:(NSString *)state metricsType:(NSString *)metricsType
{
    NSString *blobType = metricsType;
    NSString *application = @"CP_SDK";
    NSString *origin = @"IOS";
    
    NSString *reasonElement = (reason == nil) ? @"" : [NSString stringWithFormat:@"<met:reason>%@</met:reason>", reason];
    NSString *stateElement = (state == nil) ? @"" : [NSString stringWithFormat:@"<met:blob>%@</met:blob>", state];
    
    NSDateFormatter* formatter = [[NSDateFormatter alloc] init];
    [formatter setDateFormat:@"yyyy-MM-dd'T'hh:mm:ss"];
    NSDate *date = [[NSDate alloc] init];
    NSString *metricsTimestamp = [formatter stringFromDate:date];
    
    //We need an HPPrintJobRequest for these
    NSString *hardwareElement = @""; //used as the UserID
    NSString *tokenElement = @"";
    NSString *providerElement = @"";
    NSMutableString *offerIdElement = [NSMutableString stringWithString:@""];
    if (printJobRequest != nil) {
        hardwareElement = printJobRequest.hardwareId == nil ? @"" : [NSString stringWithFormat:@"<met:userId>%@</met:userId>", printJobRequest.hardwareId];
        tokenElement = [NSString stringWithFormat:@"<met:url>%@</met:url>", printJobRequest.tokenId];
        providerElement = [NSString stringWithFormat:@"<met:vendorId>%@</met:vendorId>", [printJobRequest providerAsString]];
        
        //construct offer IDs
        if (printJobRequest.assetIds != nil) {
            offerIdElement = [[NSMutableString alloc] initWithString:@""];
            for (id offerId in printJobRequest.assetIds) {
                [offerIdElement appendString:@"<met:items>"];
                [offerIdElement appendString:(NSString *)offerId];
                [offerIdElement appendString:@"</met:items>"];
            }
        } else {
            offerIdElement = [[NSMutableString alloc] initWithString:@""]; //If we don't have any items, we can't send the element
        }
        //NSLog(@"offerIdStr: %@ ^^^^^^^^^^^^^^^^^^^^^^^^^", offerIdStr);
    }
    
    NSString *metricsXml = [NSString stringWithFormat:@"<xml-fragment xmlns:met=\"http://hp.com/sips/services/xml/metrics\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">%@<met:application>%@</met:application><met:operation>%@</met:operation><met:timestamp>%@</met:timestamp>%@%@<met:blobType>%@</met:blobType>%@%@%@<met:origin>%@</met:origin></xml-fragment>", hardwareElement, application, operation, metricsTimestamp, tokenElement, providerElement, blobType, stateElement, reasonElement, offerIdElement, origin];

    // Create the request.
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:url]];
    request.HTTPMethod = @"POST";
    [request setValue:@"text/xml; charset=utf-8" forHTTPHeaderField:@"Content-Type"];

    NSData *requestBodyData = [metricsXml dataUsingEncoding:NSUTF8StringEncoding];
    request.HTTPBody = requestBodyData;
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{    NSURLResponse *response = nil;
        NSError *connectionError = nil;
        NSData *responseData = [NSURLConnection sendSynchronousRequest:request returningResponse:&response error:&connectionError];
        
        if (connectionError == nil) {
            
            NSLog(@"\n\nMetricsXml: %@\n\n", metricsXml);
            
            if ([response isKindOfClass:[NSHTTPURLResponse class]]) {
                //NSLog(@"Response description: %@", [(NSHTTPURLResponse *)response description]);
                NSInteger statusCode = [(NSHTTPURLResponse *)response statusCode];
                if (statusCode != 200) {
                    NSLog(@"\n\nHPMetricsSender METRICS:  Response code = %ld\n\n", (long)statusCode);
                    return;
                } else {
                    NSLog(@"\n\nHPMetricsSender: The metric was successfully sent.\n\n");
                }
            }
        } else {
            NSLog(@"\n\nHPMetricsSender METRICS:  Connection error = %@\n\n", connectionError);
        }
    });
}


@end
