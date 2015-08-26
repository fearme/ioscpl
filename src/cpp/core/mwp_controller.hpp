
#ifndef __MWP_CONTROLLER_HPP__
#define __MWP_CONTROLLER_HPP__

#include "mwp_mdns.hpp"
#include "mwp_printer.hpp"
#include "mwp_mq.hpp"

namespace net_mobilewebprint {

  struct controller_t
  {
    mq_t               mq;

    mdns_t             mdns;
    printer_manager_t  printers;

    controller_t();

    void start(bool should_block = true);

    void get_attributes(printer_t & printer, strset keys);
    void get_attributes(printer_t & printer, string const & key);
  };

};

#endif  // __MWP_CONTROLLER_HPP__

