
#include "mwp_types.hpp"
#include "mwp_slp.hpp"
#include "mwp_controller.hpp"
#include "mwp_utils.hpp"

#include <cstdio>
#include <cstdlib>

using namespace net_mobilewebprint::msg;
using namespace net_mobilewebprint::mq_enum;

using net_mobilewebprint::network_node_t;
using net_mobilewebprint::buffer_view_i;
using net_mobilewebprint::buffer_t;
using net_mobilewebprint::split_on_parens;
using net_mobilewebprint::parse_kv;

#define HP_SLP_MCAST_ADDR "224.0.1.60"
#define HP_SLP_PORT       427

#define SLP_V1          1

#define SLP_REQUEST     6
#define SLP_RESPONSE    7

#define SLP_US_ASCII    3

/**
 *  SLP: slp_t ctor.
 */
net_mobilewebprint::slp_t::slp_t(controller_base_t & controller_)
  : controller(controller_), mq(net_mobilewebprint::get_mq(controller_)), group_mcast(NULL), pth_resend_attr_req(NULL),
    resend_req(0, 2500, 1.1)
{
  _init_attribute_request();
  _init_group_mcast();

  mq.on(this);
  mq.on_selected(this);
}

std::string net_mobilewebprint::slp_t::mod_name()
{
  return "slp_t";
}

int net_mobilewebprint::slp_t::pre_select(mq_pre_select_data_t * pre_select_data)
{
  if (check_udp_writableness) {
    mq.check_udp_write(*group_mcast);
    check_udp_writableness = false;
  }
  return -1;
}

net_mobilewebprint::e_handle_result net_mobilewebprint::slp_t::on_select_loop_start(mq_select_loop_start_data_t const & loop_start_data)
{
  if (resend_req.has_elapsed(loop_start_data.current_loop_start)) {
    //log_d(1, "", "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ SLP timeout after %d", resend_req.interval);
    check_udp_writableness = true;
  }
}

e_handle_result net_mobilewebprint::slp_t::handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  if (name == scan_for_printers)        { return _scan_for_printers(name, payload, data, extra); }
  if (name == "_on_slp_payload")        { return _on_slp_payload(name, payload, data, extra); }

  return not_impl;
}

e_handle_result net_mobilewebprint::slp_t::_scan_for_printers(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  if (!mwp_assert(group_mcast)) { return unhandled; }
  mq.register_udp_for_select(*group_mcast);

  return handled;
}

e_handle_result net_mobilewebprint::slp_t::_mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra)
{
  if (!mwp_assert(group_mcast)) { return unhandled; }

  e_handle_result result = unhandled;

  if (extra.is_udp_writable(*group_mcast)) {
    //log_d(1, "", "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ sending SLP broadcast");
    group_mcast->send_udp_to(attribute_request);
    result = handled;
  }

  if (extra.is_udp_readable(*group_mcast)) {
    buffer_t              result_packet;
    network_node_t        sender;

    group_mcast->recv_udp_from(result_packet, sender, 2 * 1024);
    result = handled;

    char const * begin = NULL, *end = NULL;
    if (parse_packet(result_packet, begin, end)) {
      char msg_start_buf[64];
      /*int count =*/ mwp_sprintf(msg_start_buf, "(mwp-sender=%s)(mwp-port=%d)", sender.ip.c_str(), 9100);
      buffer_t * msg = mq.message("_on_slp_payload");

      msg->append_str_sans_null(msg_start_buf);
      msg->appendT(std::make_pair(begin, end));
      msg->dump("slp");
      mq.send(msg);
    }
    log_v("received from: %s:%d", sender.ip.c_str(), sender.port);
  }

  return result;
}

e_handle_result net_mobilewebprint::slp_t::_on_slp_payload(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  controller.from_slp(payload);
  return handled;
}

