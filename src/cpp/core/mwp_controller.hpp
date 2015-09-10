
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

    strlist            params;
    strmap             options;
    strset             flags;

    controller_t();

    void set_option(string const & key, string const & value);
    void set_options(string const & str);
    void set_options(char const * sz);
    void set_argv(int argc, char const *argv[], int start = 1);

    void start(bool should_block = true);

    void get_attributes(printer_t & printer, strset keys);
    void get_attributes(printer_t & printer, string const & key);
  };

};

#endif  // __MWP_CONTROLLER_HPP__

