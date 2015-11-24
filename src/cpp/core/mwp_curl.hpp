
#ifndef __MWP_CURL_HPP__
#define __MWP_CURL_HPP__

#include "mwp_mq.hpp"
#include "hp_mwp.h"

struct curl_slist;

namespace net_mobilewebprint {

  struct controller_base_t;

  using namespace net_mobilewebprint::mq_enum;

  struct curl_connection_t;
  typedef void CURLM;
  typedef void CURL;

  struct curl_t : public mq_select_handler_t, mq_handler_t
  {
    typedef std::map<uint32, curl_connection_t *>  connections_t;

    controller_base_t &    controller;
    mq_t &                 mq;

    connections_t          connections;

    string                 server_name;
    uint16                 server_port;
    string                 netapp_subdomain;

    CURLM *                mcurl;
    uint32                 connection_id;

    curl_t(controller_base_t &, string server_name, uint16 server_port);
    virtual ~curl_t();

    virtual string      mod_name();

    curl_connection_t * fetch_from_mwp_server(char const * verb, string const & path, uint32 connection_id);
    curl_connection_t * post_mwp_server(serialization_json_t const & json, string const & path, uint32 connection_id);
    curl_connection_t * post_mwp_server(string const & json_str, string const & path, uint32 connection_id);

    virtual int                  pre_select(mq_pre_select_data_t * pre_select_data);
            e_handle_result    _mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra);

    virtual e_handle_result          handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);

    connections_t::iterator        _find_by_curl_handle(CURL*);

    string                   translate_path(string const & url);

    int                         verbose_adj(string const & path);
  };

  struct curl_connection_t : public mq_select_handler_t
  {
    controller_base_t & controller;
    mq_t &              mq;
    curl_t *            parent;

    uint32              connection_id;
    string              verb;
    string              path;
    string              full_url;

    uint32              curl_code;
    long                http_code;

    CURLM *             mcurl;       // Owned by curl_t
    CURL *              curl;

    buffer_t *          recv_packet;
    int                 packet_num;
    size_t              num_recieved;

    strlist             headers;

    chunk_t *           request_payload;
    struct curl_slist * req_headers;

    // VERB path
    curl_connection_t(controller_base_t & controller, CURLM * mcurl, curl_t * parent,   char const * verb, string const & path, uint32 connection_id);

    // POST json_str
    curl_connection_t(controller_base_t & controller, CURLM * mcurl, curl_t * parent, string const & path, uint32 connection_id, string const & body_str);

    // POST json
    curl_connection_t(controller_base_t & controller, CURLM * mcurl, curl_t * parent, serialization_json_t const & json, string const & path, uint32 connection_id);
    virtual ~curl_connection_t();

    curl_connection_t & _init();
    curl_connection_t & _init_read_fn();
    curl_connection_t & _set_url(string const & url, char const * verb = "POST");
    curl_connection_t & _set_body(string const & body, bool do_logging = true);
    curl_connection_t & _set_body(serialization_json_t const & body, char const * verb = "POST");
    curl_connection_t & _go();

    virtual string          mod_name();

    virtual int                  pre_select(mq_pre_select_data_t * pre_select_data);
    virtual e_handle_result    _mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra);

    size_t  conn_write_data(void * buffer, size_t size, size_t nmemb);
    size_t conn_header_data(void * buffer, size_t size, size_t nmemb);
    size_t   conn_read_data(void * buffer, size_t size, size_t nmemb);

    int         verbose_adj();
  };
};

#endif  // __MWP_CURL_HPP__

