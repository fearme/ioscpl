

#import "HPAsyncHttpGet.h"

@implementation HPAsyncHttpGet

- (instancetype)init
{
    self = [super init];
    if (self){
        self.httpMethod = HttpGet;
    }
    return self;
}

- (void)execute:(NSString *)url
{
    // Create the request.
    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:url]];
    
    // Create url connection and fire request
    NSURLConnection *conn = [[NSURLConnection alloc] initWithRequest:request delegate:self];
}

@end
