
#include "mwp_controller.hpp"
#include "mwp_mdns.hpp"
#include "mwp_utils.hpp"

using namespace net_mobilewebprint;
using net_mobilewebprint::mq_result;

using std::string;

//---------------------------------------------------------------------------------------------------
//------------------------------------- mdns_t ------------------------------------------------------
//---------------------------------------------------------------------------------------------------
uint16    net_mobilewebprint::mdns_t::next_transaction_id = 4;
buffer_t  net_mobilewebprint::mdns_t::pdl_query_request;
buffer_t  net_mobilewebprint::mdns_t::bjnp_query_request;

net_mobilewebprint::mdns_t::mdns_t(controller_t & controller_)
  : controller(controller_), mq(controller_.mq), socket("224.0.0.251", 5353)
{
  name_ = "mdns";

  mq.register_handler(*this);
}

mq_result net_mobilewebprint::mdns_t::initialize()
{
  // Set options on socket
  socket.set_sockopt(SO_BROADCAST, 1);
  socket.set_sockopt(IP_MULTICAST_TTL, 32);
  socket.set_sockopt(IP_TTL, 32);

  // Multicast
  ip_mreq mreq;
  memset(&mreq, 0, sizeof(mreq));
  inet_pton(AF_INET, "224.0.0.251", &mreq.imr_multiaddr);

  mreq.imr_interface.s_addr = htonl(INADDR_ANY);

  socket.set_sockopt(IP_ADD_MEMBERSHIP, &mreq);

  // Build packets
  _mk_query_request_packet(pdl_query_request, mdns::ptr, "_pdl-datastream");
  _mk_query_request_packet(bjnp_query_request, mdns::ptr, "_canon-bjnp1");

  return not_impl;
}

