
#include "mwp_types.hpp"
#include "mwp_mdns.hpp"
#include "mwp_controller.hpp"
#include "mwp_utils.hpp"

#include <cstdio>
#include <cstdlib>

using namespace net_mobilewebprint::msg;
using namespace net_mobilewebprint::mq_enum;

using net_mobilewebprint::byte;
using net_mobilewebprint::network_node_t;
using net_mobilewebprint::buffer_view_i;
using net_mobilewebprint::buffer_t;
using net_mobilewebprint::split_on_parens;
using net_mobilewebprint::parse_kv;

#define HP_MDNS_MCAST_ADDR "224.0.0.251"
#define HP_MDNS_PORT       5353

static char pdl[] = "_pdl-datastream";
static char bjnp_srv_name[] = "_canon-bjnp1";
static char tcp[] = "_tcp";
static char local[] = "local";

static std::string read_stoopid_mdns_string(buffer_view_i const & payload, buffer_view_i::const_iterator & p);

/**
 *  mdns: mdns_t ctor.
 */
net_mobilewebprint::mdns_t::mdns_t(controller_base_t & controller_)
 : controller(controller_), mq(net_mobilewebprint::get_mq(controller_)), group_mcast(NULL),
   resend_query(0, /*start_at=*/2500, /*backoff_factor=*/1.2, /*max_interval=*/20000, /*restart_interval_at=*/10  * 60 * 1000),
   timer_holder(NULL), resend_query_timeout(500), transaction_id(0x100),
   ignore_start_time(0)
{
  _init_query_requests();
  _init_group_mcast();

  mq.on(this);
  mq.on_selected(this);
}

std::string net_mobilewebprint::mdns_t::mod_name()
{
  return "mdns_t";
}

int net_mobilewebprint::mdns_t::pre_select(mq_pre_select_data_t * pre_select_data)
{
  //printf("mdns pre_select\n");
  if (group_mcast == NULL) { return 0; }

  if (requests.size() > 0) {
    mq.check_udp_write(*group_mcast);
  }

  return 0;
}

e_handle_result net_mobilewebprint::mdns_t::on_select_loop_start(mq_select_loop_start_data_t const & loop_start_data)
{
  if (resend_query.has_elapsed(loop_start_data.current_loop_start)) {
    //log_d(1, "", "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ MDNS timeout after %d", resend_query.interval);
    if (group_mcast) {
      _queue_up_main_requests();
    }
  }

  if (mq.mq_normal.size() > 40) {
    if (ignore_start_time == 0 || _time_since(ignore_start_time) >= 2000) {
      // Show the warning only once
      log_d(1, "", "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ MDNS Ignoring for a while ^^^^^^^^^^^^");
    }
    ignore_start_time = get_tick_count();
  }

  return handled;
}

e_handle_result net_mobilewebprint::mdns_t::handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  //printf("mdns handling: %s\n", name.c_str());
  if (name == scan_for_printers)        { return _scan_for_printers(name, payload, data, extra); }
  if (name == "_on_mdns_packet")        { return _on_mdns_packet(name, payload, data, extra); }
  if (name == "re_scan_for_printers")   { _queue_up_main_requests(); return handled; }

  return not_impl;
}

e_handle_result net_mobilewebprint::mdns_t::_scan_for_printers(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  if (!mwp_assert(group_mcast)) { return unhandled; }
  mq.register_udp_for_select(*group_mcast);

  return handled;
}

void dump_mdns_packet(buffer_t const * packet)
{
  if (packet == NULL) { return; }

  buffer_view_i::const_iterator p = packet->first();
  uint16 id = packet->read_uint16(p);
  uint16 flags = packet->read_uint16(p);
  uint16 num_q = packet->read_uint16(p);
  uint16 num_a = packet->read_uint16(p);
  uint16 num_aa = packet->read_uint16(p);
  uint16 num_adnla = packet->read_uint16(p);
  std::string str = read_stoopid_mdns_string(*packet, p);
  uint16 type = packet->read_uint16(p);

  net_mobilewebprint::log_v("Id: 0x%04x, Flags: 0x%04x (%d,%d,%d,%d) -- %s (%d)\n", id, flags, num_q, num_a, num_aa, num_adnla, str.c_str(), type);
}

