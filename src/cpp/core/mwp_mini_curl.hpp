
#ifndef __MWP_MINI_CURL_HPP__
#define __MWP_MINI_CURL_HPP__

#include "mwp_mq.hpp"
#include "hp_mwp.h"

namespace net_mobilewebprint {

  struct controller_base_t;

  using namespace net_mobilewebprint::mq_enum;

  struct mini_curl_connection_t;

  struct mini_curl_t : public mq_select_handler_t
  {
    typedef std::map<uint32, mini_curl_connection_t *>  connections_t;

    controller_base_t &    controller;
    mq_t &                 mq;
    connections_t          connections;

    string                 server_name;
    uint16                 server_port;

    //mini_curl_connection_t * connection;

    mini_curl_t(controller_base_t &, string server_name, uint16 server_port);

    mini_curl_connection_t * fetch_from_mwp_server(char const * verb, string const & path, uint32 txn_id);
    mini_curl_connection_t * post_mwp_server(serialization_json_t const & json, string const & path, uint32 connection_id);
    mini_curl_connection_t * post_mwp_server(string const & json_str, string const & path, uint32 connection_id);

    virtual int                      pre_select(mq_pre_select_data_t * pre_select_data);
    virtual e_handle_result        _mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra);

    bool        connection_is_closed(connections_t::iterator & it, int txn_id);
    virtual string      mod_name();
    network_node_t *    _connect_to(std::string path);
  };

  struct mini_curl_connection_t : public mq_select_handler_t
  {
    controller_base_t &    controller;
    mq_t &                 mq;

    network_node_t *       server;

    uint32                 txn_id;
    string                 verb;
    string                 path;
    bool                   closed;

    buffer_t *             recv_packet;
    int                    packet_num;
    size_t                 num_recieved;

    strlist                headers;

    buffer_t               request_payload;
    bool                   request_sent;

    mini_curl_connection_t(controller_base_t &, network_node_t *, char const * verb, string const & path, uint32 txn_id);
    mini_curl_connection_t(controller_base_t &, network_node_t *, serialization_json_t const & bodyJson, string const & path, uint32 txn_id);
    mini_curl_connection_t(controller_base_t &, network_node_t *, string const & path, uint32 txn_id, string const & bodyJson);
    virtual ~mini_curl_connection_t();

    virtual int                      pre_select(mq_pre_select_data_t * pre_select_data);
    e_handle_result                _mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra);

    virtual string          mod_name();
  };

};

#endif  // __MWP_MINI_CURL_HPP__

