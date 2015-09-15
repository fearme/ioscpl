
#import "HPControlledPrintManager.h"
#import "HPPrinterAttributes.h"
#import "HPAsyncHttpGet.h"
#import "HPServices.h"
#import "HPMetricsSender.h"
#import "HPTokenValidator.h"


#include "../Includes/mwp_secure_asset_printing_api.hpp"

@interface HPControlledPrintManager () <HPAsyncHttpDelegate>
@property (strong, nonatomic) void(^httpGetCompletion)(InitStatus status);
@end


@implementation HPControlledPrintManager

int const kSendPrintersIntervalSeconds = 3;

int timerInterval = 0;
NSString *const kStatusCouponPrintRequested = @"COUPON_DOCUMENT_PRINT_REQUESTED";
NSString *const kStatusCouponPrintSuccess   = @"COUPON_DOCUMENT_PRINT_SUCCESS";
NSString *const kStatusCouponPrintFailed    = @"FAILED_PRINT_COUPON_DOCUMENT";
NSString *const kStatusCouponPrintCanceled  = @"CANCELED_PRINT_COUPON_DOCUMENT";
NSString *const kStatusCouponPrintDeferred  = @"COUPON_DOCUMENT_DEFERRED";

NSString *const kPrinterPrinting   = @"PRINTING";
NSString *const kPrinterIdle       = @"IDLE";
NSString *const kPrinterCanceling  = @"CANCELING PRINTING";
NSString *const kPrinterDone       = @"Done";

NSString *const kProviderQples     = @"QPLES";

static net_mobilewebprint::secure_asset_printing_api_t *secureAssetPrinter;

HPServices *services;
HPPrinter *lastUsedPrinter;
HPPrintJobRequest *currentPrintJobRequest;

NSString *proxyHost;
NSString *proxyPort;
NSMutableDictionary *printers;
NSTimer *sendPrinterAttributesTimer;
BOOL printerScanStarted;

- (instancetype)init
{
    self = [super init];
    if (self && secureAssetPrinter == nil){
        secureAssetPrinter = net_mobilewebprint::sap_api();
        printers = [[NSMutableDictionary alloc] init];
        timerInterval = kSendPrintersIntervalSeconds;
        printerScanStarted = NO;
    }
    
    // The following line is used to find where the Framework file is in the file system.
    // Do not remove.
    NSBundle *bundle = [NSBundle bundleForClass:[self class]];
    NSLog(@"%@", [bundle description]);
    
    return self;
}

- (void)setScanIntervalSeconds:(int32)seconds {
    timerInterval = seconds;
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
    char const * p4 = (params != NULL ? params->p4 != NULL ? (char const *)params->p4 : "" : "");
    char const * p5 = (params != NULL ? params->p5 != NULL ? (char const *)params->p5 : "" : "");
    
    int n1 = (params != NULL ? params->n1 : 0);
    int n2 = (params != NULL ? params->n2 : 0);
    int n3 = (params != NULL ? params->n3 : 0);
    int n4 = (params != NULL ? params->n4 : 0);
    int n5 = (params != NULL ? params->n5 : 0);
    
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
        }
        attributes.ip = printerIp;
        
        NSString *property = [NSString stringWithUTF8String:p2];
        NSString *value = [NSString stringWithUTF8String:p3];
        NSLog(@"property: %@ --- value: %@", property, value);
        
        if ([property isEqualToString:@"name"]) {
            attributes.name = value;
            
        } else if ([property isEqualToString:@"mac"]) {
            attributes.macAddress = value;
            
        } else if ([property isEqualToString:@"status"]) {
            attributes.status = value;
            
        } else if ([property isEqualToString:@"MFG"]) {
            attributes.manufacturer = value;
            
        } else if ([property isEqualToString:@"is_supported"]) {
            if ([value isEqualToString:@"true"]) {
                attributes.isSupported = YES;
            } else {
                attributes.isSupported = NO;
            }
        }
        
        [printers setObject:attributes forKey:printerIp];
        
        //printf("%s: %s = %s\n", p1, p2, p3);

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
        
        if (!([_p3 isEqualToString:kPrinterPrinting] || [_p3 isEqualToString:kPrinterIdle] || [_p3 isEqualToString:kPrinterCanceling])) {
            _p2 = [NSString stringWithFormat:@"%@ %@", _p2, _p3];// [_p2 stringByAppendingString:_p3];
        }
        
        [self sendPrinterStatus:_p2];
       
        [self sendFinalPrintJobMetrics:currentPrintJobRequest withUserVisibleStatus:_p2 withPrinterStatus:_p3];
        
        
        //printf("cb: %s id:%d, tid:%d p1:%s p2:%s p3:%s p4:%s p5:%s n1:%d n2:%d n3:%d n4:%d n5:%d params:%x\n", message, ident, (int)transactionId, p1, p2, p3, p4, p5, n1, n2, n3, n4, n5, params);

    } else {
        //printf("cb: %s id:%d, tid:%d p1:%s p2:%s p3:%s p4:%s p5:%s n1:%d n2:%d n3:%d n4:%d n5:%d params:%x\n", message, ident, (int)transactionId, p1, p2, p3, p4, p5, n1, n2, n3, n4, n5, params);
    }
    
}

