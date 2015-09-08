
#include "mwp_controller.hpp"
#include "mwp_snmp.hpp"
#include "mwp_utils.hpp"
#include "mwp_assert.hpp"

using namespace net_mobilewebprint;
using net_mobilewebprint::mq_result;

using std::string;

//---------------------------------------------------------------------------------------------------
//------------------------------------- snmp_t ------------------------------------------------------
//---------------------------------------------------------------------------------------------------

net_mobilewebprint::snmp_t::snmp_t(controller_t & controller_)
  : controller(controller_), mq(controller_.mq)
{
  name_ = "snmp";

  mq.register_handler(*this);
}

mq_result net_mobilewebprint::snmp_t::initialize()
{
//  // Set options on socket
//  socket.set_sockopt(SO_BROADCAST, 1);
//  socket.set_sockopt(IP_MULTICAST_TTL, 32);
//  socket.set_sockopt(IP_TTL, 32);
//
//  // Multicast
//  ip_mreq mreq;
//  memset(&mreq, 0, sizeof(mreq));
//  inet_pton(AF_INET, "224.0.0.251", &mreq.imr_multiaddr);
//
//  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
//
//  socket.set_sockopt(IP_ADD_MEMBERSHIP, &mreq);
//
//  // Build packets
//  _mk_query_request_packet(pdl_query_request, mdns::ptr, "_pdl-datastream");
//  _mk_query_request_packet(bjnp_query_request, mdns::ptr, "_canon-bjnp1");

  return not_impl;
}

mq_result net_mobilewebprint::snmp_t::on_select_loop_start(select_loop_start_extra_t const & extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::snmp_t::on_pre_select(pre_select_extra_t & extra)
{
//  //printf("MDNS pre-select num requests:%d\n", requests.size());
//
//  // Always look for readable
//  extra.fd_set_readable(socket.fd);
//
//  if (requests.size() > 0) {
//    extra.fd_set_writable(socket.fd);
//  }

  return ok;
}

mq_result net_mobilewebprint::snmp_t::on_select(select_extra_t & extra)
{
//  //printf("MDNS select\n");
//  buffer_t * packet = NULL;
//
//  // Should we send a request out?
//  if (requests.size() > 0) {
//    if (extra.is_writable(socket.fd)) {
//      if ((packet = pull_next_request()) != NULL) {
//        //printf("MDNS sending packet\n");
//        /*int num_sent =*/ socket.send_to(*packet);
//
//        // Free memory for the packet, unless it is a magic one
//        if (packet != &pdl_query_request && packet != &bjnp_query_request) {
//          delete packet;
//        }
//      }
//    }
//  }
//
//  // Are there any packets to read?
//  uint32 ip_offset = 0;
//  int    num_recvd = 0;
//  if (extra.is_readable(socket.fd)) {
//    //printf("MDNS reading packet\n");
//    if ((packet = mq.message("mdns_packet", 0, sizeof(uint32) + sizeof(uint16), ip_offset)) != NULL) {
//      string sender_ip_str;
//      uint16 sender_port = 0;
//      if ((num_recvd = socket.recv_from(*packet, sender_ip_str, sender_port)) > 0) {
//        uint32 sender_ip = 0;
//        inet_pton(AF_INET, sender_ip_str.c_str(), &sender_ip);
//        sender_ip = htonl(sender_ip);                                 // Must convert back to host byte-order
//        packet->set_at(ip_offset, sender_ip);
//        packet->set_at(ip_offset + sizeof(sender_ip), sender_port);
//
//        mq.send(packet);
//      }
//    }
//  }

  return ok;
}

mq_result net_mobilewebprint::snmp_t::on_message(string const & name_, buffer_view_t const & payload, message_extra_t & extra)
{
//  printf("MDNS on_message |%s|\n", name_.c_str());
//
//  char const * name = name_.c_str();
//  if (strcmp("scan_for_printers", name) == 0)    { on_scan_for_printers(name, payload, extra); }
//  else if (strcmp("mdns_packet", name) == 0)     { on_mdns_packet(name, payload, extra); }

  return not_impl;
}

mq_result net_mobilewebprint::snmp_t::on_select_loop_end(select_loop_end_extra_t const &   extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::snmp_t::on_select_loop_idle(select_loop_idle_extra_t const &  extra)
{
  return not_impl;
}

void net_mobilewebprint::snmp_t::on_scan_for_printers(string const & name, buffer_view_t const & payload, message_extra_t & extra)
{
//  push_request(&pdl_query_request);
//  push_request(&bjnp_query_request);
}

//void net_mobilewebprint::snmp_t::on_mdns_packet(string const & name, buffer_view_t const & payload, message_extra_t & extra)
//{
//  //printf("MDNS received packet\n");
//  packet_list.push_back(mdns_parsed_packet_t(payload, *this));
//}

//buffer_t & net_mobilewebprint::snmp_t::_mk_query_request_packet(buffer_t & result, mdns::record_type type, char const * service_name)
//{
////  uint16 sh = 0;
////  byte   by = 0;
////
////  result.append(sh = next_transaction_id++);
////  result.append(sh = 0);                            // flags
////
////  result.append(sh = 1);                            // number of questions
////  result.append(sh = 0);                            // number of answers
////  result.append(sh = 0);                            // number of authority records
////  result.append(sh = 0);                            // number of additional records
////
////  result.append_byte_and_string_nz(service_name);
////  result.append_byte_and_string_nz("_tcp");
////  result.append_byte_and_string_nz("local");
////
////  result.append(by = 0);
////  result.append(sh = type);
////  result.append(sh = 0x8001);                       // QU, IN
//
//  return result;
//}


