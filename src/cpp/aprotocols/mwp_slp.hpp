
#ifndef __MWP_SLP_HPP__
#define __MWP_SLP_HPP__

#include "mwp_mq.hpp"

namespace net_mobilewebprint {

  struct controller_base_t;

  using namespace net_mobilewebprint::mq_enum;

  struct slp_t : public mq_select_handler_t, mq_handler_t
  {
    controller_base_t &       controller;
    mq_t &                    mq;

    network_node_t *          group_mcast;
    buffer_t                  attribute_request;

    handler_holder_by_id_t *  pth_resend_attr_req;

    mq_backoff_timer_t        resend_req;
    bool                      check_udp_writableness;

    slp_t(controller_base_t &);

    virtual int                         pre_select(mq_pre_select_data_t * pre_select_data);
    virtual e_handle_result           _mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra);

    virtual e_handle_result   on_select_loop_start(mq_select_loop_start_data_t const & data);
    virtual e_handle_result                 handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);
    e_handle_result             _scan_for_printers(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);
    e_handle_result                _on_slp_payload(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);

    bool            parse_packet(buffer_view_i & payload, char const *& begin, char const *& end);
    bool            parse(buffer_view_i const & payload, map<string, string> & attrs, map<string, string> & attrs_lc, deque<string> & other_entries);

    virtual string  mod_name();

    /* privateish */
    slp_t const & _init_group_mcast();
    slp_t const & _init_group_mcast(network_node_t * node);

    slp_t const & _init_attribute_request();
  };

};

#endif  // __MWP_SLP_HPP__
