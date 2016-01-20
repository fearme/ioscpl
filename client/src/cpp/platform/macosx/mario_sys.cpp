
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include "mwp_types.hpp"

extern void darwin_platform_bootstrap();

void net_mobilewebprint::platform_bootstrap()
{
  darwin_platform_bootstrap();
}

CFStringRef CopySerialNumber()
{
  CFStringRef result = NULL;
  io_service_t platformExpert = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IOPlatformExpertDevice"));
  if (platformExpert) {

    CFTypeRef serialNumber = IORegistryEntryCreateCFProperty(platformExpert,
                                                             CFSTR(kIOPlatformSerialNumberKey),
                                                             kCFAllocatorDefault, 0);

    result = (CFStringRef)serialNumber;
    IOObjectRelease(platformExpert);
  }

  return result;
}

