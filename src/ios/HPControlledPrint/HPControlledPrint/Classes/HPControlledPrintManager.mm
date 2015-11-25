
#import <UIKit/UIKit.h>

#import "HPControlledPrintManager.h"
#import "HPControlledPrintManagerUtil.h"
#import "HPPrinterAttributes.h"
#import "HPAsyncHttpGet.h"
#import "HPServices.h"
#import "HPMetricsSender.h"
#import "HPProviderNotifier.h"
#import "HPTokenValidator.h"
#import "GAIDictionaryBuilder.h"
#import "GoogleAnalyticsService.h"

#include "../Includes/mwp_secure_asset_printing_api.hpp"

@interface HPControlledPrintManager () <HPAsyncHttpDelegate>
@property (strong, nonatomic) void(^httpGetCompletion)(InitStatus status);
@property (strong, nonatomic) NSString *uuidHashed;
@end


@implementation HPControlledPrintManager

int const kSendPrintersIntervalSeconds = 3;

int timerInterval = 0;

NSString *const kOperationNewToken           = @"COUPON_DOCUMENT_NEW_TOKEN";
NSString *const kOperationPrintRequested     = @"COUPON_DOCUMENT_PRINT_REQUESTED";
NSString *const kOperationPrintSuccess       = @"COUPON_DOCUMENT_PRINT_SUCCESS";
NSString *const kOperationPrintFailed        = @"FAILED_PRINT_COUPON_DOCUMENT";
NSString *const kOperationPrintCanceled      = @"CANCELED_PRINT_COUPON_DOCUMENT";
NSString *const kOperationPrintDeferred      = @"COUPON_DOCUMENT_DEFERRED";

NSString *const kStatePrintRequestInitiated  = @"INITIATED";
NSString *const kStatePrintSuccess           = @"SUCCESS";
NSString *const kStatePrintCancelled         = @"CANCELLED";
NSString *const kStateNewToken               = @"NEW_TOKEN";
NSString *const kStateUnknown                = @"UNKNOWN";
NSString *const kStateNetworkError           = @"NETWORK_ERROR";
NSString *const kStateUpstreamError          = @"UPSTREAM_ERROR";

NSString *const kReasonNewTokenRegistered    = @"New Token Registered";
NSString *const kReasonPrintRequestInitiated = @"Print Request Initiated";

NSString *const kPrinterStatusPrinting       = @"PRINTING";
NSString *const kPrinterStatusIdle           = @"IDLE";
NSString *const kPrinterStatusCanceling      = @"CANCELING PRINTING";
NSString *const kPrinterStatusDone           = @"Done";
NSString *const kPrinterStatusUnknown        = @"UNKNOWN";
NSString *const kPrinterStatusNetworkError   = @"NETWORK_ERROR";
NSString *const kPrinterStatusUpstreamError  = @"UPSTREAM_ERROR";

NSString *const kMetricTypeUserData          = @"USER_DATA";
NSString *const kMetricTypeErrorMessage      = @"ERROR_MESSAGE";

NSString *const kProviderQples               = @"QPLES";

NSString *const kCaymanRootUrlDev            = @"https://cayman-dev-02.cloudpublish.com";
NSString *const kCaymanRootUrlQa             = @"https://cayman-qa.cloudpublish.com";
NSString *const kCaymanRootUrlStaging        = @"https://cayman-stg.cloudpublish.com";
NSString *const kCaymanRootUrlProduction     = @"https://cayman-prod.cloudpublish.com";


static net_mobilewebprint::secure_asset_printing_api_t *secureAssetPrinter;

HPServices *services;
HPPrinter *lastUsedPrinter;
HPPrintJobRequest *currentPrintJobRequest;
ServerStack currentServerStack;

NSString *proxyHost;
NSString *proxyPort;
NSMutableDictionary *printers;
NSTimer *sendPrinterAttributesTimer;
BOOL printerScanStarted;

