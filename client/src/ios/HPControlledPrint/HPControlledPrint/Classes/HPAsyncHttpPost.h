

#import "HPAsyncHttp.h"

@interface HPAsyncHttpPost : HPAsyncHttp

- (void)execute:(NSString *)url withData:(NSString *)stringData withContentType:(NSString *)contentType;

@end
