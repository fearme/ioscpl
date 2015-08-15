
#ifndef __MWP_HOST_HPP__
#define __MWP_HOST_HPP__

namespace net_mobilewebprint {

  namespace host {
    bool start_thread(void*(*pfn)(void*data), void*data);
    uint32 get_tick_count();
  };

};

#endif  // __MWP_HOST_HPP__

