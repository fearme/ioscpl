
#ifndef __POSIX_HOST_HPP__
#define __POSIX_HOST_HPP__

#include <sstream>

namespace net_mobilewebprint {

  namespace posix {
    bool    start_thread(void*(*pfn)(void*data), void*data);
    uint32  get_tick_count();
    int     get_last_network_error();

    // Locks
    bool    allocate_lock(char const * name, void ** lock);
    bool    lock(char const * name, void * lock);
    bool    unlock(char const * name, void * lock);
    void    free_lock(char const * name, void * lock);

    // Sleep-like things
    void    interruptable_sleep(uint32 msec);
  };

  namespace cli {
    // Logging
    void    send_log(int verbosity, std::stringstream & ss);
  };

};

#endif  // __POSIX_HOST_HPP__

