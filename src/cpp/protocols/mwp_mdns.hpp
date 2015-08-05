
#ifndef __MWP_MDNS_HPP__
#define __MWP_MDNS_HPP__

#include <mwp_controller.hpp>
#include <mwp_mq.hpp>

namespace net_mobilewebprint {

  struct mdns_t : public mq_handler_t
  {
    controller_t & controller;
    mq_t         & mq;
  };

};

#endif  // __MWP_MDNS_HPP__