bool net_mobilewebprint::slp_t::parse_packet(buffer_view_i & payload, char const *& begin, char const *& end)
{
  buffer_view_i::const_iterator p = payload.first();

  mwp_assert(payload.read_byte(p) == 1);
  mwp_assert(payload.read_byte(p) == SLP_RESPONSE);

  /*uint16 packet_length =*/ payload.read_uint16(p);

  payload.read_byte(p);                                   // Flags
  payload.read_byte(p);                                   // Dialect
  payload.read_string_nz(p, 2);                           // Language (en)
  payload.read_uint16(p);                                 // Encoding (3 == us-ascii)
  payload.read_uint16(p);                                 // Transaction id

  mwp_assert(payload.read_uint16(p) == 0);                    // Error-code should be zero

  uint16 len = payload.read_uint16(p);
  return payload.read_string_nz(p, len, begin, end);
}

bool net_mobilewebprint::slp_t::parse(buffer_view_i const & payload, map<string, string> & attrs, map<string, string> & attrs_lc, deque<string> & attrs_not_kv)
{
  string  attributes_str(payload.const_begin(), payload.const_end());

  deque<string> attr_list = split_on_parens(attributes_str);

  return parse_kv(attr_list.begin(), attr_list.end(), attrs, '=', &attrs_lc, &attrs_not_kv) > 0;
}

net_mobilewebprint::slp_t const & net_mobilewebprint::slp_t::_init_group_mcast()
{
  if (group_mcast == NULL) {
    return _init_group_mcast(new network_node_t(HP_SLP_MCAST_ADDR, HP_SLP_PORT));
  }
  return *this;
}

net_mobilewebprint::slp_t const & net_mobilewebprint::slp_t::_init_group_mcast(network_node_t * node)
{
  if ((group_mcast = node) != NULL) {
    group_mcast->udp();
    group_mcast->set_sockopt(so_broadcast);

    // Join multicast group
    ip_mreq mreq;
    memset(&mreq, 0, sizeof(mreq));
    inet_pton(AF_INET, HP_SLP_MCAST_ADDR, &mreq.imr_multiaddr);

    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    group_mcast->set_sockopt(ip_add_membership, &mreq, sizeof(mreq));

    group_mcast->set_sockopt(ip_multicast_ttl, (u_char)32);
    group_mcast->set_sockopt(ip_ttl, 32);

    mq.register_udp_for_select(*group_mcast);
  }

  return *this;
}

net_mobilewebprint::slp_t const & net_mobilewebprint::slp_t::_init_attribute_request()
{
  static  char  lang[] = "en";
  static  char  service[] = "service:x-hpnp-discover:";

  byte          by = 0;
  uint16        sh = 0;

  int           size_offset = 0;                           // Offset of the uint16 that indicates the packet size

  attribute_request.append(by = SLP_V1);
  attribute_request.append(by = SLP_REQUEST);

  size_offset = attribute_request.append(sh = 0);          // Size - compute and set later

  attribute_request.append(by = 0);                        // Flags
  attribute_request.append(by = 0);                        // Dialect
  attribute_request.append_str_sans_null(lang);            // Language
  attribute_request.append(sh = SLP_US_ASCII);             // Encoding
  attribute_request.append(sh = 27000 + (rand() % 1000));  // Packet ID (xid)
  attribute_request.append(sh = 0);                        // Number in prev response list
  attribute_request.append(sh = strlen(service));          // The service we want
  attribute_request.append_str_sans_null(service);         // The service we want
  attribute_request.append(sh = 0);                        // Scope list length
  attribute_request.append(sh = 0);                        // Attribute list length

  // Go back and fill in the size
  attribute_request.set(size_offset, sh = attribute_request.data_length);

  return *this;
}

bool net_mobilewebprint::parse_slp(slp_t & slp, buffer_view_i const & payload, map<string, string> & attrs, map<string, string> & attrs_lc, deque<string> & other_entries)
{
  return slp.parse(payload, attrs, attrs_lc, other_entries);
}

