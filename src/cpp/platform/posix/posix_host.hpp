
#ifndef __POSIX_HOST_HPP__
#define __POSIX_HOST_HPP__

namespace net_mobilewebprint {

  namespace posix {
    bool start_thread(void*(*pfn)(void*data), void*data);
    uint32 get_tick_count();
  };

};

#endif  // __POSIX_HOST_HPP__

