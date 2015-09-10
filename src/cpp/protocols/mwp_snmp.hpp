
#ifndef __MWP_SNMP_HPP__
#define __MWP_SNMP_HPP__

#include "mwp_mq.hpp"
#include "mwp_socket.hpp"

#include <string>

namespace net_mobilewebprint {
  struct snmp_t : public mq_handler_t
  {
    controller_t &          controller;
    mq_t         &          mq;

    packet_list_t           packet_list;

    snmp_t(controller_t &);

    void    on_scan_for_printers(string const & name, buffer_view_t const & payload, message_extra_t & extra);
//    void          on_mdns_packet(string const & name, buffer_view_t const & payload, message_extra_t & extra);

//    static  buffer_t &  _mk_query_request_packet(buffer_t & result, mdns::record_type type, char const * service_name);

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

#endif  // __MWP_SNMP_HPP__

