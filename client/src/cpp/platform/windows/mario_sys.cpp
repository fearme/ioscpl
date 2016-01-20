/**
 *  Implementations of functions that need the system APIs -- Windows implementation.
 *
 *  Mario core has all the network functionality that Mario-client needs, and is
 *  centered on the BSD-socket APIs, which are "universally" implemented.  However,
 *  Mario needs a handful of other system APIs (like a function to start a new 
 *  thread).  This file supplies that functionality for this above-named platform.
 *
 */


#include "mwp_types.hpp"
#include "mwp_controller.hpp"
#include "mwp_core_api.hpp"
#include "mwp_secure_asset_printing_api.hpp"

#include <string>
#include <iostream>
//#include <stdio.h>
//#include <pthread.h>
//#include <unistd.h>
//#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <process.h>

#include <tchar.h>

using std::cout;
using std::endl;
using std::string;
using std::map;

bool g_silent = false;


// ----------------------------------------------------------------------------------
// Startup
// ----------------------------------------------------------------------------------
void net_mobilewebprint::platform_bootstrap()
{
  net_mobilewebprint::dumb_and_ok::platform_bootstrap();
}

void net_mobilewebprint::interop_bootstrap()
{
  net_mobilewebprint::dumb_and_ok::interop_bootstrap();
}

// ===================================================================================================================================
//             Implement system functions
// ===================================================================================================================================

char const * net_mobilewebprint::platform_name()
{
  return "windows";
}

void net_mobilewebprint::log_d(char const * msg, log_param_t)
{
  if (g_silent) { return; }

  if (!net_mobilewebprint::get_flag("quiet")) {
    cout << msg << endl;
  }
}

void net_mobilewebprint::log_v(char const * msg, log_param_t)
{
  if (g_silent) { return; }

  if (net_mobilewebprint::get_flag("verbose")) {
    std::cout << msg << endl;
  }
}

void net_mobilewebprint::log_w(char const * msg, log_param_t)
{
  if (g_silent) { return; }

  std::cout << msg << endl;
}

void net_mobilewebprint::log_e(char const * msg, log_param_t)
{
  std::cout << msg << endl;
}

// ----------------------------------------------------------------------------------
// Socket operations
// ----------------------------------------------------------------------------------
bool net_mobilewebprint::set_socket_non_blocking(int fd)
{
  u_long cmd = 1;
  int result = ioctlsocket(fd, FIONBIO, &cmd);
  return result == 0;
}

bool net_mobilewebprint::set_socket_blocking(int fd)
{
  u_long cmd = 0;
  int result = ioctlsocket(fd, FIONBIO, &cmd);
  return result == 0;
}

bool net_mobilewebprint::connect_in_progress()
{
  int my_last_error = get_last_network_error();
  return (my_last_error == WSAEINPROGRESS) || (my_last_error == WSAEWOULDBLOCK);
}

int net_mobilewebprint::get_last_network_error()
{
  return WSAGetLastError();
}

// Convert the system-preferred string type to US-ASCII -- the network APIs
// use US-ASCII.  For example, Windows prefers UNICODE as the string type
// that gets passed around.  This platform uses 'normal' C (utf-8) strings.
string net_mobilewebprint::platform_to_ascii_string(void const *str_)
{
  _TCHAR * str = (_TCHAR*)str_;

  int needed = WideCharToMultiByte(CP_ACP, 0, str, _tcslen(str), NULL, 0, NULL, NULL);
  char * buffer = new char[needed + 4];
  WideCharToMultiByte(CP_ACP, 0, str, -1, buffer, needed + 1, NULL, NULL);

  string result(buffer);
  delete[] buffer;

  return result;
}

// ----------------------------------------------------------------------------------
// Mutex
// ----------------------------------------------------------------------------------
char const win_lock[] = "windows_lock";
bool net_mobilewebprint::allocate_host_lock(char const * name, void ** result_lock_out)
{
  *result_lock_out = (void*)win_lock;
  return true;
}

bool net_mobilewebprint::host_lock(char const * name, void * lock)
{
  return true;
}

bool net_mobilewebprint::host_unlock(char const * name, void * lock)
{
  return true;
}

void net_mobilewebprint::free_host_lock(char const * name, void * lock)
{
}

// ----------------------------------------------------------------------------------
// Threads
// ----------------------------------------------------------------------------------
bool net_mobilewebprint::start_thread(void*(*pfn)(void*data), void*data)
{
  return (_beginthread((void(*)(void*))pfn, 0, data) != -1L);
}

// ----------------------------------------------------------------------------------
// Time
// ----------------------------------------------------------------------------------
bool net_mobilewebprint::interruptable_sleep(int msec)
{
  Sleep(msec);
  return true;
}

uint32 net_mobilewebprint::get_tick_count()
{
  return GetTickCount();
}

// ----------------------------------------------------------------------------------
// Assert
// ----------------------------------------------------------------------------------
void * net_mobilewebprint::assert(void * x)
{
  return x;
}

int    net_mobilewebprint::assert(int x)
{
  return x;
}

bool   net_mobilewebprint::assert(bool x)
{
  return x;
}

