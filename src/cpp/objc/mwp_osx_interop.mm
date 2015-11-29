
#import <Cocoa/Cocoa.h>
#import <SystemConfiguration/SystemConfiguration.h>
#import <IOKit/IOKitLib.h>
#import <CommonCrypto/CommonHMAC.h>
#include "hp_mwp.h"

extern CFStringRef CopySerialNumber();
void set_client_id();

int mwp_interop_bootstrap(void * app_data, char const * message, int id, int32 transaction_id, uint8 const * p1, mwp_params const * params)
{

  set_client_id();

  return 0;
}

//int darwin_log(char type, char const * tag, char const * message)
//{
//  NSLog(@"%s", message);
//  //printf("----- %s\n", message);
//  //printf("%s\n", message);
//}

void set_client_id()
{
//  NSString *uuid = [[[UIDevice currentDevice] identifierForVendor] UUIDString];

//  char const * cData = [uuid cStringUsingEncoding:NSUTF8StringEncoding];
  CFStringRef serialNumber = CopySerialNumber();
  if (serialNumber != NULL) {
    char const * cData = CFStringGetCStringPtr(serialNumber, kCFStringEncodingASCII);
    char const * salt  = "HPIsTheBestPlaceToWorkWeLoveIt!!";  //happens to be 32 chars
    unsigned char cHMAC[CC_SHA256_DIGEST_LENGTH];
    CCHmac(kCCHmacAlgSHA256, salt, strlen(salt), cData, strlen(cData), cHMAC);

    NSMutableString* output = [NSMutableString stringWithCapacity: (CC_SHA256_DIGEST_LENGTH + 1) * 2];
    for(int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++) {
      [output appendFormat:@"%02x", cHMAC[i]];
    }

    hp_mwp_set_option("clientid", [output cStringUsingEncoding:NSUTF8StringEncoding]);

    CFRelease(serialNumber);
  }
}