- (void)sendFinalPrintJobMetrics:(HPPrintJobRequest *)printJob withUserVisibleStatus:(NSString *)userVisibleStatus withPrinterStatus:(NSString *)printerStatus
{
    
    //we'll only send these particular print metrics once
    if (currentPrintJobRequest.finalStatus == nil) {
        HPMetricsSender *metricsSender = [[HPMetricsSender alloc] init];
        if ([printerStatus isEqualToString:kPrinterCanceling]) {
            [metricsSender send:services.postMetricsUrl withPrintJobRequest:currentPrintJobRequest forOperation:kStatusCouponPrintCanceled];
            currentPrintJobRequest.finalStatus = kStatusCouponPrintCanceled;
            NSLog(@"CANCEL metric sent");
            
        } else if ([userVisibleStatus isEqualToString:kPrinterDone] && [printerStatus isEqualToString:kPrinterIdle]) {
            [metricsSender send:services.postMetricsUrl withPrintJobRequest:currentPrintJobRequest forOperation:kStatusCouponPrintSuccess];
            currentPrintJobRequest.finalStatus = kStatusCouponPrintSuccess;
            NSLog(@"SUCCESS metric sent");
            
        } else if ([printerStatus isEqualToString:@"Failed"]) { //TODO, need to see a fail condition. Currently, mario crashes when network is disconnected or the printer is taken off the network.
            [metricsSender send:services.postMetricsUrl withPrintJobRequest:currentPrintJobRequest forOperation:kStatusCouponPrintFailed];
            currentPrintJobRequest.finalStatus = kStatusCouponPrintFailed;
            NSLog(@"FAIL metric sent");
        }
    }
}

