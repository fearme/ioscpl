
#ifndef __MWP_UPSTREAM_HPP__
#define __MWP_UPSTREAM_HPP__

#include "mwp_types.hpp"
#include "mwp_mq.hpp"
#include "mwp_stats.hpp"
#include <map>
#include <string>

namespace net_mobilewebprint {

  using std::map;
  using std::string;

  using namespace net_mobilewebprint::mq_enum;

  struct upstream_response_t
  {
    // when the response arrives, it will be put into the MQ.  This is the transaction name of this
    // specific message.  It is unique.
    string    txn_name;

    // When the response arrives, it will be put into the MQ.  This will be the name of the message.
    // It is not necessarily unique.
    string    message_name;

    // The headers and body
    chunks_t  headers_chunks;
    chunks_t  body_chunks;

    // An arbitrary key/value object (one level, but multiple types) that is specified when the request
    // is sent.  It does not travel to the server, but is part of the "payload" when the response is
    // parsed -- the caller of parse_response gets it.
    stats_t   stats;

    upstream_response_t(string const & txn_name, string const & message_name, stats_t const & stats);
    ~upstream_response_t();
  };

  /**
   *  The class that organizes an HTTP request/response.
   *
   *  * Manages JSON bodies for both the request and response.
   *  * Manages ability to contact other server via the mwp server. TBD
   *  * Does not do the HTTP communication at all; that is delegated to the controller object.
   */

  struct upstream_t : public mq_handler_t
  {
    typedef std::map<uint32, upstream_response_t *> response_map;

    controller_base_t &    controller;
    mq_t &                 mq;
    response_map           responses;

    upstream_t(controller_base_t &);

    // All the 'send' variants POST to the server
    // The variants that return a string will return the txn_name (to identify the specific response)

    string send(string const & mod_name, string const & endpoint, serialization_json_t & json);
//    string send(string const & mod_name, string const & endpoint,         string const & body);

    string send(string const & endpoint, serialization_json_t & json,                          string const & message_name, stats_t const & stats);

    void   send(string const & endpoint, serialization_json_t & json, string const & txn_name, string const & message_name, stats_t const & stats);
//    void   send(string const & endpoint,         string const & body, string const & txn_name, string const & message_name);

    // All the 'get' variants GET to the server
    // The variants that return a string will return the txn_name (to identify the specific response)

    string  get(string const & endpoint,                                                       string const & message_name, stats_t const & stats);

    void    get(string const & endpoint,                              string const & txn_name, string const & message_name);
    void    get(string const & endpoint,                                        uint32 txn_id, string const & message_name);

    // Parse the response.  payload is the only 'in' param
    bool   parse_response(buffer_view_i const & payload, int & code,                                          json_array_t & json,     stats_t & stats_out);
    bool   parse_response(buffer_view_i const & payload, int & code, string & http_version, strmap & headers, json_array_t & json,     stats_t & stats_out);
    bool   parse_response(buffer_view_i const & payload, int & code,                                          json_t & json,           stats_t & stats_out);
    bool   parse_response(buffer_view_i const & payload, int & code, string & http_version, strmap & headers, json_t & json,           stats_t & stats_out);
    bool   parse_response(buffer_view_i const & payload, int & code, string & http_version, strmap & headers, string & body_str,       stats_t & stats_out);

    // ---------- mq_handler_t
    virtual e_handle_result   handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);

    virtual string            mod_name();
  };

  struct upstream_handler_t {
    virtual ~upstream_handler_t() = 0;
    virtual void handle(int code, string const & http_version, strmap const & headers, json_array_t const & json, stats_t const & stats_out) = 0;
  };
  typedef std::map<std::string, upstream_handler_t*> upstream_handler_map_t;

};


#endif  // __MWP_UPSTREAM_HPP__
