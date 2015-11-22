
#ifndef __MWP_SEND_TCP_JOB_HPP__
#define __MWP_SEND_TCP_JOB_HPP__

#include "hp_mwp.h"
#include "mwp_mq.hpp"

namespace net_mobilewebprint {

  struct controller_base_t;

  using namespace net_mobilewebprint::mq_enum;

  struct tcp_job_connection_t;

  struct send_tcp_job_t : public mq_select_handler_t, mq_handler_t
  {

    typedef std::map<uint32, tcp_job_connection_t *>  connections_t;

    controller_base_t &    controller;
    mq_t &                 mq;
    connections_t          connections;

    send_tcp_job_t(controller_base_t &);

    tcp_job_connection_t *      send_to_printer(uint32 id, string const & ip, uint16 port = 9100);

    virtual int                      pre_select(mq_pre_select_data_t * pre_select_data);
    virtual e_handle_result        _mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra);

    virtual e_handle_result              handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);

    bool                   connection_is_closed(connections_t::iterator & it, int txn_id);

    virtual string      mod_name();
  };

  struct tcp_job_connection_t : public mq_select_handler_t, mq_handler_t
  {
    controller_base_t &       controller;
    mq_t &                    mq;

    network_node_t *          printer;
    uint32                    connection_id;
    int                       http_response_code;
    string                    content_type;
    bool                      txn_closed;
    bool                      closed;

    int                       packet_num;

    std::deque<chunk_t *>     chunks;

    tcp_job_connection_t(controller_base_t &, network_node_t *);
    tcp_job_connection_t(controller_base_t &, network_node_t *, uint32 id);
    virtual ~tcp_job_connection_t();

    virtual int                      pre_select(mq_pre_select_data_t * pre_select_data);
    virtual e_handle_result        _mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra);

    virtual e_handle_result              handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);
            e_handle_result    _on_http_headers(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);
            e_handle_result    _on_http_payload(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);
            e_handle_result       _on_txn_close(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);

    virtual string          mod_name();
  };

};

#endif  // __MWP_SEND_TCP_JOB_HPP__


