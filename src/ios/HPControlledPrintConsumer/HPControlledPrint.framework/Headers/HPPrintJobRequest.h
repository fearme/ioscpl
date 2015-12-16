

#import <Foundation/Foundation.h>


@interface HPPrintJobRequest : NSObject

typedef enum {
    ProviderQples
} Provider;


// Required
@property (strong, nonatomic) NSString *tokenId; //The identifier of the coupon
@property (assign) Provider providerId; //The identifier for the asset provider

// Flag for whether the final print status metric of print job was sent
@property (assign, nonatomic) BOOL finalPrintStatusMetricSent;
// Flag for whether the print status notification was sent
@property (assign, nonatomic) BOOL providerNotificationSent;


// These properties are to support HPCP, such as, for creating asset page(s) -------------------

//Array of NSNumber IDs for the assets that are to be included in the asset page(s)
@property (strong, nonatomic) NSArray *assetIds;

//The identifier for how to format the asset page(s)
@property (strong, nonatomic) NSString *pageFormatId;

//Flag indicating if exclusion logic should be applied when including the assets.
//  For example, it may keep track of already printed assets for this hwId
@property (assign, nonatomic) BOOL applyExclusion;

//URL to the asset page(s) to print, which is accessible to the lib
@property (strong, nonatomic) NSURL *assetURL;

//This is the basic AUTH user name
@property (strong, nonatomic) NSString *providerIdentification;

//This is the basic AUTH password
@property (strong, nonatomic) NSString *providerAuthentication;
//----------------------------------------------------------------------------------------------


- (NSString *)providerAsString;

@end