- (void)sendFoundPrinters
{
    
    if ([[self printerAttributesDelegate] respondsToSelector:@selector(didReceivePrinters:)]) {
        //make sure we only send printers with IPs and names
        HPDiscoveredPrinters *printersToSend = [[HPDiscoveredPrinters alloc] init];
        for (NSString *ip in printers) {
            HPPrinterAttributes *printer = [printers objectForKey:ip];
            if (printer.ip != nil && printer.name != nil) {
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

- (void)proxy:(NSString *)host onPort:(NSString *)port
{
    proxyHost = host;
    proxyPort = port;
}

- (void)validateToken:(NSString *)token withCompletion:(void (^)(InitStatus status))completion
{
    HPTokenValidator *validator = [[HPTokenValidator alloc] init];
    NSString *validationCall = [NSString stringWithFormat:@"%@?tokenId=%@", services.validateQplesToken, token];
    
    [validator validate:token withServiceUrl:validationCall withCompletion:^(BOOL success) {
        if (success) {
            completion(InitStatusTokenValid);
        } else {
            completion(InitStatusTokenInvalid);
        }
    }];
}

#pragma mark - initialize()

- (void)initialize: (ServerStack)stack withToken:(NSString *)token withCompletion:(void (^)(InitStatus status))completion
{
    __weak HPControlledPrintManager *weakSelf = self;
    [self setEnvironment:stack withCompletion:^(InitStatus status){
        if (status == InitStatusServerStackAvailable) {
            if (token != nil ) {
                [weakSelf validateToken:token withCompletion:^(InitStatus status){
                    if (completion){
                        completion(status);
                    }
                }];
            }
        }  else {
            if (completion){
                completion(InitStatusServerStackNotAvailable);
            }
        }
    }];
}

- (void)setEnvironment: (ServerStack)stack withCompletion:(void (^)(InitStatus status))completion
{
    NSString *discoveryUrl;
    if (stack == ServerStackDevelopment) {
        discoveryUrl = [NSString stringWithFormat:@"http://cayman-dev-02.cloudpublish.com/coupons/discovery?env=Dev"];
        NSLog(@"Server Stack: Dev ======================");
        
    } else if (stack == ServerStackExternalTest) {
        discoveryUrl = [NSString stringWithFormat:@"http://cayman-ext.cloudpublish.com/coupons/discovery?env=Ext"];
        NSLog(@"Server Stack: Ext ======================");
        
    } else if (stack == ServerStackProduction) {
        discoveryUrl = [NSString stringWithFormat:@"http://cayman-prod.cloudpublish.com/coupons/discovery?env=Prod"];
        NSLog(@"Server Stack: Prod ======================");
    }

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
    [metricsSender send:services.postMetricsUrl withPrintJobRequest:printJobRequest forOperation:kStatusCouponPrintRequested];
    
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
        return;
    }    
    NSLog(@"json: %@", dict);
    
    services = [[HPServices alloc] init];
    NSString *caymanRootUrl = [dict objectForKey:@"caymanrooturl"];
    
    // jobUrl = discoverProperties.getCaymanRootUrl() + discoverProperties.getCouponsJobAsyncUrl();
    services.couponsJobUrl = [NSString stringWithFormat:@"%@%@", caymanRootUrl, [dict objectForKey:@"couponsjobasyncurl"]];
    NSLog(@"couponsJob: %@", services.couponsJobUrl);
    
    // metricsPostURL = discoverProperties.getCaymanRootUrl() + discoverProperties.getMetricsUrl();
    services.postMetricsUrl = [NSString stringWithFormat:@"%@%@", caymanRootUrl, [dict objectForKey:@"metricsurl"]];
    NSLog(@"postsMetrics: %@", services.postMetricsUrl);
    
    // getDeviceUrl = discoverProperties.getCaymanRootUrl() + discoverProperties.getDeferedPrint() + "/getDevice" + "?deviceID=";
    services.getDeferredPrintForDevice = [NSString stringWithFormat:@"%@%@/getDevice?deviceID=", caymanRootUrl, [dict objectForKey:@"deferedprint"]];
    NSLog(@"getDeferredPrintForDevice: %@", services.getDeferredPrintForDevice);
    
    // createJobUrl = discoverProperties.getCaymanRootUrl() + discoverProperties.getDeferedPrint() + discoverProperties.getCreateJob();
    services.createDeferredPrintJob = [NSString stringWithFormat:@"%@%@%@", caymanRootUrl, [dict objectForKey:@"deferedprint"], [dict objectForKey:@"createjob"]];
    NSLog(@"createDeferredPrintJob: %@", services.createDeferredPrintJob);
    
    // getDeferedJobsUrl = discoverProperties.getCaymanRootUrl() + discoverProperties.getDeferedPrint() + discoverProperties.getGetJobsDevice() + "?deviceID=";
    services.getDeferredPrintJobsForDevice = [NSString stringWithFormat:@"%@%@%@?deviceID=", caymanRootUrl, [dict objectForKey:@"deferedprint"], [dict objectForKey:@"getjobsdevice"]];
    NSLog(@"getDeferredPrintJobsForDevice: %@", services.getDeferredPrintJobsForDevice);
    
    // saveDeviceUrl = discoverProperties.getCaymanRootUrl() + discoverProperties.getDeferedPrint() + "/saveDevice";
    services.setDeferredPrintDevice = [NSString stringWithFormat:@"%@%@/saveDevice", caymanRootUrl, [dict objectForKey:@"deferedprint"]];
    NSLog(@"setDeferredPrintDevice: %@", services.setDeferredPrintDevice);
    
    // notifyQplesUrl = discoverProperties.getCaymanRootUrl() + "/coupons/qples/notify";
    services.notifyQples = [NSString stringWithFormat:@"%@/coupons/qples/notify", caymanRootUrl];
    NSLog(@"notifyQples: %@", services.notifyQples);

    // validateTokenUrl = discoverProperties.getCaymanRootUrl() + "/coupons/qples/validate";
    services.validateQplesToken = [NSString stringWithFormat:@"%@/coupons/qples/validate", caymanRootUrl];
    NSLog(@"validateQplesToken: %@", services.validateQplesToken);
    
    NSArray *printServerHosts = [dict objectForKey:@"printserverhosts"];
    NSString *printServerDomain = [dict objectForKey:@"printserverdomain"];
    secureAssetPrinter->set_option("serverName", printServerHosts[0]);
    secureAssetPrinter->set_option("fallbackServiceOne", printServerHosts[1]);
    secureAssetPrinter->set_option("fallbackServiceTwo", printServerHosts[2]);
    secureAssetPrinter->set_option("domainName", printServerDomain);
    
    NSLog(@"serverName: %@", printServerHosts[0]);
    NSLog(@"fallbackServiceOne: %@", printServerHosts[1]);
    NSLog(@"fallbackServiceTwo: %@", printServerHosts[2]);
    NSLog(@"domainName: %@", printServerDomain);
    
    if (self.httpGetCompletion != nil) {
        self.httpGetCompletion(InitStatusServerStackAvailable);
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
