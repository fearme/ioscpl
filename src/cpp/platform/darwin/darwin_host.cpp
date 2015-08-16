
#include "mwp_types.hpp"
#include "mwp_host.hpp"
#include "darwin_host.hpp"
#include "posix_host.hpp"

using namespace net_mobilewebprint;

bool net_mobilewebprint::host::start_thread(void*(*pfn)(void*data), void*data)
{
  return net_mobilewebprint::posix::start_thread(pfn, data);
}

uint32 net_mobilewebprint::host::get_tick_count()
{
  return net_mobilewebprint::posix::get_tick_count();
}

int net_mobilewebprint::host::get_last_network_error()
{
  return net_mobilewebprint::posix::get_last_network_error();
}

bool net_mobilewebprint::host::allocate_lock(char const * name, void ** lock)
{
  return net_mobilewebprint::posix::allocate_lock(name, lock);
}

bool net_mobilewebprint::host::lock(char const * name, void * lock)
{
  return net_mobilewebprint::posix::lock(name, lock);
}

bool net_mobilewebprint::host::unlock(char const * name, void * lock)
{
  return net_mobilewebprint::posix::unlock(name, lock);
}

void net_mobilewebprint::host::free_lock(char const * name, void * lock)
{
  net_mobilewebprint::posix::free_lock(name, lock);
}

void net_mobilewebprint::host::interruptable_sleep(uint32 msec)
{
  net_mobilewebprint::posix::interruptable_sleep(msec);
}

