
#include "mwp_types.hpp"
#include "posix_host.hpp"

#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <iostream>

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

int net_mobilewebprint::posix::get_last_network_error()
{
  return errno;
}

bool net_mobilewebprint::posix::allocate_lock(char const * name, void ** lock)
{
  // TODO: implement this
  return true;
}

bool net_mobilewebprint::posix::lock(char const * name, void * lock)
{
  // TODO: implement this
  return true;
}

bool net_mobilewebprint::posix::unlock(char const * name, void * lock)
{
  // TODO: implement this
  return true;
}

void net_mobilewebprint::posix::free_lock(char const * name, void * lock)
{
  // TODO: implement this
}

void net_mobilewebprint::posix::interruptable_sleep(uint32 msec)
{
  usleep(msec * 1000);
}

void net_mobilewebprint::cli::send_log(int verbosity, std::stringstream & ss)
{
  std::cout << ss.str();
}

