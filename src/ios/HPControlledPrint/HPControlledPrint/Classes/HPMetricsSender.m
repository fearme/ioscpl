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

@interface HPMetricsSender () <HPAsyncHttpDelegate>

@end


@implementation HPMetricsSender

- (void)send:(NSString *)url withPrintJobRequest:(HPPrintJobRequest *)printJobRequest forOperation:(NSString *)operation metricsType:(NSString *)metricsType
{
    NSString *blob = @"Print Request Initiated";
    NSString *blobType = metricsType;
    NSString *application = @"CP_SDK";
    NSString *origin = @"IOS";
    NSString *hardwareId = printJobRequest.hardwareId == nil ? @"" : printJobRequest.hardwareId; //used as the UserID
    
    NSDateFormatter* formatter = [[NSDateFormatter alloc] init];
    [formatter setDateFormat:@"yyyy-MM-dd'T'hh:mm:ss"];
    NSDate *date = [[NSDate alloc] init];
    NSString *metricsTimestamp = [formatter stringFromDate:date];
    
    //construct offer IDs
    NSMutableString *offerIdStr;
    if (printJobRequest.assetIds != nil) {
        offerIdStr = [[NSMutableString alloc] initWithString:@""];
        for (id offerId in printJobRequest.assetIds) {
            [offerIdStr appendString:@"<met:items>"];
            [offerIdStr appendString:(NSString *)offerId];
            [offerIdStr appendString:@"</met:items>"];
        }
    } else {
        offerIdStr = [[NSMutableString alloc] initWithString:@""]; //If we don't have any items, we can't send the element
    }
    //NSLog(@"offerIdStr: %@ ^^^^^^^^^^^^^^^^^^^^^^^^^", offerIdStr);
    
    NSString *metricsXml = [NSString stringWithFormat:@"<xml-fragment xmlns:met=\"http://hp.com/sips/services/xml/metrics\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"><met:userId>%@</met:userId><met:application>%@</met:application><met:operation>%@</met:operation><met:timestamp>%@</met:timestamp><met:url>%@</met:url><met:vendorId>%@</met:vendorId><met:blobType>%@</met:blobType><met:blob>%@</met:blob>%@<met:origin>%@</met:origin></xml-fragment>", hardwareId, application, operation, metricsTimestamp, printJobRequest.tokenId, [printJobRequest providerAsString], blobType, blob, offerIdStr, origin];
    NSLog(@"metricsXml: %@ ^^^^^^^^^^^^^^^^^^^^^^^^^", metricsXml);

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
            if ([response isKindOfClass:[NSHTTPURLResponse class]]) {
                //NSLog(@"Response description: %@", [(NSHTTPURLResponse *)response description]);
                NSInteger statusCode = [(NSHTTPURLResponse *)response statusCode];
                if (statusCode != 200) {
                    NSLog(@"HPMetricsSender METRICS:  Response code = %ld", (long)statusCode);
                    return;
                } else {
                    NSLog(@"HPMetricsSender: The metric was successfully sent.");
                }
            }
        } else {
            NSLog(@"HPMetricsSender METRICS:  Connection error = %@", connectionError);
        }
    });
    
    //HPAsyncHttpPost *httpPost = [[HPAsyncHttpPost alloc] init];
    //httpPost.delegate = self;
    //[httpPost execute:url withData:metricsXml withContentType:@"application/xml; charset=utf-8"];
}

#pragma mark - HPAsyncHttpDelegate

- (void)asyncHttpSucceeded: (NSData *)data
{
    NSLog(@"SUCCEEDED in sending print job metrics ====================");
}

- (void)asyncHttpErrored:(NSError *)error
{
    NSLog(@"FAILED in sending print job metrics ====================");
}

@end
