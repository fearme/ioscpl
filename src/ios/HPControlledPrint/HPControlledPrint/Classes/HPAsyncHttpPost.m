//
//  HPAsyncHttpPost.m
//  HPControlledPrint
//
//  Created by Fredy on 7/8/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

#import "HPAsyncHttpPost.h"

@implementation HPAsyncHttpPost

- (instancetype)init
{
    self = [super init];
    if (self){
        self.httpMethod = HttpPost;
    }
    return self;
}

- (void)execute:(NSString *)url withData:(NSString *)stringData withContentType:(NSString *)contentType
{
    // Create the request.
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:url]];
    request.HTTPMethod = @"POST";
    [request setValue:contentType forHTTPHeaderField:@"Content-Type"];
    
    NSData *requestBodyData = [stringData dataUsingEncoding:NSUTF8StringEncoding];
    request.HTTPBody = requestBodyData;
    
    NSString *bodyLength = [NSString stringWithFormat: @"%ld", (long)[requestBodyData length]];
    [request addValue:bodyLength forHTTPHeaderField: @"Content-Length"];
    
    // Create url connection and fire request
    NSURLConnection *conn = [[NSURLConnection alloc] initWithRequest:request delegate:self];
}

@end
