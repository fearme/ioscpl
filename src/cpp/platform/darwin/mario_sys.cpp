/**
 *  Implementations of functions that need the system APIs -- Darwin implementation.
 *
 *  Mario core has all the network functionality that Mario-client needs, and is
 *  centered on the BSD-socket APIs, which are "universally" implemented.  However,
 *  Mario needs a handful of other system APIs (like a function to start a new
 *  thread).  This file supplies that functionality for this above-named platform.
 */

#include "mwp_types.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <CFNetwork/CFNetwork.h>

using namespace net_mobilewebprint;

// ----------------------------------------------------------------------------------
// Startup
// ----------------------------------------------------------------------------------
void net_mobilewebprint::platform_bootstrap()
{
  // Get proxy
  int             port          = -1;
  char const *    host          = NULL;

  CFDictionaryRef proxyDicRef   = NULL;
  CFDictionaryRef urlDicRef     = NULL;
  CFURLRef        urlRef        = NULL;
  CFArrayRef      proxyArrayRef = NULL;
  CFNumberRef     portNumberRef = NULL;
  CFStringRef     hostNameRef   = NULL;

  if ((proxyDicRef = CFNetworkCopySystemProxySettings()) != NULL) {
    if ((urlRef = CFURLCreateWithBytes(kCFAllocatorDefault, (UInt8* const)MWP_DEFAULT_ROOT_URL, ::strlen(MWP_DEFAULT_ROOT_URL), kCFStringEncodingASCII, NULL)) != NULL) {
      if ((proxyArrayRef = CFNetworkCopyProxiesForURL(urlRef, proxyDicRef)) != NULL) {
        if ((urlDicRef = (CFDictionaryRef)CFArrayGetValueAtIndex(proxyArrayRef, 0)) != NULL) {
          if ((portNumberRef = (CFNumberRef)CFDictionaryGetValue(urlDicRef, (void const *)kCFProxyPortNumberKey)) != NULL) {
            if ((hostNameRef = (CFStringRef)CFDictionaryGetValue(urlDicRef, (void const *)kCFProxyHostNameKey)) != NULL) {

              if (CFNumberGetValue(portNumberRef, kCFNumberSInt32Type, &port)) {
                if ((host = CFStringGetCStringPtr(hostNameRef, kCFStringEncodingASCII)) != NULL) {

                  // Wow.  Finally got it
                  hp_mwp_set_option("http_proxy_name", host);
                  hp_mwp_set_int_option("http_proxy_port", port);
                }
              }
            }
          }
        }
      }
    }
  }

  if (proxyDicRef != NULL) { CFRelease(proxyDicRef); }
  if (urlDicRef != NULL) { CFRelease(urlDicRef); }
  if (urlRef != NULL) { CFRelease(urlRef); }
  if (proxyArrayRef != NULL) { CFRelease(proxyArrayRef); }
  if (portNumberRef != NULL) { CFRelease(portNumberRef); }

}

// ----------------------------------------------------------------------------------
// Convert the system-preferred string type to US-ASCII -- the network APIs
// use US-ASCII.  For example, Windows prefers UNICODE as the string type
// that gets passed around.  This platform uses 'normal' C (utf-8) strings.
// ----------------------------------------------------------------------------------
string net_mobilewebprint::platform_to_ascii_string(void const *str)
{
  return net_mobilewebprint::dumb_and_ok::platform_to_ascii_string(str);
}

// ----------------------------------------------------------------------------------
// Mutex
// ----------------------------------------------------------------------------------
bool net_mobilewebprint::allocate_host_lock(char const * name, void ** result_lock_out)
{
  return net_mobilewebprint::POSIX::allocate_host_lock(name, result_lock_out);
}

bool net_mobilewebprint::host_lock(char const * name, void * lock_)
{
  return net_mobilewebprint::POSIX::host_lock(name, lock_);
}

bool net_mobilewebprint::host_unlock(char const * name, void * lock_)
{
  return net_mobilewebprint::POSIX::host_unlock(name, lock_);
}

void net_mobilewebprint::free_host_lock(char const * name, void * lock_)
{
  return net_mobilewebprint::POSIX::free_host_lock(name, lock_);
}

// ----------------------------------------------------------------------------------
// Threads
// ----------------------------------------------------------------------------------
bool net_mobilewebprint::start_thread(void*(*pfn)(void*data), void*data)
{
  return net_mobilewebprint::POSIX::start_thread(pfn, data);
}

// ----------------------------------------------------------------------------------
// Time
// ----------------------------------------------------------------------------------
uint32 net_mobilewebprint::get_tick_count()
{
  return net_mobilewebprint::POSIX::get_tick_count();
}

bool net_mobilewebprint::interruptable_sleep(int msec)
{
  return net_mobilewebprint::POSIX::interruptable_sleep(msec);
}

// ----------------------------------------------------------------------------------
// Socket operations
// ----------------------------------------------------------------------------------
bool net_mobilewebprint::set_socket_non_blocking(int fd)
{
  return net_mobilewebprint::POSIX::set_socket_non_blocking(fd);
}

bool net_mobilewebprint::set_socket_blocking(int fd)
{
  return net_mobilewebprint::POSIX::set_socket_blocking(fd);
}

bool net_mobilewebprint::connect_in_progress()
{
  return net_mobilewebprint::POSIX::connect_in_progress();
}

int net_mobilewebprint::get_last_network_error()
{
  return net_mobilewebprint::POSIX::get_last_network_error();
}

// ----------------------------------------------------------------------------------
// Assert
// ----------------------------------------------------------------------------------
void * net_mobilewebprint::mwp_assert(void * x)
{
  return net_mobilewebprint::dumb_and_ok::mwp_assert(x);
}

int    net_mobilewebprint::mwp_assert(int x)
{
  return net_mobilewebprint::dumb_and_ok::mwp_assert(x);
}

bool   net_mobilewebprint::mwp_assert(bool x)
{
  return net_mobilewebprint::dumb_and_ok::mwp_assert(x);
}

