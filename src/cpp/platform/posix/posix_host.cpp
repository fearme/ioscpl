
#include "mwp_types.hpp"
#include "posix_host.hpp"

#include <pthread.h>
#include <sys/time.h>

using namespace net_mobilewebprint;

uint32 get_tick();

bool net_mobilewebprint::posix::start_thread(void*(*pfn)(void*data), void*data)
{
  pthread_attr_t attr;
  pthread_t      tid = 0;

  if (pthread_attr_init(&attr) == 0) {
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, pfn, data);
    pthread_attr_destroy(&attr);
  }

  return true;
}

uint32 net_mobilewebprint::posix::get_tick_count()
{
  static uint32 first_tick = get_tick();
  return get_tick() - first_tick;
}

uint32 get_tick()
{
  struct timeval result = {0};
  gettimeofday(&result, NULL);
  return (uint32)((result.tv_sec * 1000) + (result.tv_usec * 0.001));
}