- (instancetype)init
{
    self = [super init];
    if (self && secureAssetPrinter == nil) {
        secureAssetPrinter = net_mobilewebprint::sap_api();
        printers = [[NSMutableDictionary alloc] init];
        timerInterval = kSendPrintersIntervalSeconds;
        printerScanStarted = NO;
    }
    
    // The following line is used to find where the Framework file is in the file system.
    // Do not remove.
    NSBundle *bundle = [NSBundle bundleForClass:[self class]];
    NSLog(@"%@", [bundle description]);
    
    self.uuidHashed = [self hashUUID];
    
    [GoogleAnalyticsService setUpGoogleAnalyticsService];
    
//    [GAI sharedInstance].dispatchInterval = 20;
//    
//    [GAI sharedInstance].trackUncaughtExceptions = YES;
//    [[GAI sharedInstance].logger setLogLevel:kGAILogLevelVerbose];
////    self.tracker = [[GAI sharedInstance] trackerWithName:@"CuteAnimals"
////                                              trackingId:@"UA-69772755-5"];
//    
//    id<GAITracker> tracker = [[GAI sharedInstance] trackerWithTrackingId:@"UA-69772755-5"];
    
    return self;
}


- (NSString *)hashUUID
{
    NSString *uuid = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
    NSString *hashed = [HPControlledPrintManagerUtil hash:uuid withSalt:[HPControlledPrintManagerUtil salt:32]];
    NSLog(@"UUID: %@", uuid);
    NSLog(@"UUID Hashed: %@", hashed);
    NSLog(@"UUID Hashed Length: %lu", (unsigned long)[hashed length]);
    
    return hashed;
}

- (void)setScanIntervalSeconds:(int32)seconds {
    timerInterval = seconds;
}

- (int)getScanIntervalSeconds {
    return timerInterval;
}

//register_handler    (char const * name, void * app_data, hp_sap_callback_t callback);
- (BOOL)registerStatusListener //:(NSString *)listener appData:(void *)appData callback:hp_sap_callback_t callback
{
    if (sendPrinterAttributesTimer == nil) {
        sendPrinterAttributesTimer = [NSTimer scheduledTimerWithTimeInterval:timerInterval
                                                                      target:self
                                                                    selector:@selector(sendFoundPrinters)
                                                                    userInfo:nil
                                                                     repeats:YES];
    }
    return secureAssetPrinter->register_handler("app", (__bridge void *)(self), printStatusListener);
}



//- (int)printStatusListener:(void *)listenerObject message:(char const *)message ident:(int)ident
//             transactionId:(int32) transactionId p1:(uint8 const *)p1 params:(sap_params const *)params
int printStatusListener(void *listenerObject, char const *message, int ident,
                        int32 transactionId, uint8 const *p1, sap_params const *params)
{
    HPControlledPrintManager * hpControlledPrintManager = (__bridge HPControlledPrintManager *)listenerObject;

    [hpControlledPrintManager handleMessages:message identifiedBy:ident forTransaction:transactionId withP1:p1 withParams:params];
    
   
    //printf("app_data:       %s\n", (const char*)listenerObject);
    //printf("msg:            %s\n", message);
    //printf("ident:          %d\n", ident);
    //printf("transaction_id: %d\n", transactionId);
    //printf("p1:             %s\n", p1);
    //printf("params:         %s\n\n\n", (const char*)params);
    
    return 0;
}


