
#ifndef __MWP_CONTROLLER_HPP__
#define __MWP_CONTROLLER_HPP__

#include "mwp_mdns.hpp"
#include "mwp_mq.hpp"

namespace net_mobilewebprint {

  struct controller_t
  {
    mq_t        mq;

    mdns_t      mdns;

    controller_t();

    void start(bool should_block = true);
  };

};

#endif  // __MWP_CONTROLLER_HPP__

