
#ifndef __POSIX_HOST_HPP__
#define __POSIX_HOST_HPP__

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
  };

};

#endif  // __POSIX_HOST_HPP__

