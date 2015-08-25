
#ifndef __MWP_MDNS_HPP__
#define __MWP_MDNS_HPP__

#include "mwp_mq.hpp"
#include "mwp_socket.hpp"

#include <string>

namespace net_mobilewebprint {

  using std::string;

  struct controller_t;

  namespace mdns {
    enum record_type {
      a         =   1,  /* 0x0001 */
      cname     =   5,  /* 0x0005 */
      soa       =   6,  /* 0x0006 */
      ptr       =  12,  /* 0x000c */
      mx        =  15,  /* 0x000f */
      txt       =  16,  /* 0x0010 */
      aaaa      =  28,  /* 0x001c */
      srv       =  33,  /* 0x0021 */
      cert      =  37,  /* 0x0025 */
      ipseckey  =  45,  /* 0x002d */

      last      = 0xffff
    };
  };

  struct mdns_header_t
  {
    string              record_name;
    uint16              type;
    uint16              flags;
    uint32              ttl;
    uint16              data_length;
    buffer_reader_t     data;

    mdns_header_t(buffer_reader_t & reader, bool is_question = false);

    buffer_reader_t data_reader() const;
  };

  struct mdns_record_base_t
  {
    string              record_name;
    uint16              type;
    uint16              flags;
    uint32              ttl;

    mdns_record_base_t(mdns_header_t const &);
  };

  struct mdns_a_record_t : public mdns_record_base_t
  {
    string ip;

    mdns_a_record_t(mdns_header_t const &);
  };
  typedef deque<mdns_a_record_t> a_record_list;

  struct mdns_srv_record_t : public mdns_record_base_t
  {
    uint16              priority;
    uint16              weight;
    uint16              port;
    string              target;

    mdns_srv_record_t(mdns_header_t const &);
  };
  typedef deque<mdns_srv_record_t> srv_record_list;

  struct mdns_ptr_record_t : public mdns_record_base_t
  {
    string value;

    mdns_ptr_record_t(mdns_header_t const &);

    string   key() const;
    string  name() const;
  };
  typedef deque<mdns_ptr_record_t> ptr_record_list;

  struct mdns_txt_record_t : public mdns_record_base_t
  {
    strmap dict;

    mdns_txt_record_t(mdns_header_t const &);
  };
  typedef deque<mdns_txt_record_t> txt_record_list;

  struct mdns_parsed_packet_t
  {
    //buffer_t *    buffer;
    strmap          properties;
    a_record_list   a_records;
    ptr_record_list ptr_records;
    srv_record_list srv_records;
    txt_record_list txt_records;

    mdns_parsed_packet_t(buffer_view_t const & payload);

    static string read_stoopid_mdns_string(buffer_reader_t &);

    mdns_a_record_t   const *   get_A(string const & key);
    mdns_srv_record_t const * get_SRV(string const & key);
  };

  struct mdns_t : public mq_handler_t
  {
    controller_t &          controller;
    mq_t         &          mq;

    udp_socket_t            socket;

    static uint16           next_transaction_id;
    static buffer_t         pdl_query_request;
    static buffer_t         bjnp_query_request;

    mdns_t(controller_t &);

    void    on_scan_for_printers(string const & name, buffer_view_t const & payload, message_extra_t & extra);
    void          on_mdns_packet(string const & name, buffer_view_t const & payload, message_extra_t & extra);

    static  buffer_t &  _mk_query_request_packet(buffer_t & result, mdns::record_type type, char const * service_name);

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

#endif  // __MWP_MDNS_HPP__

