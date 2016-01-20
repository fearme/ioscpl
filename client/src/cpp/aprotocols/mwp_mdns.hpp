
#ifndef __MWP_MDNS_HPP__
#define __MWP_MDNS_HPP__

#include "mwp_mq.hpp"

namespace net_mobilewebprint {

  struct controller_base_t;

  using namespace net_mobilewebprint::mq_enum;

  struct mdns_record_t {
    uint16 type;
    uint32 ttl;
    uint32 expires_at;

    mdns_record_t(uint16 type, uint32 ttl);
    mdns_record_t(uint16 type, uint32 ttl, uint32 now);

    /* private */
    void _init(uint32 now);
  };

  struct mdns_A_record_t : public mdns_record_t
  {
    string ip;
    string name;

    mdns_A_record_t(uint32 ttl, string const & ip, string const & name);
    mdns_A_record_t(uint32 ttl, string const & ip, string const & name, uint32 now);
  };

  struct mdns_SRV_record_t : public mdns_record_t
  {
    string ip;
    string name, dns_name;
    uint16 port;

    mdns_SRV_record_t(uint32 ttl, string const & ip, string const & dns_name, string const & name, uint16 port);
    mdns_SRV_record_t(uint32 ttl, string const & ip, string const & dns_name, string const & name, uint16 port, uint32 now);
  };

  struct mdns_t : public mq_select_handler_t, mq_handler_t
  {
    controller_base_t &       controller;
    mq_t &                    mq;

    network_node_t *          group_mcast;
    buffer_t                  query_request;
    buffer_t                  query_request_bjnp;

    deque<buffer_t *>         requests;

    // Timeouts for resending the query
    handler_holder_by_id_t *  timer_holder;
    int                       resend_query_timeout;

    //mq_backoff_timer_t resend_query;
    mq_rolling_backoff_timer_t resend_query;
    uint32                    ignore_start_time;

    // Database of info received
    map<string, mdns_A_record_t *>    a_records;
    map<string, mdns_SRV_record_t *>  srv_records;

    mdns_t(controller_base_t &);

    virtual int                         pre_select(mq_pre_select_data_t * pre_select_data);
    virtual e_handle_result           _mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra);

    virtual e_handle_result   on_select_loop_start(mq_select_loop_start_data_t const & data);
    virtual e_handle_result                 handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);
    e_handle_result             _scan_for_printers(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);
    e_handle_result                _on_mdns_packet(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);

    /* implementation */
    void _report_discovery(mdns_SRV_record_t *);

    virtual string  mod_name();

    /* privateish */
    mdns_t const & _init_group_mcast();
    mdns_t const & _init_group_mcast(network_node_t * node);

    mdns_t const & _init_query_requests();
    mdns_t const & _queue_up_main_requests();

    uint16 transaction_id;
    buffer_t * _mk_request_packet(buffer_t & out, uint16 q_type, string const & name_part_a);
    buffer_t * _mk_request_packet(buffer_t & out, uint16 q_type, string const & name_part_a, string const & name_part_b);
    buffer_t * _mk_request_packet(buffer_t & out, uint16 q_type, strvlist const & name_parts);

    mdns_A_record_t * _lookup(string const & name);
    mdns_SRV_record_t * _lookup_srv(string const & name);

    void push_request(buffer_t * req);

    // Should we even be sending packets
    bool should_send();
  };

};

#endif  // __MWP_MDNS_HPP__