e_handle_result net_mobilewebprint::mdns_t::_mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra)
{
  if (!mwp_assert(group_mcast)) { return unhandled; }

  buffer_t * packet = NULL;
  e_handle_result result = unhandled;

  if (requests.size() > 0) {
    if (extra.is_udp_writable(*group_mcast)) {
      if ((packet = pull(requests)) != NULL) {
        dump_mdns_packet(packet);
        //log_d(1, "", "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ sending MDNS packet");
        group_mcast->send_udp_to(*packet);
        if (packet != &query_request && packet != &query_request_bjnp) {
          delete packet; /**/ num_buffer_allocations -= 1;
        }
        result = handled;
      }
    }
  }

  int recv_result = 0;
  if (extra.is_udp_readable(*group_mcast)) {
    network_node_t    sender;

    packet = mq.message("_on_mdns_packet", 0, extra.txn_id);
    size_t ip_offset = packet->data_length;
    packet->append((uint32)0);
    packet->append((uint16)0);

    if ((recv_result = group_mcast->recv_udp_from(*packet, sender, 2 * 1024)) > 0) {
      result = handled;
      log_v("received from: %s:%d", sender.ip.c_str(), sender.port);

      controller.packet_stat_incr(sender.ip, "mdns_recv_count", 1);
      controller.packet_stat_incr(sender.ip, "mdns_recv_bytes", recv_result);

      uint32 sender_ip = 0;
      inet_pton(AF_INET, sender.ip.c_str(), &sender_ip);
      sender_ip = ntohl(sender_ip);                           // Must convert back to host byte-order
      packet->set((int)ip_offset, sender_ip);
      packet->set((int)(ip_offset + sizeof(sender_ip)), sender.port);
      mq.send(packet);
    }
  }

  return result;
}

static std::string read_stoopid_mdns_string(buffer_view_i const & payload, buffer_view_i::const_iterator & p)
{
  std::string result;

  while (p < payload.const_end()) {
    byte first = payload.read_byte(p);

    if (first == 0) {
      break;
    } else if (first == 0xc0) {
      byte offset = payload.read_byte(p);
      buffer_view_i::const_iterator p2 = payload.first() + offset;
      return result + read_stoopid_mdns_string(payload, p2);
    }

    /* otherwise */
    byte len = first;
    result += payload.read_string_nz(p, len) + ".";
  }

  return net_mobilewebprint::rtrim(result, '.');
}

static bool read_record_header(
            buffer_view_i const & payload, buffer_view_i::const_iterator & p, int num_qs,
            std::string & rname, uint16 & type, uint16 & flags,
            uint32 & ttl, uint16 & dlen, int rec_num = 1000000)
{
  rname = read_stoopid_mdns_string(payload, p);
  type = payload.read_uint16(p);
  flags = payload.read_uint16(p);

  // Is this a question?
  if ((flags)& 0x8000 || rec_num < num_qs) {
    return false;
  }

  /* otherwise */
  ttl = payload.read_uint32(p);
  dlen = payload.read_uint16(p);

  return true;
}

bool net_mobilewebprint::mdns_t::should_send()
{
  if (ignore_start_time != 0) {
    if (_time_since(ignore_start_time) < 2000) {
      //log_d(1, "", "^^^^^^^^^^^^^^^^^^^^^^^^^^^^ Do not send MDNS packet");
      return false;
    }
  }

  return true;
}

/**
 *  Parse the MDNS packet.
 *
 *  We loop over the records three times, so that earlier loops can make their data
 *  available for subsequent loops.  HP printers (and others, but not Canon) put all
 *  the pertinent records into the packet, so you don't have to query for the various
 *  record types separately.  But, of course, the code must be able to make those
 *  independent requests, if necessary.
 *
 *  The first loop looks for the A recoreds (for the IP address); the second looks for
 *  the SRV records, and the third reads everything else (PTR).
 */
