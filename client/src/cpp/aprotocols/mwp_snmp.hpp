
#ifndef __MWP_SNMP_HPP__
#define __MWP_SNMP_HPP__

#include "mwp_types.hpp"
#include "mwp_udp.hpp"
#include "mwp_mq.hpp"

#include <string>
#include <vector>

namespace net_mobilewebprint {

  using namespace net_mobilewebprint::mq_enum;

  struct snmp_t : public mq_select_handler_t, mq_handler_t
  {
    controller_base_t &            controller;
    mq_t &                         mq;

    network_node_t *               socket;
    request_queue_with_dest_t      requests;

    snmp_t(controller_base_t &);

    void send_device_id_request(string const & ip, char const * oid);
    void send_status_request(string const & ip, char const * oid);

    virtual int                      pre_select(mq_pre_select_data_t * pre_select_data);
    virtual e_handle_result        _mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra);

    virtual e_handle_result              handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);
    e_handle_result              _on_raw_packet(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);
    e_handle_result                    _on_work(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);

    /* private */
    snmp_t & _init();
    snmp_t & _send_work_packet(uint32 work_id, string const & ip, char const * oid);

    buffer_t                  device_id_pkt;
    buffer_t                  status_pkt;

    uint32                    cleanup_time,   cleanup_interval;

    buffer_t const *            get_request_pkt(string const & oid);
    buffer_t const *            get_request_pkt(char const * oid);
    std::map<string, buffer_t > request_pkts;

    virtual string  mod_name();
  };

  namespace snmp {

    using std::string;
    using std::vector;

    struct snmp_t
    {
      udp2_t udp;

      bool _1284_device_id(network_node_t & network_node, string & result);
      bool _1284_device_id(network_node_t & network_node, string & result, string & key);
      bool status(network_node_t & network_node, string & result);
      bool status(network_node_t & network_node, string & result, string & key);
    };

    bool get_snmp(udp2_t udp, buffer_t pkt, network_node_t & network_node, string & result, string * pkey = NULL);

    buffer_t decode(buffer_view_i const & response, string & key);

    buffer_t get_1284_pkt();
    buffer_t get_status_pkt();

    buffer_t get_oid_pkt(int const * pn, int count);
    buffer_t get_oid_pkt(char const * oid);
    buffer_t oid_field(int const * pn, int count);
    buffer_t octet_string(char const * s);
    buffer_t sequence(vector<buffer_t> const & seq);
    buffer_t sequence(buffer_t a);
    buffer_t sequence(buffer_t a, buffer_t b);
    buffer_t sequence(buffer_t a, buffer_t b, buffer_t c);
    buffer_t sequence(buffer_t a, buffer_t b, buffer_t c, buffer_t d);
    buffer_t sequence(buffer_t a, buffer_t b, buffer_t c, vector<buffer_t> seq);
    buffer_t get_request(buffer_t a, buffer_t b, buffer_t c, buffer_t d);
    buffer_t encode(int n);

    buffer_t binary_oid(char const * str_oid);

  };
};

#endif // __MWP_SNMP_HPP__
