
#include "mwp_types.hpp"
#include "mwp_core_api.hpp"
#include "mwp_utils.hpp"

#include <string>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

using std::string;

using namespace net_mobilewebprint;

bool g_verbose = false;

int main(int argc, char const * argv[])
{
  args_t ARGV(argc, argv);

  g_verbose = ARGV.get_flag("verbose");

  core_api_t mario;

  mario.start();
}



void net_mobilewebprint::log_d(char const * msg, log_param_t)
{
  printf("%s\n", msg);
}

void net_mobilewebprint::log_v(char const * msg, log_param_t)
{
  if (g_verbose) {
    net_mobilewebprint::log_d(msg);
  }
}

void net_mobilewebprint::log_e(char const * msg, log_param_t)
{
  net_mobilewebprint::log_d(msg);
}

string net_mobilewebprint::platform_to_ascii_string(void const *str_)
{
  return string((char const *)str_);
}

char const darwin_lock[] = "darwin_lock";
bool net_mobilewebprint::allocate_host_lock(char const * name, void ** result_lock_out)
{
  *result_lock_out = (void*)darwin_lock;
  return true;
}

bool net_mobilewebprint::host_lock(void * lock)
{
  return true;
}

bool net_mobilewebprint::host_unlock(void * lock)
{
  return true;
}

void net_mobilewebprint::free_host_lock(void * lock)
{
}

bool net_mobilewebprint::start_thread(void*(*pfn)(void*data), void*data)
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

bool net_mobilewebprint::interruptable_sleep(int msec)
{
  usleep(msec * 1000);
  return true;
}

int net_mobilewebprint::get_tick_count()
{
  struct timeval tm;
  if (gettimeofday(&tm, NULL) != 0) { return -1; }

  // On Darwin, gettimeofday returns the number of usec since epoch, which is now > 31 bits, 
  // so subtract off a lot of the bits (this time is near the end of Feburary, 2015).
  tm.tv_sec -= 1425150000;
  //printf("get_tick_count: %ld %03d\n", tm.tv_sec * 1000, tm.tv_usec / 1000);

  /* otherwise */
  return (tm.tv_sec * 1000) + (tm.tv_usec / 1000);
}

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