e_handle_result net_mobilewebprint::mdns_t::_on_mdns_packet(string const & name, buffer_view_i const & message_payload, buffer_t * data, mq_handler_extra_t & extra)
{
  uint8 const *rec_start = NULL;
  uint16 num_qs = 0, num_rrs = 0, num_auth_rrs = 0, num_additional_rrs = 0, total_records = 0;
  uint16 type = 0, flags = 0, dlen = 0, priority = 0, weight = 0, source_port = 0;
  uint32 ttl = 0;
  char buffer[32];
  string rname, source_ip;

  mdns_A_record_t * a_record = NULL;
  mdns_SRV_record_t * srv_record = NULL;

  buffer_view_i::const_iterator next_rr;
  deque<pair<uint16, uint8 const *> > record_list;
  deque<pair<uint16, uint8 const *> >::const_iterator rl_it;

  // If the queue gets too big, we will ignore the packet.
  if (mq.mq_normal.size() > 60) {
    //log_d(1, "", "^^^^^^^^^^^^^^^^^^^^^^^^^^^^ Ignoring MDNS packet");
    return handled;
  }


  buffer_view_i::const_iterator p = message_payload.first();

  source_ip = message_payload.read_ip(p);
  source_port = message_payload.read_uint16(p);

  // The MDNS payload starts here
  buffer_range_view_t payload(p, message_payload.const_end());

  //  payload.dump();

  payload.read_uint16(p);                             // Identifier

  if ((flags = payload.read_uint16(p)) & 0x8000) {
    total_records = 0;

    total_records += num_qs = payload.read_uint16(p);                           // Number of questions
    total_records += num_rrs = payload.read_uint16(p);
    total_records += num_auth_rrs = payload.read_uint16(p);
    total_records += num_additional_rrs = payload.read_uint16(p);

    string name, ip, target;
    uint16 port = 0;

    // First, find the A records and put them into the DB
    for (int i = 0; i < total_records; ++i) {
      rec_start = p;

      if (!read_record_header(payload, p, num_qs, rname, type, flags, ttl, dlen, i)) {
        // This is a question
        record_list.push_back(make_pair(type | 0x8000, rec_start));
        continue;
      }

      /* otherwise */
      record_list.push_back(make_pair(type, rec_start));
      next_rr = p + dlen;

      if (type == 0x01 /*A*/) {
        if (dlen >= 4) {
          inet_ntop(AF_INET, (void*)p, buffer, sizeof(buffer));
          ip = buffer;

          a_records[rname] = a_record = new mdns_A_record_t(ttl, ip, rname, extra.time);

          // We just got an A record... see if we already have the SRV record
          if ((srv_record = _lookup_srv(rname)) != NULL && srv_record->ip.length() == 0) {
            srv_record->ip = a_record->ip;
            _report_discovery(srv_record);
          }
        }
      }

      p = next_rr;
    }

    // Then, find SRV records
    for (rl_it = record_list.begin(); rl_it != record_list.end(); ++rl_it) {
      if (rl_it->first & 0x8000) { continue; }    // This is a question

      p = rl_it->second;
      read_record_header(payload, p, num_qs, rname, type, flags, ttl, dlen);

      if ((type = rl_it->first) == 0x21 /* SRV */) {
        priority = payload.read_uint16(p);
        weight = payload.read_uint16(p);
        port = payload.read_uint16(p);
        target = read_stoopid_mdns_string(payload, p);

        strvlist rname_parts = splitv(rname, '.');
        srv_records[target] = srv_record = new mdns_SRV_record_t(ttl, "", target, rname_parts[0], port, extra.time);

        // Get the associated A record
        if ((a_record = _lookup(target)) != NULL) {
          srv_record->ip = a_record->ip;
          _report_discovery(srv_record);
        } else {
          // We haven't seen the A record for this service... request it.
          strvlist name_parts = splitv(target, '.');
          //log_d(1, "", "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ queueing MDNS request for A record for %s", source_ip.c_str());
          push_request(_mk_request_packet(*(new buffer_t()), 0x01 /* A */, name_parts)); /**/ num_buffer_allocations += 1;
        }
      }
    }

    // Then loop over the other records
    for (rl_it = record_list.begin(); rl_it != record_list.end(); ++rl_it) {
      if (rl_it->first & 0x8000) { continue; }    // This is a question

      p = rl_it->second;
      read_record_header(payload, p, num_qs, rname, type, flags, ttl, dlen);

      if (type == 0x0c /* PTR */) {
        target = read_stoopid_mdns_string(payload, p);

        // If this is a PTR, and we don't have the SRV, yet, request it.
        if (_lookup_srv(target) == NULL) {
          strvlist name_parts = splitv(target, '.');
          //log_d(1, "", "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ queueing MDNS request for SRV record for %s", source_ip.c_str());
          push_request(_mk_request_packet(*(new buffer_t()), 0x21 /* SRV */, name_parts[0], name_parts[1])); /**/ num_buffer_allocations += 1;
        }
      }
    }
  }

  return handled;
}

