
#ifndef __MWP_CONTROLLER_HPP__
#define __MWP_CONTROLLER_HPP__

#include "mwp_mq.hpp"
#include "mwp_slp.hpp"
#include "mwp_mdns.hpp"
#include "mwp_snmp.hpp"
#include "mwp_bjnp.hpp"
#include "mwp_send_tcp_job.hpp"
#include "mwp_curl.hpp"
#include "mwp_mini_curl.hpp"
#include "mwp_printer.hpp"
#include "mwp_upstream.hpp"
#include "mwp_stats.hpp"
#include "hp_mwp.h"
#include "hp_sap.h"

#define MWP_SERVER_NAME "hqext.mobiledevprint.net"
//#define MWP_SERVER_NAME "hqpub.mobilewebprint.net"
#define MWP_SERVER_PORT 80

namespace net_mobilewebprint {

  namespace msg {
    extern string up_and_running;
    extern string scan_for_printers;
    extern string mq_selected;
    extern string __select__;
  };

  namespace tags {
    extern char const * TAG_DATA_FLOW;
  };

  struct mwp_app_callback_t {
    typedef hp_mwp_callback_t callback_t;

    std::string        name;
    void *             app_data;
    callback_t         callback;

    mwp_app_callback_t();
    mwp_app_callback_t(std::string name, void * app_data, hp_mwp_callback_t callback);
  };

  struct sap_app_callback_t {
    typedef hp_sap_callback_t callback_t;

    std::string        name;
    void *             app_data;
    callback_t         callback;

    sap_app_callback_t();
    sap_app_callback_t(std::string name, void * app_data, hp_sap_callback_t callback);
  };

  template <typename T>
  struct app_type_traits {
  };

  template <>
  struct app_type_traits<hp_mwp_callback_t>
  {
    typedef mwp_app_callback_t type_t;
  };

  template <>
  struct app_type_traits<hp_sap_callback_t>
  {
    typedef sap_app_callback_t type_t;
  };

  typedef std::map<std::string, mwp_app_callback_t>  mwp_app_cb_list_t;
  typedef std::map<std::string, sap_app_callback_t>  sap_app_cb_list_t;
  typedef std::map<int, std::string>                 app_timer_table_t;

  struct controller_http_request_t
  {
    uint32                txn_id;
    std::string           verb;
    std::string           url;
    serialization_json_t  json_body;

    controller_http_request_t(uint32 txn_id, std::string const & verb, std::string const & url, serialization_json_t const & json);
    controller_http_request_t(uint32 txn_id, std::string const & verb, std::string const & url, strmap const & query, serialization_json_t const & json);
  };

  struct server_command_response_t : public upstream_handler_t {
    virtual ~server_command_response_t();
    virtual void handle(int code, std::string const & http_version, strmap const & headers, json_array_t const & json, stats_t const & stats_out);
  };

  struct controller_base_t : public mq_handler_t
  {
    mq_t                mq;

    controller_base_t(mwp_app_callback_t *);
    controller_base_t(sap_app_callback_t *);
    ~controller_base_t();

    // --------------------------------------------------------------------------------
    //
    // API
    //
    // These functions are handling calls that originate from the app.
    //
    // --------------------------------------------------------------------------------

    // ---------- The main API ----------

    // Start the system
    bool start(bool start_scanning = true, bool block = true);
    bool mq_is_done();

    // Send a print job
    bool                    send_job(string const & url, string const & printer_ip);
    e_handle_result _allocate_job_id(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);
    e_handle_result        _send_job(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);

    // ---------- options / args ----------
    string const &               arg(char const * key);                         // returns "" if not found
    string const &               arg(char const * key, string const & def);
    int                          arg(char const * key, int def);
    bool                        flag(char const * key);
    bool                        flag(char const * key, bool def);

    controller_base_t &      set_arg(string const & name, string const & value);
    controller_base_t &      set_arg(string const & name, int value);
    controller_base_t &     set_flag(string const & name, bool value = true);

    controller_base_t &      set_arg(char const *name, char const *value);
    controller_base_t &      set_arg(char const *name, int value);
    controller_base_t &     set_flag(char const *name, bool value = true);
    controller_base_t &   clear_flag(char const *name);

    controller_base_t &    parse_cli(int argc, void const * argv[]);

    void                show_options();

    // ---------- The app is messaging with MWP ----------

    // The app calls this to register a handler
    bool register_handler(char const * name, void * app_data, hp_mwp_callback_t callback);
    bool register_handler(char const * name, void * app_data, hp_sap_callback_t callback);
    bool deregister_handler(char const * name);

    // The app sending a message
    int app_send(char const * name, char const * payload = NULL);
    int app_set_timeout(char const * message_to_send, int msecs_to_wait);

