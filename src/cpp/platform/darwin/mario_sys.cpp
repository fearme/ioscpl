/**
 *  Implementations of functions that need the system APIs -- Darwin implementation.
 *
 *  Mario core has all the network functionality that Mario-client needs, and is
 *  centered on the BSD-socket APIs, which are "universally" implemented.  However,
 *  Mario needs a handful of other system APIs (like a function to start a new 
 *  thread).  This file supplies that functionality for this above-named platform.
 */

#include "mwp_types.hpp"

using namespace net_mobilewebprint;

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