void net_mobilewebprint::mdns_t::push_request(buffer_t * req)
{
  if (!should_send()) { return; }

  requests.push_back(req);
}

void net_mobilewebprint::mdns_t::_report_discovery(mdns_SRV_record_t * srv_record)
{
  log_v("Service!: %s:%d - %s\n", srv_record->ip.c_str(), srv_record->port, srv_record->name.c_str());

  strmap attrs;
  add_kv(attrs, "ip", srv_record->ip);
  add_kv(attrs, "port", srv_record->port);
  add_kv(attrs, "name", srv_record->name);

  controller.from_attrs(attrs);
  controller.sendTelemetry("printerScan", "mdnsRecord", "mdnsRecordName", srv_record->name, "ip", srv_record->ip, "port", srv_record->port);
}

net_mobilewebprint::mdns_t const & net_mobilewebprint::mdns_t::_init_group_mcast()
{
  if (group_mcast == NULL) {
    return _init_group_mcast(new network_node_t(HP_MDNS_MCAST_ADDR, HP_MDNS_PORT));
  }
  return *this;
}

net_mobilewebprint::mdns_t const & net_mobilewebprint::mdns_t::_init_group_mcast(network_node_t * node)
{
  if ((group_mcast = node) != NULL) {

//    group_mcast->try_mac = true;

    group_mcast->udp();
    group_mcast->set_sockopt(so_broadcast);

    // Join multicast group
    ip_mreq mreq;
    memset(&mreq, 0, sizeof(mreq));
    inet_pton(AF_INET, HP_MDNS_MCAST_ADDR, &mreq.imr_multiaddr);

    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    group_mcast->set_sockopt(ip_add_membership, &mreq, sizeof(mreq));

    group_mcast->set_sockopt(ip_multicast_ttl, (u_char)32);
    group_mcast->set_sockopt(ip_ttl, 32);
  }

  return *this;
}

net_mobilewebprint::buffer_t * net_mobilewebprint::mdns_t::_mk_request_packet(buffer_t & out, uint16 q_type, string const & service_name)
{
  return _mk_request_packet(out, q_type, service_name, "");
}

net_mobilewebprint::buffer_t * net_mobilewebprint::mdns_t::_mk_request_packet(buffer_t & out, uint16 q_type, string const & name_part_a_, string const & name_part_b_)
{
  strvlist name_parts;

  name_parts.push_back(name_part_a_);
  name_parts.push_back(name_part_b_);
  name_parts.push_back(tcp);
  name_parts.push_back(local);

  return _mk_request_packet(out, q_type, name_parts);
}