- (void)handleMessages:(char const*)message identifiedBy:(int)ident forTransaction:(int32)transactionId
               withP1:(uint8 const *)p1_ withParams:(sap_params const *)params
{
    char const * p1 = (p1_ != NULL ? (char const *)p1_ : "");
    char const * p2 = (params != NULL ? params->p2 != NULL ? (char const *)params->p2 : "" : "");
    char const * p3 = (params != NULL ? params->p3 != NULL ? (char const *)params->p3 : "" : "");
    
    //printf("cb: %s id:%d, tid:%d p1:%s p2:%s p3:%s p4:%s p5:%s n1:%d n2:%d n3:%d n4:%d n5:%d params:%x\n", message, ident, (int)transactionId, p1, p2, p3, p4, p5, n1, n2, n3, n4, n5, params);

    //begin_printer_changes
    //Doc URL: https://www.dropbox.com/s/o9m66y4h67yfwmq/Avatar1.jpg?dl=0
    //end_printer_enum
    //g for printers
    //log_d
    //log_w
    //print_progress
    //Printer IP: 15.80.127.89
    //printer_attribute
    //recd_register_handler
    //Scanninlog_d
    
    NSString *stringMessage = [NSString stringWithCString:message encoding:NSASCIIStringEncoding];
    
    if ([stringMessage isEqualToString:@"printer_attribute"]) {
        
        NSString *printerIp = [NSString stringWithUTF8String:p1];
        HPPrinterAttributes *attributes = [printers objectForKey:printerIp];
        if (attributes == nil) {
            attributes = [[HPPrinterAttributes alloc] init];
            attributes.ip = printerIp;
        }
        
        NSString *property = [NSString stringWithUTF8String:p2];
        NSString *value = [NSString stringWithUTF8String:p3];
        NSLog(@"%@ - %@: %@", printerIp, property, value);
        
        if ([property isEqualToString:@"name"]) {
            attributes.name = value;
            
        } else if ([property isEqualToString:@"mac"]) {
            attributes.macAddress = value;
            
        } else if ([property isEqualToString:@"status"]) {
            attributes.status = value;
            
        } else if ([property isEqualToString:@"MFG"]) {
            attributes.manufacturer = value;
            
        } else if ([property isEqualToString:@"is_supported"]) {
            if ([value isEqualToString:@"1"]) {
                attributes.isSupported = YES;
            } else {
                attributes.isSupported = NO;
            }
            attributes._supportedFlagIsSet = YES;
        }
        
        //printf("%s: %s = %s\n", p1, p2, p3);
        [printers setObject:attributes forKey:printerIp];

    } else if ([stringMessage isEqualToString:@"print_progress"]) {
        //Notes on parameter values
        //  p1: human readable but for developer use
        //  p2: message for user
        //  p3: exact status string from printer
        //  p4: job id
        //  p5: raw string from printer or server

        //NSString *_p1 = [NSString stringWithCString:p1 encoding:NSASCIIStringEncoding];
        NSString *_p2 = [NSString stringWithCString:p2 encoding:NSASCIIStringEncoding];
        NSString *_p3 = [NSString stringWithCString:p3 encoding:NSASCIIStringEncoding];
        //NSString *_p4 = [NSString stringWithCString:p4 encoding:NSASCIIStringEncoding];
        //NSString *_p5 = [NSString stringWithCString:p5 encoding:NSASCIIStringEncoding];
        //NSString *_p2 = [NSString stringWithUTF8String:p2];
        
        if (!([_p3 isEqualToString:kPrinterStatusPrinting] || [_p3 isEqualToString:kPrinterStatusIdle] || [_p3 isEqualToString:kPrinterStatusCanceling])) {
            _p2 = [NSString stringWithFormat:@"%@ %@", _p2, _p3];// [_p2 stringByAppendingString:_p3];
        }
        
        [self sendPrinterStatus:_p2];
       
        [self sendFinalPrintJobMetrics:currentPrintJobRequest withUserVisibleStatus:_p2 withPrinterStatus:_p3];
        
        [self notifyProviderOfPrintStatus:currentPrintJobRequest withUserVisibleStatus:_p2 withPrinterStatus:_p3];
        
    } else if ([stringMessage isEqualToString:@"begin_new_printer_list"]) {
        [printers removeAllObjects];
    }
}

- (void)notifyProviderOfPrintStatus:(HPPrintJobRequest *)printJob withUserVisibleStatus:(NSString *)userVisibleStatus withPrinterStatus:(NSString *)printerStatus
{
    NSLog(@"userVisibleStatus: %@", userVisibleStatus);
    NSLog(@"printerStatus: %@", printerStatus);
    
    
    //IMPORTANT: The order of the if conditions matters. Do not change.
    //  The reason is that when a print job is cancelled, Mario sends a canceling status followed by a
    //  "success" status (which is "Done" + "Idle"). During a print cancel, if we check for
    //  "success" first, we never process the cancel status.
    if (!printJob.providerNotificationSent) {
        // if canceling, unknown, or network error
        if ([printerStatus isEqualToString:kPrinterStatusCanceling] || [printerStatus isEqualToString:kPrinterStatusUnknown] || [printerStatus isEqualToString:kPrinterStatusNetworkError]) {
            HPProviderNotifier *notifier = [[HPProviderNotifier alloc] init];
            [notifier sendPrintStatus:@"error" notifyUrl:services.notifyQples  tokenId:printJob.tokenId serverStack:[self serverStackAsString]];
            printJob.providerNotificationSent = YES;
            
        // if success or upstream error
        } else if ( ([userVisibleStatus isEqualToString:kPrinterStatusDone] && [printerStatus isEqualToString:kPrinterStatusIdle]) || [printerStatus isEqualToString:kPrinterStatusUpstreamError] ) {
            HPProviderNotifier *notifier = [[HPProviderNotifier alloc] init];
            [notifier sendPrintStatus:@"success" notifyUrl:services.notifyQples  tokenId:printJob.tokenId serverStack:[self serverStackAsString]];
            printJob.providerNotificationSent = YES;
        }
    }
}

