
#import <SystemConfiguration/SystemConfiguration.h>
#import <UIKit/UIKit.h>
#import <CommonCrypto/CommonHMAC.h>
#include "hp_mwp.h"

static SCNetworkReachabilityRef reachability;

void set_client_id();
void setup_network_monitor();
static void networkReachabilityCallback(SCNetworkReachabilityRef target, SCNetworkConnectionFlags flags, void * object);
static void printReachabilityFlags(SCNetworkReachabilityFlags flags, const char* comment);
static bool is_reachable(SCNetworkConnectionFlags flags);
static void notify_connectivity_change(bool reachable);

int mwp_interop_bootstrap(void * app_data, char const * message, int id, int32 transaction_id, uint8 const * p1, mwp_params const * params)
{

  set_client_id();
  setup_network_monitor();

  return 0;
}

int darwin_log(char type, char const * tag, char const * message)
{
  NSLog(@"%s", message);
  //printf("----- %s\n", message);
  //printf("%s\n", message);
}

void set_client_id()
{
  NSString *uuid = [[[UIDevice currentDevice] identifierForVendor] UUIDString];

  char const * cData = [uuid cStringUsingEncoding:NSUTF8StringEncoding];
  char const * salt  = "HPIsTheBestPlaceToWorkWeLoveIt!!";  //happens to be 32 chars
  unsigned char cHMAC[CC_SHA256_DIGEST_LENGTH];
  CCHmac(kCCHmacAlgSHA256, salt, strlen(salt), cData, strlen(cData), cHMAC);

  NSMutableString* output = [NSMutableString stringWithCapacity: (CC_SHA256_DIGEST_LENGTH + 1) * 2];
  for(int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++) {
    [output appendFormat:@"%02x", cHMAC[i]];
  }

  hp_mwp_set_option("clientid", [output cStringUsingEncoding:NSUTF8StringEncoding]);
}

void setup_network_monitor()
{
  SCNetworkConnectionFlags flags;

  reachability = SCNetworkReachabilityCreateWithName(NULL, "dev.mobiledevprint.net");

  // If we successfully get flags now, we will not get them initially via the callback
  if (SCNetworkReachabilityGetFlags(reachability, &flags)) {
    networkReachabilityCallback(reachability, flags, NULL);
  }

  // Now, register for the callback
  if (reachability != NULL) {
    SCNetworkReachabilityContext context = {0, NULL, NULL, NULL, NULL};
    if (SCNetworkReachabilitySetCallback(reachability, networkReachabilityCallback, &context)) {
      if (SCNetworkReachabilityScheduleWithRunLoop(reachability, [[NSRunLoop currentRunLoop] getCFRunLoop], kCFRunLoopCommonModes)) {
        return;
      }
    }
    CFRelease(reachability);
  }

}

static int                      numReachabilityCalls  = 0;
static SCNetworkConnectionFlags connection_flags      = 0;

static void networkReachabilityCallback(SCNetworkReachabilityRef target, SCNetworkConnectionFlags flags, void * object)
{
  printReachabilityFlags(flags, "");

  SCNetworkConnectionFlags old = connection_flags;

  connection_flags = flags;

  // Is this the first notificaiton?
  if (numReachabilityCalls++ == 0) {

    notify_connectivity_change(is_reachable(flags));

  } else {

    if (is_reachable(flags)) {

      // Do we already know that we are reachable?
      if (!is_reachable(old)) {
        notify_connectivity_change(is_reachable(flags));
      }

    } else {

      // Do we already know that we are unreachable?
      if (is_reachable(old)) {
        notify_connectivity_change(is_reachable(flags));
      }
    }
  }
}

static void notify_connectivity_change(bool reachable)
{
  hp_mwp_send_immediately("LOCAL_REACHABILITY_CHANGED", reachable ? "REACHABLE" : "UNREACHABLE");
}

static bool is_reachable(SCNetworkConnectionFlags flags)
{
  return ( (flags & kSCNetworkReachabilityFlagsReachable) &&
          !(flags & kSCNetworkReachabilityFlagsConnectionRequired) &&
          !(flags & kSCNetworkReachabilityFlagsIsWWAN));
}

static void printReachabilityFlags(SCNetworkReachabilityFlags flags, const char* comment)
{
  NSLog(@"-------------------------------------------------- Reachability Flag Status: %c%c %c%c%c%c%c%c%c %s\n",
        (flags & kSCNetworkReachabilityFlagsIsWWAN)               ? 'W' : '-',
        (flags & kSCNetworkReachabilityFlagsReachable)            ? 'R' : '-',

        (flags & kSCNetworkReachabilityFlagsTransientConnection)  ? 't' : '-',
        (flags & kSCNetworkReachabilityFlagsConnectionRequired)   ? 'c' : '-',
        (flags & kSCNetworkReachabilityFlagsConnectionOnTraffic)  ? 'C' : '-',
        (flags & kSCNetworkReachabilityFlagsInterventionRequired) ? 'i' : '-',
        (flags & kSCNetworkReachabilityFlagsConnectionOnDemand)   ? 'D' : '-',
        (flags & kSCNetworkReachabilityFlagsIsLocalAddress)       ? 'l' : '-',
        (flags & kSCNetworkReachabilityFlagsIsDirect)             ? 'd' : '-',
        comment
        );
}


