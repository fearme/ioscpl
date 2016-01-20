//
//  HPTokenValidator.m
//  HPControlledPrint
//
//  Created by Fredy on 8/24/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

#import "HPTokenValidator.h"
#import "HPAsyncHttpGet.h"


@interface HPTokenValidator () <HPAsyncHttpDelegate, NSURLConnectionDelegate>
@property (strong, nonatomic) NSMutableData *responseData;
@property (strong, nonatomic) NSString *token;
@end


@implementation HPTokenValidator

HttpGetCompletion validationCompletion;

- (void)validate:(NSString *)token withServiceUrl:(NSString *)serviceUrl withCompletion:(HttpGetCompletion)completion
{
    self.token = token;
    validationCompletion = completion;
    
    
    // Create the request.
    NSString *validationCall = [NSString stringWithFormat:@"%@?tokenId=%@", serviceUrl, token];
    NSLog(@"Validation URL: %@", validationCall);
    NSLog(@"Token to Validate: %@", token);
    
    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:validationCall]];
    
    // Create url connection and fire request
    NSURLConnection *conn = [[NSURLConnection alloc] initWithRequest:request delegate:self];
}




- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response {
    // A response has been received, this is where we initialize the instance var you created
    // so that we can append data to it in the didReceiveData method
    // Furthermore, this method is called each time there is a redirect so reinitializing it
    // also serves to clear it
    self.responseData = [[NSMutableData alloc] init];
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data {
    // Append the new data to the instance variable you declared
    [self.responseData appendData:data];
}

- (NSCachedURLResponse *)connection:(NSURLConnection *)connection
                  willCacheResponse:(NSCachedURLResponse*)cachedResponse {
    // Return nil to indicate not necessary to store a cached response for this connection
    return nil;
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection {
    // The request is complete and data has been received
    // You can parse the stuff in your instance variable now
    
    NSData *nsData = [NSData dataWithData:self.responseData];
    [self asyncHttpSucceeded:nsData];
    
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error {
   [self asyncHttpErrored:error];
}




#pragma mark - HPAsyncHttpDelegate

- (void)asyncHttpSucceeded: (NSData *)data
{
    NSError* error;
    NSDictionary* dict = [NSJSONSerialization JSONObjectWithData:data options:kNilOptions error:&error];
    if (error != nil) {
        NSLog(@"ERROR in JSON Object containing Qples token validation result !!!  - %@", error);
        return;
    }    
    NSLog(@"json: %@", dict);
    
    NSString *status = [dict objectForKey:@"status"];
    NSString *message = [dict objectForKey:@"message"];
    
    NSLog(@"Qples Token Validation result. Token: '%@' Status: %@. Message: %@", self.token, status, message);
    
    if (validationCompletion != nil){
        if ([status isEqualToString:@"success"]) {
            validationCompletion(true);
        } else {
            validationCompletion(false);
        }
    }
}

- (void)asyncHttpErrored:(NSError *)error
{
    if (validationCompletion != nil){
        validationCompletion(false);
    }
    NSLog(@"AsyncHttpGet FAILED with error: %@ ===================", error);
}

@end