- (void)sendFinalPrintJobMetrics:(HPPrintJobRequest *)printJob withUserVisibleStatus:(NSString *)userVisibleStatus withPrinterStatus:(NSString *)printerStatus
{
    // We'll only send these particular print metrics once.
    if (!printJob.finalPrintStatusMetricSent) {
        
        HPMetricsSender *metricsSender = [[HPMetricsSender alloc] init];
        
        // if canceling
        if ([printerStatus isEqualToString:kPrinterStatusCanceling]) {
            [metricsSender send:services.postMetricsUrl withPrintJobRequest:printJob forHardwarId:self.uuidHashed forOperation:kOperationPrintCanceled forReason:userVisibleStatus forState:kStatePrintCancelled metricsType:kMetricTypeUserData];
            printJob.finalPrintStatusMetricSent = YES;
            NSLog(@"%@ metric sent", printerStatus);
            
        // if unknown or netork error
        } else if ([printerStatus isEqualToString:kPrinterStatusUnknown] || [printerStatus isEqualToString:kPrinterStatusNetworkError]) {
            NSString *state;
            if ([printerStatus isEqualToString:kPrinterStatusUnknown]) {
                state = kStateUnknown;
            } else {
                state = kStateNetworkError;
            }
            [metricsSender send:services.postMetricsUrl withPrintJobRequest:printJob forHardwarId:self.uuidHashed forOperation:kOperationPrintFailed forReason:userVisibleStatus forState:state metricsType:kMetricTypeErrorMessage];
            printJob.finalPrintStatusMetricSent = YES;
            NSLog(@"%@ metric sent", printerStatus);
            
        // if success or upstream error are treated as success
        } else if ( ([userVisibleStatus isEqualToString:kPrinterStatusDone] && [printerStatus isEqualToString:kPrinterStatusIdle]) || [printerStatus isEqualToString:kPrinterStatusUpstreamError] ) {
            NSString *state;
            if ([printerStatus isEqualToString:kPrinterStatusUpstreamError]) {
                state = kStateUpstreamError;
            } else {
                state = kStatePrintSuccess;
            }
            [metricsSender send:services.postMetricsUrl withPrintJobRequest:printJob forHardwarId:self.uuidHashed forOperation:kOperationPrintSuccess forReason:userVisibleStatus forState:state metricsType:kMetricTypeUserData];
            printJob.finalPrintStatusMetricSent = YES;
            NSLog(@"SUCCESS metric sent");
        }
    }
}

- (void)sendFoundPrinters
{
    if ([[self printerAttributesDelegate] respondsToSelector:@selector(didReceivePrinters:)]) {
        //make sure we only send printers with IPs, names, and isSupported set
        HPDiscoveredPrinters *printersToSend = [[HPDiscoveredPrinters alloc] init];
        for (NSString *ip in printers) {
            HPPrinterAttributes *printer = [printers objectForKey:ip];
            if (printer.ip != nil && printer.name != nil && printer._supportedFlagIsSet) {
                [printersToSend.printers setObject:printer forKey:ip];
            }
        }
        [self.printerAttributesDelegate didReceivePrinters:printersToSend];
    }
}

- (void)sendPrinterStatus:(NSString *)status
{
    if ([[self printerAttributesDelegate] respondsToSelector:@selector(didReceivePrintJobStatus:)]) {
        [self.printerAttributesDelegate didReceivePrintJobStatus:status];
    }
}

- (NSString *)serverStackAsString
{
    @try {
        NSString *stack;
        if (currentServerStack == ServerStackDevelopment) {
            stack = @"DEV";
        } else if (currentServerStack == ServerStackQa) {
            stack = @"QA";
        } else if (currentServerStack == ServerStackStaging) {
            stack = @"STG";
        } else {
            stack = @"PROD";
        }
        
        return stack;
    }
    @catch (NSException *exception) {
        NSLog(@"Exception: %@", exception.reason);
        [GoogleAnalyticsService trackException:exception withHwId:@"HarshHwId"];
    }
}

