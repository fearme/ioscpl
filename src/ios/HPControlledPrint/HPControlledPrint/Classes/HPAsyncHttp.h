

#import <Foundation/Foundation.h>


@protocol HPAsyncHttpDelegate;
@protocol HPAsyncHttpDelegate <NSObject>
- (void)asyncHttpSucceeded: (NSData *)data;
- (void)asyncHttpErrored:(NSError *)error;
@end


@interface HPAsyncHttp : NSObject

typedef enum {
    HttpGet,
    HttpPost
} HttpMethod;

@property (nonatomic, weak) id<HPAsyncHttpDelegate> delegate;
@property (assign) HttpMethod httpMethod;

@end