mq_result net_mobilewebprint::mdns_t::on_select_loop_start(select_loop_start_extra_t const & extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::mdns_t::on_pre_select(pre_select_extra_t & extra)
{
  //printf("MDNS pre-select num requests:%d\n", requests.size());

  // Always look for readable
  extra.fd_set_readable(socket.fd);

  if (requests.size() > 0) {
    extra.fd_set_writable(socket.fd);
  }

  return ok;
}

mq_result net_mobilewebprint::mdns_t::on_select(select_extra_t & extra)
{
  //printf("MDNS select\n");
  buffer_t * packet = NULL;

  // Should we send a request out?
  if (requests.size() > 0) {
    if (extra.is_writable(socket.fd)) {
      if ((packet = pull_next_request()) != NULL) {
        //printf("MDNS sending packet\n");
        /*int num_sent =*/ socket.send_to(*packet);

        // Free memory for the packet, unless it is a magic one
        if (packet != &pdl_query_request && packet != &bjnp_query_request) {
          delete packet;
        }
      }
    }
  }

  // Are there any packets to read?
  uint32 ip_offset = 0;
  int    num_recvd = 0;
  if (extra.is_readable(socket.fd)) {
    //printf("MDNS reading packet\n");
    if ((packet = mq.message("mdns_packet", 0, sizeof(uint32) + sizeof(uint16), ip_offset)) != NULL) {
      string sender_ip_str;
      uint16 sender_port = 0;
      if ((num_recvd = socket.recv_from(*packet, sender_ip_str, sender_port)) > 0) {
        uint32 sender_ip = 0;
        inet_pton(AF_INET, sender_ip_str.c_str(), &sender_ip);
        sender_ip = htonl(sender_ip);                                 // Must convert back to host byte-order
        packet->set_at(ip_offset, sender_ip);
        packet->set_at(ip_offset + sizeof(sender_ip), sender_port);

        mq.send(packet);
      }
    }
  }

  return ok;
}

mq_result net_mobilewebprint::mdns_t::on_message(string const & name_, buffer_view_t const & payload, message_extra_t & extra)
{
  printf("MDNS on_message |%s|\n", name_.c_str());

  char const * name = name_.c_str();
  if (strcmp("scan_for_printers", name) == 0)    { on_scan_for_printers(name, payload, extra); }
  else if (strcmp("mdns_packet", name) == 0)     { on_mdns_packet(name, payload, extra); }

  return not_impl;
}

mq_result net_mobilewebprint::mdns_t::on_select_loop_end(select_loop_end_extra_t const &   extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::mdns_t::on_select_loop_idle(select_loop_idle_extra_t const &  extra)
{
  return not_impl;
}

void net_mobilewebprint::mdns_t::on_scan_for_printers(string const & name, buffer_view_t const & payload, message_extra_t & extra)
{
  push_request(&pdl_query_request);
  push_request(&bjnp_query_request);
}

void net_mobilewebprint::mdns_t::on_mdns_packet(string const & name, buffer_view_t const & payload, message_extra_t & extra)
{
  //printf("MDNS received packet\n");
  mdns_parsed_packet_t mdns_packet(payload);
}

buffer_t & net_mobilewebprint::mdns_t::_mk_query_request_packet(buffer_t & result, mdns::record_type type, char const * service_name)
{
  uint16 sh = 0;
  byte   by = 0;

  result.append(sh = next_transaction_id++);
  result.append(sh = 0);                            // flags

  result.append(sh = 1);                            // number of questions
  result.append(sh = 0);                            // number of answers
  result.append(sh = 0);                            // number of authority records
  result.append(sh = 0);                            // number of additional records

  result.append_byte_and_string_nz(service_name);
  result.append_byte_and_string_nz("_tcp");
  result.append_byte_and_string_nz("local");

  result.append(by = 0);
  result.append(sh = type);
  result.append(sh = 0x8001);                       // QU, IN

  return result;
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- mdns_parsed_packet_t ----------------------------------------
//---------------------------------------------------------------------------------------------------
net_mobilewebprint::mdns_parsed_packet_t::mdns_parsed_packet_t(buffer_view_t const & payload)
{
  buffer_reader_t payload_reader(payload);

  string ip           = payload_reader.read_ip();
  uint16 port         = payload_reader.read_uint16();

  buffer_range_t  sub_payload = mk_buffer_range(payload_reader);
  //sub_payload.dump("mdns parsing");

  buffer_reader_t reader(sub_payload);

  uint16 id           = reader.read_uint16();
  uint16 flags        = reader.read_uint16();
  uint16 num_qs       = reader.read_uint16();
  uint16 num_rrs      = reader.read_uint16();
  uint16 num_auth_rrs = reader.read_uint16();
  uint16 num_addl_rrs = reader.read_uint16();

  uint16 total        = num_qs + num_rrs + num_auth_rrs + num_addl_rrs;
  printf("MDNS total: %d\n", total);

  for (uint16 i = 0; i < total; ++i) {
    bool is_question = i < num_qs;
    //mk_buffer_range(reader).dump("mdns parsing header");
    mdns_header_t header(reader, is_question);

    printf("MDNS header type: 0x%04x\n", header.type);

    if (header.type == mdns::ptr) {
      if (!is_question) {
        buffer_reader_t reader2 = header.data_reader();
        //mk_buffer_range(reader2).dump("mdns header data-reader");
        string value = mdns_parsed_packet_t::read_stoopid_mdns_string(reader2);
        printf("%s == %s\n", header.record_name.c_str(), value.c_str());
      } else {
        printf("%s\n", header.record_name.c_str());
      }
    }
  }
}

string net_mobilewebprint::mdns_parsed_packet_t::read_stoopid_mdns_string(buffer_reader_t & reader)
{
  string result;

  while (!reader.at_end()) {
    byte first = reader.read_byte();
    //printf("0x%02x ", first);

    if (first == 0) {
      return result;
    }

    /* otherwise */
    if ((first & 0xc0) == 0xc0) {
      uint16 offset = (first & ~0xc0) << 8;
      offset += reader.read_byte();
      //printf("offset: 0x%04x \n", offset);

      buffer_reader_t reader2(reader);
      reader2.seek_to(offset);
//      mk_buffer_range(reader2).dump("mdns parsing a c0 substring");

      return _accumulate(result, read_stoopid_mdns_string(reader2), ".");
    } else {
      byte length = first;
      _accumulate(result, reader.read_string_nz(length), ".");
    }
  }

  return result;
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- mdns_header_t -----------------------------------------------
//---------------------------------------------------------------------------------------------------
net_mobilewebprint::mdns_header_t::mdns_header_t(buffer_reader_t & reader, bool is_question)
  : type(0), flags(0), ttl(0), data_length(0), data(reader)
{
  record_name = mdns_parsed_packet_t::read_stoopid_mdns_string(reader);

  type          = reader.read_uint16();
  flags         = reader.read_uint16();

  if (is_question) {
    return;
  }

  ttl           = reader.read_uint32();
  data_length   = reader.read_uint16();

  data          = reader;

  reader.seek(data_length);
}

buffer_reader_t net_mobilewebprint::mdns_header_t::data_reader() const
{
  return data;
}

mdns_record_base_t::mdns_record_base_t(mdns_header_t const & header)
  : record_name(header.record_name), type(header.type), flags(header.flags), ttl(header.ttl)
{
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- mdns_a_record_t----------------------------------------------
//---------------------------------------------------------------------------------------------------
mdns_a_record_t::mdns_a_record_t(mdns_header_t const & header)
  : mdns_record_base_t(header)
{
  ip = header.data_reader().read_ip();
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- mdns_srv_record_t -------------------------------------------
//---------------------------------------------------------------------------------------------------
mdns_srv_record_t::mdns_srv_record_t(mdns_header_t const & header)
  : mdns_record_base_t(header)
{
  buffer_reader_t reader = header.data_reader();

  priority = reader.read_uint16();
  weight   = reader.read_uint16();
  port     = reader.read_uint16();

  target = mdns_parsed_packet_t::read_stoopid_mdns_string(reader);
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- mdns_txt_record_t -------------------------------------------
//---------------------------------------------------------------------------------------------------
mdns_txt_record_t::mdns_txt_record_t(mdns_header_t const & header)
  : mdns_record_base_t(header)
{
  buffer_reader_t reader = header.data_reader();
  byte const *     start = reader.p;
  byte const *       end = start + header.data_length;

  string kv;
  while (reader.p < end) {

    size_t length = reader.read_byte();
    kv = reader.read_string_nz(length);
    //printf("kv: %s\n", kv.c_str());
    _add_kv(dict, kv, "=");
  }
}

