/**
 *  Implementations of functions that need the system APIs -- POSIX implementation.
 *
 *  Mario core has all the network functionality that Mario-client needs, and is
 *  centered on the BSD-socket APIs, which are "universally" implemented.  However,
 *  Mario needs a handful of other system APIs (like a function to start a new 
 *  thread).  This file supplies that functionality for this above-named platform.
 */

#include "mwp_types.hpp"
#include "mwp_controller.hpp"
#include "mwp_utils.hpp"

#include <string>
#include <map>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

using std::string;
using std::map;

using namespace net_mobilewebprint;

// ----------------------------------------------------------------------------------
// Convert the system-preferred string type to US-ASCII -- the network APIs
// use US-ASCII.  For example, Windows prefers UNICODE as the string type
// that gets passed around.  This platform uses 'normal' C (utf-8) strings.
// ----------------------------------------------------------------------------------
string net_mobilewebprint::dumb_and_ok::platform_to_ascii_string(void const *str_)
{
  return string((char const *)str_);
}

// ----------------------------------------------------------------------------------
// Mutex
// ----------------------------------------------------------------------------------
map<string, pthread_mutex_t *> mutexes;
bool net_mobilewebprint::POSIX::allocate_host_lock(char const * name, void ** result_lock_out)
{
  if (result_lock_out == NULL) { return false; }

  pthread_mutex_t * mutex = _item(mutexes, name);
  if (mutex != NULL) {
    //*(pthread_mutex_t*)result_lock_out = mutex;
    *result_lock_out = (void*)mutex;
    return true;
  }

  /* otherwise */
  mutex = new pthread_mutex_t();
  mutexes[name] = mutex;
  return pthread_mutex_init(mutex, NULL) == 0;
}

bool net_mobilewebprint::POSIX::host_lock(char const * name, void * lock_)
{
  //printf("Locking: %s\n", name);
  pthread_mutex_t * lock = _item(mutexes, name);
  return pthread_mutex_lock(lock) == 0;
}

bool net_mobilewebprint::POSIX::host_unlock(char const * name, void * lock_)
{
  //printf("UnLocking: %s\n", name);
  pthread_mutex_t * lock = _item(mutexes, name);
  return pthread_mutex_unlock(lock) == 0;
}

void net_mobilewebprint::POSIX::free_host_lock(char const * name, void * lock_)
{
  pthread_mutex_t * lock = _item(mutexes, name);
  pthread_mutex_destroy(lock);
}

// ----------------------------------------------------------------------------------
// Threads
// ----------------------------------------------------------------------------------
bool net_mobilewebprint::POSIX::start_thread(void*(*pfn)(void*data), void*data)
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

// ----------------------------------------------------------------------------------
// Time
// ----------------------------------------------------------------------------------
struct timeval get_time_val();

uint32 net_mobilewebprint::POSIX::get_tick_count()
{
  // Remember the time when we first started
  static struct timeval first_tv = get_time_val();

  struct timeval now = get_time_val();

  //printf("get_tick_count: %d %ld\n", now.tv_sec, now.tv_sec);

  now.tv_sec -= first_tv.tv_sec;
  now.tv_usec -= first_tv.tv_usec;

  uint32 result = (uint32)((now.tv_sec * 1000) + (now.tv_usec / 1000));
  //printf("3get_tick_count: 0x%09x %d\n", result, result);

  /* otherwise */
  return result;
}

struct timeval get_time_val()
{
  struct timeval result = {0};
  gettimeofday(&result, NULL);
  return result;
}

bool net_mobilewebprint::POSIX::interruptable_sleep(int msec)
{
  usleep(msec * 1000);
  return true;
}

// ----------------------------------------------------------------------------------
// Socket operations
// ----------------------------------------------------------------------------------
bool net_mobilewebprint::POSIX::set_socket_non_blocking(int fd)
{
  long arg = fcntl(fd, F_GETFL, NULL);
  if (arg < 0) { return false; }

  arg |= O_NONBLOCK;
  return (fcntl(fd, F_SETFL, arg) >= 0);
}

bool net_mobilewebprint::POSIX::set_socket_blocking(int fd)
{
  long arg = fcntl(fd, F_GETFL, NULL);
  if (arg < 0) { return false; }

  arg &= ~O_NONBLOCK;
  return (fcntl(fd, F_SETFL, arg) >= 0);
}

bool net_mobilewebprint::POSIX::connect_in_progress()
{
  int my_last_error = get_last_network_error();
  return (my_last_error == EINPROGRESS) || (my_last_error == EWOULDBLOCK);
}

int net_mobilewebprint::POSIX::get_last_network_error()
{
  return errno;
}

// ----------------------------------------------------------------------------------
// Assert
// ----------------------------------------------------------------------------------
void * net_mobilewebprint::dumb_and_ok::mwp_assert(void * x)
{
  if (x == NULL) {
    printf("ASSERT-FAIL*!!!!!!!!!!!!!!!!!\n");
    if (net_mobilewebprint::get_flag("fast_fail")) {
      *(int*)(0) = 1234;
    }
  }

  return x;
}

int    net_mobilewebprint::dumb_and_ok::mwp_assert(int x)
{
  if (x == 0) {
    printf("ASSERT-FAILx!!!!!!!!!!!!!!!!!\n");
    if (net_mobilewebprint::get_flag("fast_fail")) {
      *(int*)(0) = 1234;
    }
  }

  return x;
}

bool   net_mobilewebprint::dumb_and_ok::mwp_assert(bool x)
{
  if (!x) {
    printf("ASSERT-FAILb!!!!!!!!!!!!!!!!!\n");
    if (net_mobilewebprint::get_flag("fast_fail")) {
      *(int*)(0) = 1234;
    }
  }

  return x;
}