- (void)proxy:(NSString *)host onPort:(NSString *)port
{
    proxyHost = host;
    proxyPort = port;
}

#pragma mark - initialize:

- (void)initialize: (ServerStack)stack withCompletion:(void (^)(InitStatus status))completion
{
//    id<GAITracker> tracker = [[GAI sharedInstance] defaultTracker];

//    NSMutableDictionary *event =
//    [[GAIDictionaryBuilder createEventWithCategory:@"ControlledPrintIos"
//                                            action:@"Initialized"
//                                             label:@"DefaultTracker2"
//                                             value:nil] build];
//    [[GAI sharedInstance].defaultTracker send:event];
//    [[GAI sharedInstance] dispatch];
    
//     id<GAITracker> tracker = [[GAI sharedInstance] trackerWithTrackingId:@"UA-69772755-5"];
//
//    [tracker send:[[GAIDictionaryBuilder createEventWithCategory:@"ControlledPrintIos"
//                                                          action:@"Initialized"
//                                                           label:@"NotDefaultTracker2"
//                                                           value:nil] build]];
    
    [GoogleAnalyticsService trackScreenView:@"ControlledPrintIosScreen" withHwId:@"HarshHwId"];
    
    [GoogleAnalyticsService trackEventCategory:@"ControlledPrintIos" withAction:@"Initialized" andLabel:@"NotDefaultWithGaServiceClass"];
    
    currentServerStack = stack;
    [self setEnvironment: ^(InitStatus status){
        if (completion){
            completion(status);
        }
    }];
}

- (NSString *) caymanRootUrl
{
    NSString *rootUrl;
    if (currentServerStack == ServerStackDevelopment) {
        rootUrl = kCaymanRootUrlDev;
        NSLog(@"Server Stack: Dev ======================");
    } else if (currentServerStack == ServerStackQa) {
        rootUrl = kCaymanRootUrlQa;
        NSLog(@"Server Stack: QA ======================");
    } else if (currentServerStack == ServerStackStaging) {
        rootUrl = kCaymanRootUrlStaging;
        NSLog(@"Server Stack: Staging ======================");
    } else if (currentServerStack == ServerStackProduction) {
        rootUrl = kCaymanRootUrlProduction;
        NSLog(@"Server Stack: Production ======================");
    }
    return rootUrl;
}

- (void)setEnvironment: (void (^)(InitStatus status))completion
{
    NSString *discoveryUrl = [NSString stringWithFormat:@"%@/coupons/discovery", [self caymanRootUrl]];
    NSLog(@"Discovery URL: %@", discoveryUrl);
    
    [self fetchDiscoveredProperties:discoveryUrl withCompletion:^(InitStatus status){
        if (completion != nil) {
            completion(status);
        }
    }];
}

- (void)fetchDiscoveredProperties: (NSString *)discoveryUrl withCompletion:(void (^)(InitStatus status))completion
{
    self.httpGetCompletion = completion;
    HPAsyncHttpGet *httpGet = [[HPAsyncHttpGet alloc] init];
    httpGet.delegate = self;
    [httpGet execute:discoveryUrl];
}

#pragma mark - validateToken:

- (void)validateToken:(NSString *)token withCompletion:(void (^)(BOOL valid))completion
{
    NSLog(@"\n\nMetrics URL: %@\n\n", services.postMetricsUrl);
    HPMetricsSender *metricsSender = [[HPMetricsSender alloc] init];
    HPPrintJobRequest *tempRequest = [[HPPrintJobRequest alloc] init];
    [metricsSender send:services.postMetricsUrl withPrintJobRequest:tempRequest forHardwarId:self.uuidHashed forOperation:kOperationNewToken forReason:kReasonNewTokenRegistered forState:kStateNewToken metricsType:kMetricTypeUserData];
    
    HPTokenValidator *validator = [[HPTokenValidator alloc] init];    
    [validator validate:token withServiceUrl:services.validateQplesToken withCompletion:^(BOOL valid) {
        if (completion) {
            completion(valid);
        }
    }];
}

#pragma mark - scanForPrinters()