net_mobilewebprint::buffer_t * net_mobilewebprint::mdns_t::_mk_request_packet(buffer_t & out, uint16 q_type, strvlist const & name_parts)
{
  byte          by = 0;
  uint16        sh = 0;

  out.append(sh = transaction_id);      // Transaction ID
  out.append(sh = 0);                   // Flags

  out.append(sh = 1);                   // Number of questions
  out.append(sh = 0);                   // Number of answers (zero, this is a question)
  out.append(sh = 0);                   // Number of authority records (zero, this is a question)
  out.append(sh = 0);                   // Number of additional records (zero, this is a question)

#if 0
  char const *  name_part_a = name_part_a_.c_str();
  char const *  name_part_b = name_part_b_.c_str();

  if (name_part_a_.length() > 0) {
    out.append(by = strlen(name_part_a));         // Length of service_name
    out.append_str_sans_null(name_part_a);        // The service name
  }

  if (name_part_b_.length() > 0) {
    out.append(by = strlen(name_part_b));         // Length of service_name
    out.append_str_sans_null(name_part_b);        // The service name
  }

  out.append(by = strlen(tcp));         // Length of "_tcp"
  out.append_str_sans_null(tcp);        // The service "_tcp"
  out.append(by = strlen(local));       // Length of "local"
  out.append_str_sans_null(local);      // The service "local"
#else
  for (strvlist::const_iterator it = name_parts.begin(); it != name_parts.end(); ++it) {
    string const & str = *it;
    if (str.length() > 0) {
      char const * sz = (*it).c_str();
      out.append(by = strlen(sz));         // Length of service name part
      out.append_str_sans_null(sz);        // The service name part
    }
  }
#endif

  out.append(by = 0);                   // Terminating zero

  out.append(q_type);                   // The service type, like SRV=0x21
  out.append(sh = 0x8001);              // QU, IN

  transaction_id += 1;

  return &out;
}

net_mobilewebprint::mdns_t const & net_mobilewebprint::mdns_t::_init_query_requests()
{
  uint16 saved_transaction_id = transaction_id;

  transaction_id = 0x73;
  _mk_request_packet(query_request, 0x0c /*PTR*/, pdl);

  transaction_id = 0x74;
  _mk_request_packet(query_request_bjnp, 0x0c /*PTR*/, bjnp_srv_name);

  transaction_id = saved_transaction_id;

  return _queue_up_main_requests();
}

net_mobilewebprint::mdns_t const & net_mobilewebprint::mdns_t::_queue_up_main_requests()
{
  //log_d(1, "", "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ queueing MDNS broadcast");
  push_request(&query_request);
  //log_d(1, "", "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ queueing MDNS broadcast for BJNP");
  push_request(&query_request_bjnp);

  return *this;
}

net_mobilewebprint::mdns_A_record_t * net_mobilewebprint::mdns_t::_lookup(string const & name)
{
  map<string, mdns_A_record_t *>::const_iterator it;
  if ((it = a_records.find(name)) != a_records.end()) {
    mdns_A_record_t * result = it->second;
    return result;
  }

  return NULL;
}

net_mobilewebprint::mdns_SRV_record_t * net_mobilewebprint::mdns_t::_lookup_srv(string const & name)
{
  map<string, mdns_SRV_record_t *>::const_iterator it;
  if ((it = srv_records.find(name)) != srv_records.end()) {
    mdns_SRV_record_t * result = it->second;
    return result;
  }

  return NULL;
}

net_mobilewebprint::mdns_record_t::mdns_record_t(uint16 type_, uint32 ttl_)
  : type(type_), ttl(ttl_)
{
  _init(get_tick_count());
}

net_mobilewebprint::mdns_record_t::mdns_record_t(uint16 type_, uint32 ttl_, uint32 now)
  : type(type_), ttl(ttl_)
{
  _init(now);
}

void net_mobilewebprint::mdns_record_t::_init(uint32 now)
{
  expires_at = now + (ttl * 1000);
}

net_mobilewebprint::mdns_A_record_t::mdns_A_record_t(uint32 ttl, string const & ip_, string const & name_)
  : mdns_record_t(1, ttl), ip(ip_), name(name_)
{
}

net_mobilewebprint::mdns_A_record_t::mdns_A_record_t(uint32 ttl, string const & ip_, string const & name_, uint32 now)
  : mdns_record_t(1, ttl, now), ip(ip_), name(name_)
{
}

net_mobilewebprint::mdns_SRV_record_t::mdns_SRV_record_t(uint32 ttl, string const & ip_, string const & dns_name_, string const & name_, uint16 port_)
  : mdns_record_t(1, ttl), ip(ip_), name(name_), dns_name(dns_name_), port(port_)
{
}

net_mobilewebprint::mdns_SRV_record_t::mdns_SRV_record_t(uint32 ttl, string const & ip_, string const & dns_name_, string const & name_, uint16 port_, uint32 now)
  : mdns_record_t(1, ttl, now), ip(ip_), name(name_), dns_name(dns_name_), port(port_)
{
}
