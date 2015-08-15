
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

