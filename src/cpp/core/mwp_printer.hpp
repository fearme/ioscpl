
#ifndef __MWP_PRINTER_HPP__
#define __MWP_PRINTER_HPP__

#include "mwp_mq.hpp"

#include <string>
#include <vector>

namespace net_mobilewebprint {

  using std::string;
  using std::vector;

  struct controller_t;
  struct printer_manager_t;

  typedef std::map<std::string, uint32> prop_times_t;

  struct printer_t
  {
    controller_t &          controller;

    strmap                  properties;
    prop_times_t            property_times;   // When was this property last requested (so we do not flood with requests)

    printer_t(printer_manager_t &);

    void set(string const & key, string const & value);
    void set(string const & key, uint16 value);

    bool matches(string const & key, string const & value);

    void auto_update();
  };
  typedef vector<printer_t> printer_list_t;

  struct printer_manager_t : public mq_handler_t
  {
    controller_t &          controller;
    mq_t         &          mq;

    printer_list_t          printer_list;

    printer_manager_t(controller_t &);

    printer_t *    find_by_key(string const & key, string const & value);
    printer_t *  create_by_key(string const & key, string const & value);

    // ----- Hooking into the select loop -----
    virtual mq_result                 initialize();

    virtual mq_result       on_select_loop_start(select_loop_start_extra_t const & extra);

    virtual mq_result              on_pre_select(pre_select_extra_t & extra);
    virtual mq_result                  on_select(select_extra_t & extra);
    virtual mq_result                 on_message(string const & name, buffer_view_t const & payload, message_extra_t & extra);

    virtual mq_result         on_select_loop_end(select_loop_end_extra_t const &   extra);
    virtual mq_result        on_select_loop_idle(select_loop_idle_extra_t const &  extra);

  };
};

#endif  // __MWP_PRINTER_HPP__


