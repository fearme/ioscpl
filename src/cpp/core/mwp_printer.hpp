
#ifndef __MWP_PRINTER_HPP__
#define __MWP_PRINTER_HPP__

#include "mwp_types.hpp"
#include "mwp_utils.hpp"
#include "mwp_mq.hpp"
#include "mwp_upstream.hpp"

#include "mwp_bjnp.hpp"
#include "mwp_send_tcp_job.hpp"

#include <string>
#include <map>
#include <set>
#include <deque>

namespace net_mobilewebprint {

  struct controller_base_t;
  struct slp_t;
  struct mdns_t;

  using std::string;
  using std::deque;
  using std::set;
  using std::map;
  using std::make_pair;
  using std::pair;

  struct printer_t;

  struct fixup_snapshot_t {
    string ip;
    int    port;
    fixup_snapshot_t(printer_t const &);
    void fixup(printer_t &);
  };

  struct printer_t
  {
    controller_base_t &             controller;

    // Necessary
    string   ip;                      // Special -- the IP address
    int      port;                    // Special -- the port
    string   _1284_device_id;         // Special -- the device id
    string   mac;                     // Special -- the MAC address
    bool *   is_supported;            // Special -- the server can generate PCL for this model
    int      num_is_supported_asks;
    int      score() const;           // Special -- the display score
    string   name() const;            // Special -- the name
    string   status() const;          // Special -- the current status

    strmap   attrs, attrs_lc;
    strlist  tags;
    strmap   _1284_attrs, _1284_attrs_lc;

    int      num_soft_network_errors;

    uint32   last_status_arrival;
    bool     status_request_pending;
    int      num_status_misses;

    uint32   status_time;
    uint32   status_interval;

    printer_t(controller_base_t & controller);
    printer_t(controller_base_t & controller, string const & ip);

    ~printer_t();

    int   port_for_proto(int udp_tcp, int proto);    // SOCK_DGRAM or SOCK_STREAM

    void dump();

    bool has_ip() const;

    bool has_mac() const;
    void set_mac(string const & mac);

    printer_t const & merge(printer_t const & that);

    bool from_attrs(strmap const & attrs);
    bool from_attrs(strmap const & attrs, strlist const & tags);

    bool from_1284_attrs(strmap const & attrs);
    bool from_1284_attrs(strmap const & attrs, strlist const & tags);

    bool from_snmp(string ip, map<string, buffer_view_i const *> const & attrs);
    bool from_slp(slp_t & slp, buffer_view_i const & payload);

    void send_print_job(uint32 & connection_id);

    string attr_list();
    strmap filtered_attrs(std::set<std::string> names);

    bool request_updates();

    virtual mq_enum::e_handle_result on_select_loop_start(mq_select_loop_start_data_t const & data);

    void set_attr(string const & key, string const & key_lc, string const & value);

    string to_json(bool for_debug);
    void   make_server_json(serialization_json_t & json);
    bool   is_unknown(char const * purpose) const;
    bool   is_missing();

    /* private */
    network_node_t node;
    tcp_job_connection_t *  connection;
    uint32                  connection_id;

//    uint32                  bjnp_send_to_printer_start;
    bjnp_connection_t *     bjnp_connection;

    static uint32 status_interval_normal;
    static uint32 status_interval_printing;
  };

  struct printer_list_response_t : public upstream_handler_t {
    virtual void handle(int code, std::string const & http_version, strmap const & headers, json_array_t const & json, stats_t const & stats_out);
  };

  struct printer_list_t
  {

    typedef map<string, printer_t*> plist_t;

    controller_base_t &        controller;
    mq_t &                     mq;

    plist_t                    by_ip;
    plist_t                    by_mac;

    printer_list_t(controller_base_t &controller_);

    // Get a printer from its IP (or mac)
    printer_t * _get_printer(string const & ip_or_mac);

    bool from_attrs(strmap const & attrs);
    bool from_attrs(strmap const & attrs, strlist const & tags);

    bool from_1284_attrs(strmap const & attrs);
    bool from_1284_attrs(strmap const & attrs, strlist const & tags);

    bool from_snmp(string ip, map<string, buffer_view_i const *> const & attrs);
    bool from_slp(slp_t & slp, buffer_view_i const & payload);
    bool assimilate_printer_stats(printer_t * printer);
    int  send_list_to_app();

    void soft_network_error(string const & ip, int error_number);
    void network_error(string const & ip, int error_number);
    void remove_printer(printer_t*&);

    void send_print_job(uint32 & connection_id, string const & ip);

    string get_device_id(string const & ip);
    int   port_for_proto(string const & ip, int udp_tcp, int proto);    // SOCK_DGRAM or SOCK_STREAM

    bool cleanup();

    uint32 update_time;
    bool request_updates();

    virtual mq_enum::e_handle_result   on_select_loop_start(mq_select_loop_start_data_t const & data);
    virtual mq_enum::e_handle_result    on_select_loop_idle(mq_select_loop_idle_data_t  const & data);
    virtual mq_enum::e_handle_result                 handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);

    string to_json(bool for_debug);
    int    make_server_json(serialization_json_t & json, char const * purpose = NULL);

    // 'protected'
    static std::set<std::string> app_attribute_names;
    int printer_enum_id;

    bool send_begin_msg(bool & have_sent_begin_msg);
    bool next_best_score(int & score);

    uint32                    start_time;

    mq_manual_timer_t         heartbeat;

    mq_manual_timer_t         sending_printer_list;
    bool                      printer_list_in_flight;

    void handle_filter_printers(int code, std::string const & http_version, strmap const & headers, json_array_t const & json, stats_t const & stats_out);

    mq_manual_timer_t *       send_scan_done;
    mq_manual_timer_t *       send_scan_done_zero_printers;
    mq_manual_timer_t *       send_scan_done_last_resort;
    void                      _start_timers();
    void                      scan_activity_happened(uint32 now = 0);
    void                      re_scan();

    printer_t * has_unknown_is_supported();
    int         unknown_is_supported_count();

    string      get_ip(string const & mac);

    static string get_pml_status(string const & status, bool & is_universal_status);

  };

};

#endif  // __MWP_PRINTER_HPP__