- (BOOL)scanForPrinters  //:(BOOL)start blockingScan:(BOOL)blocking
{
    //Telling mario to scan after it is already scanning will cause a crash. So we only tell it to scan once
    if (!printerScanStarted) {

        [self registerStatusListener];
        
        //    hp_sap_set_flag("vvvverbose", true);
        //    hp_sap_set_flag("quiet", false);
        
        if (proxyHost != nil && proxyPort != nil) {
            const char *cHost = [proxyHost cStringUsingEncoding:NSASCIIStringEncoding];
            const char *cPort = [proxyPort cStringUsingEncoding:NSASCIIStringEncoding];
            hp_sap_set_option("http_proxy_name", cHost); //test: "proxy.atlanta.hp.com"
            hp_sap_set_option("http_proxy_port", cPort); //test: "8080"
        }
        printerScanStarted = secureAssetPrinter->net_mobilewebprint::core_api_t::start(YES, NO);
    }
    return printerScanStarted;
}

#pragma mark - print()

- (BOOL)print: (HPPrinter *)selectedPrinter withJobRequest:(HPPrintJobRequest *)printJobRequest
{
    if (selectedPrinter == nil || printJobRequest == nil) {
        return NO;
    }

    if (printJobRequest.providerId == ProviderQples) {
        secureAssetPrinter->set_option("providerName", kProviderQples);
    } else { //we currently only support Qples
        return NO;
    }
    
    lastUsedPrinter = selectedPrinter;
    currentPrintJobRequest = printJobRequest;
    
    NSString *tokenId;
    if (printJobRequest.providerId == ProviderQples && printJobRequest.tokenId != nil) {
        tokenId = printJobRequest.tokenId; //this is the unique ID to the Qples coupon from which Qples will generate the PDF
    } else {
        tokenId = @"Test Token ID";
        
        //the test token can also be passed through printJobRequest.tokenId. Thus:
        //tokenId = printJobRequest.tokenId;;
    }
    NSLog(@"tokenId: %@", tokenId);
    
    [self sendPrintJob:tokenId toPrinterIp:selectedPrinter.ipAddress];
    
    HPMetricsSender *metricsSender = [[HPMetricsSender alloc] init];
    [metricsSender send:services.postMetricsUrl withPrintJobRequest:printJobRequest forHardwarId:self.uuidHashed forOperation:kOperationPrintRequested forReason:kReasonPrintRequestInitiated forState:kStatePrintRequestInitiated metricsType:kMetricTypeUserData];
    
    return YES;
}

- (BOOL)sendPrintJob:(NSString *)url toPrinterIp:(NSString *) printerIp
{
    const char *cUrl = [url cStringUsingEncoding:NSASCIIStringEncoding];
    const char *cPrinterIp = [printerIp cStringUsingEncoding:NSASCIIStringEncoding];
    return secureAssetPrinter->send_job(cUrl, cPrinterIp);
}

#pragma mark - exit()

- (BOOL)exit
{
    //we currently do nothing except log this function call
    NSLog(@"exit() called");
    
    return YES;
}

#pragma mark - HPAsyncHttpDelegate

- (void)asyncHttpSucceeded: (NSData *)data
{
    NSError* error;
    NSDictionary* dict = [NSJSONSerialization JSONObjectWithData:data options:kNilOptions error:&error];
    if (error != nil) {
        NSLog(@"ERROR in JSON Object containing Cayman services information !!!  - %@", error);
        services = nil;
    } else {
        services = [HPControlledPrintManagerUtil parseDiscoveryData:dict secureAssetPrint:secureAssetPrinter caymanRootUrl:[self caymanRootUrl]];
    }
    

    
    if (self.httpGetCompletion != nil) {
        if (services == nil) {
            self.httpGetCompletion(InitStatusServerStackNotAvailable);

        } else {
            self.httpGetCompletion(InitStatusServerStackAvailable);
        }
        self.httpGetCompletion = nil; //clear it after using it
    }    
}

- (void)asyncHttpErrored:(NSError *)error
{
    if (self.httpGetCompletion != nil) {
        self.httpGetCompletion(InitStatusServerStackAvailable);
        self.httpGetCompletion = nil; //clear it after using it
    }
    NSLog(@"FAILED in retrieving Cayman services info !!! ====================");
}


@end