    // --------------------------------------------------------------------------------
    //
    // Internal-API
    //
    // These functions are handling calls that originate from within MWP.
    //
    // --------------------------------------------------------------------------------

    // ---------- The MQ handler ----------

    // The 3 functions that make up the mq_handler_t interface
    virtual e_handle_result on_select_loop_start(mq_select_loop_start_data_t const & data);
    virtual e_handle_result  on_select_loop_idle(mq_select_loop_idle_data_t const & data);

    virtual e_handle_result   handle           (string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);

    virtual string            mod_name();

    // The messages we handle
            e_handle_result     _up_and_running(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);
            e_handle_result    _on_http_headers(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);
            e_handle_result    _on_http_payload(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);
            e_handle_result       _on_txn_close(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);

    std::map<int, std::deque<chunk_t *> > chunkses;

    // ---------- General processing ----------
    bool from_slp(buffer_view_i const & payload);
    bool from_attrs(strmap const & attrs);
    bool from_attrs(strmap const & attrs, strlist const & tags);
    bool from_1284_attrs(strmap const & attrs);
    bool from_1284_attrs(strmap const & attrs, strlist const & tags);

    // ---------- Make network requests to get the data ----------

    // ---------- Any MWP module can send a message to the app ----------

    // Send a message to the app, takes the same args as the message that is sent
    int  send_to_app(char const *   name, int id, int32 transaction_id, uint8 const * p1, mwp_params const * params);
    int  send_to_app(char const *   name, int id, int32 transaction_id, uint8 const * p1, sap_params const * params);

    // Send a message to the app, sending three "C" strings
    int  send_to_app(string const & name, int id, int32 transaction_id,
                     char const * p1, char const * p2, char const * p3, char const * p4 = NULL, char const * p5 = NULL,
                     uint32 n1 = 0, uint32 n2 = 0, uint32 n3 = 0, uint32 n4 = 0, uint32 n5 = 0);

    // Send an easy message to the app
    int  send_to_app(string const & name, int id = -1, int32 transaction_id = 0);

    // Send an "internal" message to the app
    int  send_to_app(string const & name, int id, int32 transaction_id, buffer_view_i const & payload, buffer_t * data = NULL);

    // Send the full printer list
    bool send_full_printer_list();

    // ---------- Internal infrastructure for messaging with the app ----------

    // Our handler for any messages that are for the app that go through MQ
    e_handle_result            app_handler(string const & name, int id, int txn_id, buffer_view_i const & payload, buffer_t * data);

    // Our handler for timers that are for the app that go through MQ
    e_handle_result   _app_timeout_handler(string const & name, int id, int txn_id, buffer_view_i const & payload, buffer_t * data);

    // ---------- Stats ----------
    map<uint32, stats_t> job_stats;

    string    job_stat(uint32 id, char const * stat_name,   char const * value, bool silent = false);
    string    job_stat(uint32 id, char const * stat_name, string const & value, bool silent = false);
    int       job_stat(uint32 id, char const * stat_name,            int value, bool silent = false);
    int  job_stat_incr(uint32 id, char const * stat_name,            int value, bool silent = false);
    bool      job_stat(uint32 id, char const * stat_name,           bool value, bool silent = false);

    bool      job_stat_changed(uint32 id, bool did_change, char const * stat_name);

    map<string, stats_t> packet_stats;

    void      packet_stat(string const & ip, char const * stat_name, string const & value);
    void      packet_stat(string const & ip, char const * stat_name,            int value);
    void packet_stat_incr(string const & ip, char const * stat_name,            int value);
    void      packet_stat(string const & ip, char const * stat_name,           bool value);

    template <typename T>
    T _lookup_job_stat(uint32 id, char const * stat_name, T def)
    {
      typename map<uint32, stats_t>::iterator it = job_stats.find(id);
      if (it == job_stats.end()) {
        return def;
      }

      /* otherwise */
      stats_t const & stats = it->second;
      return stats.lookup(stat_name, def);
    }

    // ---------- Utilities for anyone ----------
    string                send_upstream(string const & mod_name, string const & endpoint, serialization_json_t & json, upstream_handler_t *);
    string                send_upstream(string const & mod_name, string const & endpoint, serialization_json_t & json);

    private:
      uint32               curl_http_post(string const & url, serialization_json_t &);
      uint32                curl_http_get(string const & url);

      uint32               curl_http_post(string const & url, serialization_json_t &, uint32 id);
      uint32               curl_http_post(controller_http_request_t const & request);
      uint32                curl_http_get(string const & url, uint32 id);

    friend struct upstream_t;
    public:

    // ---------- Internal to MWP ----------
    uint32    _unique();
    string    _unique(string prefix);

    /* protected-ish */
    slp_t                     slp;
    mdns_t                    mdns;
    snmp_t                    snmp;
    bjnp_t                    bjnp;
    send_tcp_job_t            job_flow;
    mini_curl_t               mini_curl;
    curl_t                    curl;
    printer_list_t            printers;
    upstream_t                upstream;

    int                       client_start_in_flight_txn_id;

    upstream_handler_map_t    upstream_messages;

    uint32                    scan_start_time;

    uint32                    mq_report_time,                 mq_report_interval;
    mq_manual_timer_t         upload_job_stats;
    mq_manual_timer_t         alloc_report;
    mq_manual_timer_t         mq_report;
    mq_manual_timer_t         packet_stats_report;
    mq_manual_timer_t         cleanup_time;
    mq_manual_timer_t         server_command_timer;
    //uint32                    job_stats_upload_time,          job_stats_upload_interval;

    app_timer_table_t         timer_table;
    mwp_app_cb_list_t *       mwp_app_callbacks_;
    sap_app_cb_list_t *       sap_app_callbacks_;

    mwp_app_cb_list_t &       mwp_app_callbacks();
    sap_app_cb_list_t &       sap_app_callbacks();

    args_t                    ARGS;

    uint32                    unique_number;
    string                    clientId();

    std::deque<controller_http_request_t>* delayed_http_requests;

    // Make an HTTP request that we (the controller) will handle
    uint32                     _make_http_post(char const * url, strmap const & query, serialization_json_t &);

    //
    e_handle_result process_upstream_response(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);
    e_handle_result _on_progress_response(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);

    void handle_server_command(int code, std::string const & http_version, strmap const & headers, json_array_t const & json, stats_t const & stats_out);

    // Log an api if that option is set
    void                log_api(char const * format, ...);

    /* private-ish */
    template <typename T> controller_base_t & set_args(int argc, T const *argv[]) {
      ARGS.merge(ARGS(argc, argv));
    }

    // ---------------------- Heartbeat ----------------------
    mq_manual_timer_t         heartbeat_timer;

    // ---------------------- Telemetry ----------------------
    string                    sessionId;
    mq_manual_timer_t         telemetry_report;

    map<string, serialization_json_t>      buckets;      // Data common to the bucket
    map<string, jsonlist>     bucketData;   // Data items
    void                      startBucket(string bucket, uint32 start_time = 0);
    void                      sendTelemetry(string bucket, char const * eventType, serialization_json_t const &);

    template <typename T>
    void                      sendTelemetry(char const * bucketName,
                                            char const * eventType,
                                            char const * key, T const & value)
    {
      serialization_json_t json;
      json.set(key, value);
      sendTelemetry(bucketName, eventType, json);
    }

    template <typename T1, typename T2, typename T3>
    void                      sendTelemetry(char const * bucketName,
                                            char const * eventType,
                                            char const * key1, T1 const & value1,
                                            char const * key2, T2 const & value2,
                                            char const * key3, T3 const & value3)
    {
      serialization_json_t json;
      json.set(key1, value1);
      json.set(key2, value2);
      json.set(key3, value3);
      sendTelemetry(bucketName, eventType, json);
    }

    template <typename T1, typename T2, typename T3, typename T4>
    void                      sendTelemetry(char const * bucketName,
                                            char const * eventType,
                                            char const * key1, T1 const & value1,
                                            char const * key2, T2 const & value2,
                                            char const * key3, T3 const & value3,
                                            char const * key4, T4 const & value4)
    {
      serialization_json_t json;
      json.set(key1, value1);
      json.set(key2, value2);
      json.set(key3, value3);
      json.set(key4, value4);
      sendTelemetry(bucketName, eventType, json);
    }

  };

  struct telemetry_response_t : public upstream_handler_t {
    virtual void handle(int code, std::string const & http_version, strmap const & headers, json_array_t const & json, stats_t const & stats_out);
  };

  extern controller_base_t * g_controller;

  extern const int max_poll_timeout;
  extern const float poll_timeout_ratio;

  extern string const & get_option(char const * key);                         // returns "" if not found
  extern string const & get_option(char const * key, string const & def);
  extern int            get_option(char const * key, int def);
  extern bool             get_flag(string const & key);

  // Send a message to the app, takes the same args as the message that is sent
  int  send_to_app(char const * name, int id, int32 transaction_id, uint8 const * p1 = NULL, mwp_params const * params = NULL);
  int  send_to_app(char const * name, int id, int32 transaction_id, uint8 const * p1 = NULL, sap_params const * params = NULL);

};


#endif  // __MWP_CONTROLLER_HPP__
