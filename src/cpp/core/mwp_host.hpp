
#ifndef __MWP_HOST_HPP__
#define __MWP_HOST_HPP__

#include "mwp_types.hpp"

namespace net_mobilewebprint {

  namespace host {
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

#endif  // __MWP_HOST_HPP__

