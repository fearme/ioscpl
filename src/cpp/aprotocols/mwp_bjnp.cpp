
#include "mwp_types.hpp"
#include "mwp_bjnp.hpp"
#include "mwp_controller.hpp"

#include <cstdlib>

using namespace net_mobilewebprint::msg;
using namespace net_mobilewebprint::mq_enum;
using namespace net_mobilewebprint::tags;

using net_mobilewebprint::network_node_t;
using net_mobilewebprint::buffer_view_i;
using net_mobilewebprint::buffer_t;

#define TAG ":bjnp:"

// A typical packet - starts "BJNP"
net_mobilewebprint::byte packet_proto[] = { 0x42, 0x4a, 0x4e, 0x50,  0x01, 0x21, 0x00, 0x00,     0x00, 0x15, 0x65, 0x06,  0x00, 0x00, 0x00, 0x00 };
//                                             B     J     N     P    req  prnt    ??    ??      16-bit seq   16-bit id    32-bit payload length
/**
 *  bjnp_t ctor.
 */
net_mobilewebprint::bjnp_t::bjnp_t(controller_base_t & controller_)
  : controller(controller_), mq(controller_.mq)
{
  mq.on(this);
  mq.on_selected(this);
}

std::string net_mobilewebprint::bjnp_t::mod_name()
{
  return "bjnp_t";
}

net_mobilewebprint::bjnp_connection_t * net_mobilewebprint::bjnp_t::discover(uint32 & connection_id, string const & ip, uint16 port)
{
  bjnp_connection_t * connection = _connection(connection_id, ip, port);
  if (connection != NULL) {
    connection->discover();
  }

  return connection;
}

net_mobilewebprint::bjnp_connection_t * net_mobilewebprint::bjnp_t::send_to_printer(uint32   connection_id, string const & ip, uint16 port)
{
  uint32 connection_id_orig = connection_id;

  bjnp_connection_t * connection = _connection(connection_id, ip, port);
  if (connection != NULL) {
    connection->send_to_printer();
  }

  connection_ids.insert(make_pair(connection_id_orig, connection_id));
  http_connection_ids.insert(make_pair(connection_id, connection_id_orig));

  return connection;
}

bool net_mobilewebprint::bjnp_t::is_connection_alive(uint32 connection_id)
{
  connections_t::iterator it = connections.find(connection_id);
  if (it == connections.end()) { return false; }

  /* otherwise */
  bjnp_connection_t * connection = it->second;
  if (connection == NULL)       { return false; }

  /* otherwise */
  return !connection->is_done();
}

net_mobilewebprint::bjnp_connection_t * net_mobilewebprint::bjnp_t::_connection(uint32 & connection_id, string const & ip, uint16 port)
{
  connections_t::iterator it = connections.find(connection_id);

  if (it != connections.end()) {
    return it->second;
  }

  /* otherwise -- the connection_id might be wrong */
  bjnp_connection_t * connection = NULL;
  for (it = connections.begin(); it != connections.end(); ++it) {
    uint32 con_id = it->first;
    connection = it->second;

    if (connection != NULL && connection->udp_printer->ip == ip) {
      // We already had a connection_id for this IP, use that, instead
      connection_id = con_id;
      return connection;
    }
  }

  /* otherwise */
  network_node_t * printer = new network_node_t(ip.c_str(), port);
  network_node_t * udp_printer = new network_node_t(ip.c_str(), port);

  if (mwp_assert(printer) && mwp_assert(udp_printer)) {
    connection = new bjnp_connection_t(controller, printer, udp_printer, connection_id);
    connections[connection_id] = connection;
    return connection;
  }

  /* otherwise */
  return NULL;
}

int net_mobilewebprint::bjnp_t::pre_select(mq_pre_select_data_t * pre_select_data)
{
  bjnp_connection_t * connection = NULL;
  connections_t::iterator it;
  for (it = connections.begin(); it != connections.end(); ++it) {
    if ((connection = it->second) != NULL) {
      connection->pre_select(pre_select_data);
    }
  }

  return 0;
}

e_handle_result net_mobilewebprint::bjnp_t::_mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra)
{
  e_handle_result result = handled;

  connections_t::iterator it;
  bjnp_connection_t * connection = NULL;
  for (it = connections.begin(); it != connections.end(); ++it) {
    if ((connection = it->second) != NULL) {
      connection->_mq_selected(name, payload, data, extra);
      if (connection_is_closed(it)) {
        return result;
      }
    }
  }

  return result;
}

e_handle_result net_mobilewebprint::bjnp_t::handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  int txn_id = extra.txn_id;
  //int id = extra.id;

  e_handle_result result = unhandled;
  connections_t::iterator it;
  bjnp_connection_t * connection = NULL;

  if (name == "_on_http_payload" || name == "_on_bjnp_udp_packet" || name == "_on_txn_close") {

    it = connections.find(connection_id_for(txn_id));
    if (it != connections.end()) {
      connection = it->second;
    }

    if (connection != NULL) {
      result = connection->handle(name, payload, data, extra);

      if (connection_is_closed(it)) {
        return result;
      }
    }
  }

//  /* otherwise */
//  if (name == mq_selected) {
//    result = handled;
//    for (it = connections.begin(); it != connections.end(); ++it) {
//      if ((connection = it->second) != NULL) {
//        connection->handle(name, payload, data, extra);
//        if (connection_is_closed(it)) {
//          return result;
//        }
//      }
//    }
//  }

  return result;
}

uint32 net_mobilewebprint::bjnp_t::connection_id_for(uint32 http_connection_id)
{
  if (connection_ids.find(http_connection_id) == connection_ids.end()) {
    return http_connection_id;
  }

  return connection_ids[http_connection_id];
}

uint32 net_mobilewebprint::bjnp_t::http_connection_id_for(bjnp_connection_t const * connection)
{
  if (http_connection_ids.find(connection->connection_id) == http_connection_ids.end()) {
    return connection->connection_id;
  }

  return http_connection_ids[connection->connection_id];
}

bool net_mobilewebprint::bjnp_t::connection_is_closed(connections_t::iterator & it)
{
  bjnp_connection_t const * connection = it->second;

  if (connection != NULL && connection->is_done()) {

    controller.job_stat(http_connection_id_for(connection), "byte_stream_done", true);

    // Dont take it out of the list, keep it there, but make it NULL
    connections[connection->connection_id] = NULL;

    // Delete the object
    delete connection;
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
//
//  Helpers
//
//----------------------------------------------------------------------------

// Payload types
#define PRINTER_DISCOVERY     0x01
#define PRINT_DETAILS         0x10
#define PRINT_CLOSE           0x11
#define GET_STATUS            0x20
#define PRINT_BITS            0x21
#define PRINTER_IDENTITY      0x30
#define GET_PAYLOAD_SIZE      0x51

// Offsets into the BJNP packet structure
#define COMMAND   5
#define SEQUENCE  8
#define ID       10
#define LENGTH   12
#define DATA     16

bool                  is_xml(buffer_view_i const & buffer);
bool                  is_bjnp_cmd(buffer_view_i const & buffer);
int                   find_xml(buffer_view_i const & buffer);
int                   find_xml_end(buffer_view_i const & buffer);
int                   find_bjnp_cmd(buffer_view_i const & buffer, int offset_ = 0);
std::string           get_xml_value(std::string const & xml, char const * ctag);
bool                  xml_match(buffer_view_i const & chunk, buffer_view_i const & response);
bool                  xml_is(buffer_view_i const & chunk, buffer_view_i const & response, char const * operation_type);

char const * name_for(uint16 type)
{
  switch ((net_mobilewebprint::bjnp_connection_t::e_payload_type)type) {
  case net_mobilewebprint::bjnp_connection_t::xml_payload:     return "xml";
  case net_mobilewebprint::bjnp_connection_t::cmd_payload:     return "cmd";
  case net_mobilewebprint::bjnp_connection_t::bits_payload:    return "bits";
  case net_mobilewebprint::bjnp_connection_t::job_status:      return "progress";
  }

  return "unk";
}

uint16 seq_num_of(net_mobilewebprint::bjnp_chunk_t * chunk) {
  if (chunk == NULL) { return 0; }
  return chunk->seq_num;
}

//----------------------------------------------------------------------------
//
//  The chunk type
//
//----------------------------------------------------------------------------

net_mobilewebprint::bjnp_chunk_t::bjnp_chunk_t(buffer_t * data_, uint16 seq_num_, uint16 type1_, uint16 type2_, bool is_udp_)
  : have_result(false), known_failed(false), seq_num(seq_num_), that_seq_num(0), resend_seq_num(0), data(data_), type1(type1_), type2(type2_), is_udp(is_udp_), time(0)
{
}

net_mobilewebprint::bjnp_chunk_t::bjnp_chunk_t(bjnp_chunk_t const & that)
  : have_result(false), known_failed(false), seq_num(that.seq_num), that_seq_num(that.that_seq_num), resend_seq_num(that.resend_seq_num), data(that.data), type1(that.type1), type2(that.type2), is_udp(that.is_udp), time(0)
{
}

net_mobilewebprint::bjnp_chunk_t::~bjnp_chunk_t()
{
  if (data != NULL) {
    delete data; /**/ num_buffer_allocations -= 1;
  }
  data = NULL;
}

uint32 net_mobilewebprint::bjnp_chunk_t::time_since()
{
  return get_tick_count() - time;
}



//----------------------------------------------------------------------------
//
//  The connection
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Free functions to allow bjnp_connection_t to have multiple handlers
//
//----------------------------------------------------------------------------
MWP_MQ_BASE_HANDLER_HELPER(bjnp_connection_t, bjnp_watchdog)

/**
 *  bjnp_connection_t ctor.
 */
net_mobilewebprint::bjnp_connection_t::bjnp_connection_t(controller_base_t & controller_, network_node_t * connection_, network_node_t * udp_connection_, uint32 id_)
  : controller(controller_), mq(controller_.mq), udp_printer(udp_connection_),
    tcp_is_open(false), printer(connection_), connection_id(id_), bjnp_job_num(73), bjnp_seq_num(1), bjnp_payload_size(64 * 1024), curr_seq_num(0),
    watchdog_holder(NULL), watchdog_timeout(500), watchdog_num(-1), discover_is_in_flight(false), send_job_is_in_flight(false), last_tcp_status(0),
    packet_num(-1), special_tcp_chunk(NULL), payload_type(bits_payload),
    closed(false), printer_ackd_end_job(false), txn_closed(false)
{
  srand(get_tick_count());
  bjnp_job_num = 2000 + (rand() % 5000);

  mq.check_udp_read(*udp_printer);

  log_v(4, TAG, "bjnp_connection udp_fd: 0x%04x tcp_fd: 0x%04x", udp_printer->udp_fd, printer->tcp_fd);
  mq.setTimeout(watchdog_holder, mod_name(), bjnp_watchdog, this, watchdog_timeout);
}

net_mobilewebprint::bjnp_connection_t::~bjnp_connection_t()
{
  if (printer != NULL) {
    delete printer;
    printer = NULL;
  }

  if (udp_printer != NULL) {
    delete udp_printer;
    udp_printer = NULL;
  }

  // Loop through the packets and release the resources
  for (std::map<uint16, bjnp_chunk_t *>::const_iterator it = packet_by_seq.begin(); it != packet_by_seq.end(); ++it) {
    bjnp_chunk_t * chunk = it->second;
    delete chunk;
  }
}

std::string net_mobilewebprint::bjnp_connection_t::mod_name()
{
  return "bjnp_connection_t";
}

bool net_mobilewebprint::bjnp_connection_t::is_done() const
{
  return closed && watchdog_timeout == -1;
}

e_handle_result net_mobilewebprint::bjnp_connection_t::_bjnp_watchdog(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  // If the timer is zero, stop running this timer
  if (watchdog_timeout == 0) {
    watchdog_timeout = -1;        // This is the signal that the timer has been stopped
    return handled;
  }

  buffer_t empty_buffer;

  watchdog_num += 1;
  mq.setTimeout(watchdog_holder, mod_name(), bjnp_watchdog, this, watchdog_timeout);

  // Watch to see if we are getting responses to our packets
  std::map<uint16, bjnp_chunk_t *>::const_iterator it;
  for (it = packet_by_seq.begin(); it != packet_by_seq.end(); ++it) {
    uint16            chunk_seq_num = it->first;
    bjnp_chunk_t *            chunk = it->second;

    // Dont need to check chunks that arent sent, yet
    if (chunk_seq_num > curr_seq_num) {
      break;
    }

    // Dont check chunks that have already been taken care of
    if (chunk->data == NULL) {
      continue;
    }

    /* otherwise */
    bool is_bad = false;

    if (chunk->is_udp) {
      if (!chunk->have_result && chunk->time_since() > 1000) {
        is_bad = true;
      }

      if (is_bad) {
        if (chunk->seq_num == curr_seq_num) {
          curr_seq_num = 0;
        }
        log_v(4, TAG, "bjnp-UDP-resend 0x%x", (uint32)chunk->seq_num);
        relabel_chunk(chunk = new bjnp_chunk_t(*chunk), 0x9999);
        udp_packets.push_front(chunk);
        continue;
      }
    } else {
      // TCP
    }

    if (watchdog_num % 10 == 0) {
      if (!chunk->have_result && chunk_seq_num <= curr_seq_num) {
        log_v(4, TAG, "Still in flight: 0x%04x 0x%04x", (uint32)chunk->data->uint16_at(8), (uint32)chunk_seq_num);
      }
    }
  }

  int since_last = get_tick_count() - last_tcp_status;
  if ((last_tcp_status > 0) && (since_last > 3000)) {
    if (special_tcp_chunk == NULL) {
      log_v(5, TAG, "bjnp sending progress STATUS after a delay of %d", since_last);
      special_tcp_chunk = tcp_chunk(GET_STATUS, buffer_t(), 0, 0, job_status, 0);
      last_tcp_status = get_tick_count();   // NOTE: would be better if this was the time that the packet was sent
    }
  }

  return handled;
}

void net_mobilewebprint::bjnp_connection_t::discover()
{
  if (discover_is_in_flight) { return; }
  log_v(4, TAG, "bjnp-discover for %s", printer->ip.c_str());

  log_v(4, TAG, "bjnp sending DISCOVERY packet");
  discover_is_in_flight = true;
  push_back_udp(new buffer_t(bjnp::discover_pkt(bjnp_job_num, bjnp_seq_num)), PRINTER_DISCOVERY, 0); /**/ num_buffer_allocations += 1;
}

void net_mobilewebprint::bjnp_connection_t::send_to_printer()
{
  if (send_job_is_in_flight) { return; }
  log_d(1, TAG, "bjnp-start-print for %s", printer->ip.c_str());

  log_v(4, TAG, "bjnp sending GET_PAYLOAD_SIZE packet");
  send_job_is_in_flight = true;
  push_back_udp(new buffer_t(bjnp::payload_size_pkt(bjnp_job_num, bjnp_seq_num)), GET_PAYLOAD_SIZE, 0); /**/ num_buffer_allocations += 1;
}

int net_mobilewebprint::bjnp_connection_t::pre_select(mq_pre_select_data_t * pre_select_data)
{
  if (closed) {
    return 0;
  }

  // UDP write
  bool checked_udp_write = false;
  if (curr_seq_num == 0 && udp_packets.size() > 0 && !printer_ackd_end_job) {
    checked_udp_write = true;
    mq.check_udp_write(*udp_printer);
  }

  // TCP write -- check the super-fast tests before looking at the next chunk
  if (curr_seq_num == 0 && !printer_ackd_end_job && tcp_is_open) {

    bjnp_chunk_t * next_chunk = next_tcp_chunk();
    if (next_chunk != NULL) {

      // If this is a re-send, dont send too soon
      if (next_chunk->resend_seq_num == 0) {
        mq.check_tcp_write(*printer);
      } else {
        std::map<uint16, bjnp_chunk_t *>::const_iterator it;
        if ((it = packet_by_seq.find(next_chunk->resend_seq_num)) != packet_by_seq.end()) {
          bjnp_chunk_t * chunk = it->second;

          // Wait for one second before re-sending
          uint32 time_since = chunk->time_since();
          if (time_since > 1000) {
            mq.check_tcp_write(*printer);
          }
        }
      }
    }
  }

  // TCP read
  if (tcp_is_open) {
    mq.check_tcp_read(*printer);
  }

  // Is this the "first open" event?
  if (last_tcp_status == 0 && printer->tcp_fd != 0) {
    last_tcp_status = get_tick_count();
  }

  return 0;
}

e_handle_result net_mobilewebprint::bjnp_connection_t::handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  if (name == "_on_bjnp_udp_packet")    { return _on_bjnp_udp_packet(name, payload, data, extra); }
  if (name == "_on_http_payload")       { return _on_http_payload(name, payload, data, extra); }
  if (name == "_on_txn_close")          { return _on_txn_close(name, payload, data, extra); }

  return not_impl;
}

e_handle_result net_mobilewebprint::bjnp_connection_t::_mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra)
{
  if (!mwp_assert(printer)) { return unhandled; }

  if (!mq.is_fd_part_of_select(*printer) && !mq.is_fd_part_of_select(*udp_printer)) {
    return unhandled;
  }

  e_handle_result result = unhandled;
  buffer_t *      packet = NULL;

  // First, look at the UDP fd
  if (curr_seq_num == 0 && udp_packets.size() > 0) {

    if (extra.is_udp_writable(*udp_printer)) {

      bjnp_chunk_t * packet = pull(udp_packets);

      packet->time = get_tick_count();
      relabel_chunk(packet, curr_seq_num = bjnp_seq_num++);

      packet->data->dump("bjnp", "sending UDP packet");

      udp_printer->send_udp_to(*packet->data, 'b');
    }
  }

  int recv_result = 0;
  if (extra.is_udp_readable(*udp_printer)) {

    packet = mq.message_with_ip("_on_bjnp_udp_packet", 0, connection_id);

    network_node_t    sender;
    if ((recv_result = udp_printer->recv_udp_from(*packet, sender, 4 * 1024)) > 0) {
      result = handled;

      mq.add_ip_to_message(packet, sender.ip, sender.port);
      mq.send(packet);
    }
  }

  if (tcp_is_open) {
    bjnp_chunk_t * chunk = NULL;

    if (curr_seq_num == 0 && next_tcp_chunk(/*remove_it=*/false) != NULL) {
      if (extra.is_tcp_writable(*printer)) {
        packet_num += 1;

        chunk = next_tcp_chunk(/*remove_it=*/true);
        chunk->time = get_tick_count();
        relabel_chunk(chunk, curr_seq_num = bjnp_seq_num++);

        if (bjnp::get_command_id(*chunk->data) == GET_STATUS) {
          log_v(4, TAG, "bjnp sending a STATUS request (special:%d)", (int)(special_tcp_chunk != NULL));
          last_tcp_status = chunk->time;
        }

        chunk->data->dump("bjnp", "sending to printer", 1024);

        int num_sent = 0;
        num_sent = printer->send_tcp(*chunk->data, 'b');
        if (num_sent != chunk->data->dsize()) {
          log_e("Partial send");
        }

        log_v(4, TAG, "bjnp_connection::wrote %s: 0x%08x / 0x%08x(0x%08x) -- seq:0x%x(0x%x), %d chunks remaining; %d in buildout", name_for(chunk->type1), num_sent, chunk->data->dsize(), chunk->data->mem_length, (int)chunk->seq_num, (int)seq_num_of(resend_of(chunk)), (int)chunks.size(), (int)buildout_buffer.dsize());

        // If this is not an extra request for status, log the number of bytes sent to the printer
        if (special_tcp_chunk == NULL) {
          controller.job_stat_incr(controller.bjnp.http_connection_id_for(this), "totalSent", num_sent);
        }

        if (special_tcp_chunk != NULL) {
          special_tcp_chunk = NULL;
        }

        result = handled;
      }
    }

    if (packet_num >= 0) {
      if (extra.is_tcp_readable(*printer)) {
        buffer_t tcp_read_packet;
        result = handled;

        if ((recv_result = printer->recv_tcp(tcp_read_packet, 16 * 1024)) > 0) {
          tcp_read_packet.dump("bjnp", "Recvd from printer");
          result_found(tcp_read_packet, false);
        } else if (recv_result == 0) {
          // Closed!
          tcp_is_open = false;
        }
      }
    }
  }

  if (txn_closed && chunks.size() == 0 && printer_ackd_end_job) {
    log_v("Closing printer node %d", connection_id);
    mq.deregister_for_select(*printer);
    printer->close();
    mq.deregister_for_select(*udp_printer);
    udp_printer->close();

    watchdog_timeout = 0;   // Tell the watchdog to stop

    closed = true;
  }

  return result;
}

e_handle_result net_mobilewebprint::bjnp_connection_t::_on_bjnp_udp_packet(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  string mac, ip;

  uint16 type         = 0;
  uint16 seq_num      = 0;
  uint16 job_num      = 0;
  uint32 payload_size = 0;
  uint32 num_recvd    = 0;

  strmap attrs;

  type = _parse_packet(payload, seq_num, job_num, payload_size, num_recvd);
  result_found(payload, true);
  payload.dump("bjnp", "Recvd packet");
  if (type == PRINTER_DISCOVERY) {
    log_v(4, TAG, "bjnp recvd DISCOVERY packet");
    log_v(4, TAG, "bjnp sending ID packet");
    push_back_udp(new buffer_t(bjnp::get_id_pkt(bjnp_job_num, bjnp_seq_num)), PRINTER_IDENTITY, 0); /**/ num_buffer_allocations += 1;

    if (bjnp::parse_discover_response(payload, mac, ip)) {
      add_kv(attrs, "ip", ip);
      add_kv(attrs, "mac", mac);
      controller.from_attrs(attrs);

      return handled;
    }
  } else if (type == PRINTER_IDENTITY) {
    log_v(4, TAG, "bjnp recvd ID packet");
    string device_str;
    if (bjnp::parse_identity_response(payload, device_str)) {
      split_kv(attrs, device_str, ';', ':');
      add_kv(attrs, "ip", udp_printer->ip);
      controller.from_1284_attrs(attrs);
      return handled;
    }
  } else if (type == GET_PAYLOAD_SIZE) {
    log_v(4, TAG, "bjnp recvd PAYLOAD_SIZE packet");
    log_v(4, TAG, "bjnp sending DETAILS (start_job) packet");
    push_back_udp(new buffer_t(bjnp::start_job_pkt("", "", "", bjnp_job_num, bjnp_seq_num)), PRINT_DETAILS, 0); /**/ num_buffer_allocations += 1;

    uint32 bjnp_payload_size_ = 0;
    if (bjnp::parse_payload_size_response(payload, bjnp_payload_size_)) {
      bjnp_payload_size = min(bjnp_payload_size_, 64 * 1024);     // At most 64k
      bjnp_payload_size = max(bjnp_payload_size, 4 * 1024);       // At least 4k
      return handled;
    }
  } else if (type == PRINT_DETAILS) {
    log_v(4, TAG, "bjnp recvd DETAILS (start_job) packet");
    uint32 bjnp_job_id_ = 0;
    uint16 bjnp_short_job_id = 0;
    if (bjnp::parse_print_details_response(payload, bjnp_short_job_id, bjnp_job_id_)) {
      tcp_is_open = true;
      bjnp_job_num = bjnp_short_job_id;
      mq.check_tcp_read(*printer);
      printer->set_sockopt(so_keepalive);
      return handled;
    }
  }

  return handled;
}

e_handle_result net_mobilewebprint::bjnp_connection_t::_on_http_payload(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  controller.job_stat_incr(controller.bjnp.http_connection_id_for(this), "numDownloaded", (int)payload.dsize());

  int input_size = (int)payload.dsize();

  buildout_buffer.append(payload);
  delete data;

  int num_xml = 0, num_cmd = 0, num_print_bits = 0, num_chunks = 0;

  while (buildout_buffer.dsize() > 0) {
    int end = 0;

    int orig_size     = (int)buildout_buffer.dsize();
    int num_good      = (int)buildout_buffer.dsize();
    int num_too_many  = 0;

    // Does the buffer start with XML?
    if (is_xml(buildout_buffer)) {

      // The buffer starts with XML... does it have all of the XML?
      if ((end = find_xml_end(buildout_buffer)) == -1) {
        // No.  This is just a partial XML chunk... exit and maybe the next chunk will have it
        return handled_and_took_message;
      }

      num_too_many    = (int)buildout_buffer.dsize() - end;
      num_good       -= num_too_many;
      payload_type    = xml_payload;
      num_xml        += 1;
    } else {
      if (is_bjnp_cmd(buildout_buffer)) {
        payload_type = cmd_payload;
        num_cmd += 1;
      } else {
        payload_type = bits_payload;
        num_print_bits += 1;
      }

      // The buffer does not start with XML.  See if we have to limit the size because we have XML or
      // a command in the middle of this chunk
      num_good      = min(num_good, find_xml(buildout_buffer));
      num_good      = min(num_good, find_bjnp_cmd(buildout_buffer, 1));

      num_too_many  = orig_size - num_good;
    }

    // Make a chunk for the write-queue
    num_good      = min(num_good, (int)bjnp_payload_size);
    num_too_many  = orig_size - num_good;

    if (num_too_many == 0 && payload_type != xml_payload) {
      // This would be the entire rest of the input, but we dont want to do that.
      break;
    }

    push_back_tcp(buildout_buffer, 0, num_good, payload_type, 0);
    num_chunks += 1;

    log_v(5, TAG, "building out a chunk. original:%d --> size:%d(0x%08x) / remaining:%d type:%s", buildout_buffer.dsize(), num_good, num_good, num_too_many, name_for(payload_type));

    // Do we have to put some back?
    if (num_good > 0) {
      buildout_buffer.lshift(NULL, num_good);
    }
  }

  log_v(5, TAG, "bjnp_on_http: %d recvd; %d xml; %d cmd.  Created %d chunks; %d remaining on buildout. Final type: %s", input_size, num_xml, num_cmd, num_chunks, (int)buildout_buffer.dsize(), name_for(payload_type));

  return handled_and_took_message;    // We now have ownership of the data memory
}

net_mobilewebprint::bjnp_connection_t & net_mobilewebprint::bjnp_connection_t::push_back_udp(buffer_t * packet, uint16 type1, uint16 type2)
{
  uint16 seq_num = bjnp::get_seq_num(*packet);

  bjnp_chunk_t * chunk = new bjnp_chunk_t(packet, seq_num, 0, 0, true);
  udp_packets.push_back(chunk);

  return *this;
}

// packet_proto[] = { 0x42, 0x4a, 0x4e, 0x50,  0x01, 0x21, 0x00, 0x00,   0x00, 0x15, 0x65, 0x06,  0x00, 0x00, 0x00, 0x00 };
net_mobilewebprint::bjnp_connection_t & net_mobilewebprint::bjnp_connection_t::push_back_tcp(buffer_view_i const & data, int begin_offset, int end_offset, uint16 type1, uint16 type2)
{
  chunks.push_back(tcp_chunk(PRINT_BITS, data, begin_offset, end_offset, type1, type2));
  return *this;
}

net_mobilewebprint::bjnp_chunk_t * net_mobilewebprint::bjnp_connection_t::tcp_chunk(byte command, buffer_view_i const & data, int begin_offset, int end_offset, uint16 type1, uint16 type2)
{
  uint16 seq_num = 0x9999;

  int num_from_data = end_offset - begin_offset;
  buffer_t * packet = new buffer_t(packet_proto, sizeof(packet_proto), num_from_data); /**/ num_buffer_allocations += 1;

  packet->append(data.const_begin() + begin_offset, num_from_data);

  packet->set(COMMAND, command);
  packet->set(SEQUENCE, seq_num);
  packet->set(ID, bjnp_job_num);
  packet->set(LENGTH, (uint32)num_from_data);

  return new bjnp_chunk_t(packet, seq_num, type1, type2, false);
}

net_mobilewebprint::bjnp_connection_t & net_mobilewebprint::bjnp_connection_t::result_found(buffer_view_i const & result_payload, bool is_udp)
{
  uint32 result_payload_size = 0, request_payload_size = 0, num_recvd = 0, _dont_care = 0;
  uint16 seq_num = 0, job_num = 0, type = _parse_packet(result_payload, seq_num, job_num, result_payload_size, num_recvd);

  std::map<uint16, bjnp_chunk_t *>::iterator it;
  if ((it = packet_by_seq.find(seq_num)) != packet_by_seq.end()) {

    bjnp_chunk_t * that_chunk = NULL, *chunk = it->second;

    // Have we even sent this packet?
    if (chunk->time == 0) {
      return *this;
    }

    // Is there an associated chunk?
    if (chunk->that_seq_num != 0 && (it = packet_by_seq.find(chunk->that_seq_num)) != packet_by_seq.end()) {
      that_chunk = it->second;
    }

    chunk->have_result = true;

    bool is_good = true;
    if (is_udp) {
    } else {
      type = _parse_packet(*chunk->data, seq_num, job_num, request_payload_size, _dont_care);

      if (type == PRINT_BITS) {
        if (chunk->type1 == cmd_payload || chunk->type1 == bits_payload) {
          is_good = (num_recvd == request_payload_size);
          if (chunk->type1 == cmd_payload) {
            log_v(4, TAG, "bjnp recvd CMD response, good:%d", is_good);
          } else {
            log_v(4, TAG, "bjnp recvd BITS response, good:%d", is_good);
          }
        } else if (chunk->type1 == xml_payload) {
          // We have to wait for the printer to send status that this
          // XML command worked
          log_v(4, TAG, "bjnp recvd XML response");
          log_v(4, TAG, "bjnp sending XML-STATUS request");
          special_tcp_chunk = tcp_chunk(GET_STATUS, buffer_t(), 0, 0, chunk->type1, chunk->type2);
          special_tcp_chunk->that_seq_num = seq_num;
        }
      } else if (type == GET_STATUS) {
        string operation, status, response, num_bytes, jobid;
        string xml = bjnp::payload_of(result_payload).to_lower();
        operation = get_xml_value(xml, "operation");
        status = get_xml_value(xml, "status");
        response = get_xml_value(xml, "response");
        //string name = get_xml_value(xml, "name");
        //string readdata = get_xml_value(xml, "readdata");
        num_bytes = get_xml_value(xml, "numberofbytes");
        jobid = get_xml_value(xml, "jobid");
        string jobprogress = get_xml_value(xml, "jobprogress");

        // Other things that could be gotten from the XML:
        // "response_detail", "status_detail"

        // TODO: If this is general status, push the status to the printer list

        string bjnp_operation = "UNK";
        if (chunk->type1 == xml_payload) {
          bjnp_operation = "XML-STATUS";

          // Check that the XML is for the expected operation
          is_good = false;

          // Does the result XML type match the XML type we sent?
          if (xml_match(*that_chunk->data, result_payload)) {
            is_good = true;

            if (xml_is(*that_chunk->data, result_payload, "endjob")) {
              // We are done -- close things, etc.
              log_v(2, TAG, "-------- bjnp DONE ----------");
              printer_ackd_end_job = true;
            }
          }
          log_v(4, TAG, "bjnp recvd XML-STATUS response, good:%d", is_good);
        } else if (chunk->type1 == job_status) {
          bjnp_operation = "JOB-STATUS";
        }

        log_v(5, TAG, "bjnp recvd %s request, operation: %s - %s - %s - %s (%s) (%s)", bjnp_operation.c_str(), operation.c_str(), response.c_str(), jobprogress.c_str(), status.c_str(), num_bytes.c_str(), jobid.c_str());
        log_v(4, TAG, "XML:%s", xml.c_str());
      }

      if ((chunk->known_failed = !is_good) == true) {
        log_v(4, TAG, "bjnp re-queueing");
        relabel_chunk(chunk = new bjnp_chunk_t(*chunk), 0x9999);
        chunks.push_front(chunk);

        if (chunk && chunk->data) {
          controller.job_stat_incr(controller.bjnp.http_connection_id_for(this), "totalSent", -(chunk->data->dsize()));
        }
      }
    }

    curr_seq_num = 0;
  }

  return *this;
}

e_handle_result net_mobilewebprint::bjnp_connection_t::_on_txn_close(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  txn_closed = true;

  // That's all the input were going to get... put it all on the chunk list
  if (buildout_buffer.dsize() > 0) {
    push_back_tcp(buildout_buffer, 0, (int)buildout_buffer.dsize(), payload_type, 0);
    buildout_buffer.clear();
  }

  log_v(2, TAG, "############################# At close time, we had %d chunks remaining", (int)chunks.size());

  if (chunks.size() == 0) {
    mq.deregister_for_select(*printer);
    printer->close();
  }

  return handled;
}

net_mobilewebprint::bjnp_connection_t & net_mobilewebprint::bjnp_connection_t::relabel_chunk(bjnp_chunk_t * chunk, uint16 seq_num)
{
  uint16 orig_chunk_seq_num = chunk->seq_num;

  chunk->seq_num = seq_num;
  chunk->data->set(SEQUENCE, seq_num);

  if (seq_num != 0x9999) {
    packet_by_seq[seq_num] = chunk;
  }

  if (orig_chunk_seq_num != 0x9999) {
    if (packet_by_seq.find(orig_chunk_seq_num) != packet_by_seq.end()) {
      packet_by_seq.find(orig_chunk_seq_num)->second->data = NULL;
      chunk->resend_seq_num = orig_chunk_seq_num;
    }
  }

  return *this;
}

net_mobilewebprint::bjnp_chunk_t * net_mobilewebprint::bjnp_connection_t::next_tcp_chunk(bool remove)
{
  // What's the next TCP chunk to send?
  bjnp_chunk_t * result = special_tcp_chunk;
  if (result != NULL) { return result; }

  /* otherwise */
  if (chunks.size() > 0) {
    result = chunks.front();
    if (remove) {
      chunks.pop_front();
    }
  }

  return result;
}

net_mobilewebprint::bjnp_chunk_t * net_mobilewebprint::bjnp_connection_t::resend_of(bjnp_chunk_t * chunk)
{
  bjnp_chunk_t * result = chunk;

  std::map<uint16, bjnp_chunk_t *>::const_iterator it;
  while (result->resend_seq_num != 0) {
    if ((it = packet_by_seq.find(result->resend_seq_num)) != packet_by_seq.end()) {
      result = it->second;
    }
  }

  if (result != chunk) {
    return result;
  }

  return NULL;
}

int net_mobilewebprint::bjnp_connection_t::_parse_packet(buffer_view_i const & packet, uint16 & seq_num, uint16 & job_num, uint32 & payload_size, uint32 & num_recvd)
{
  buffer_view_i::const_iterator p = packet.first() + 4;

  /* req_res= */  packet.read_byte(p);      // Is this a request or a response
  int type      = packet.read_byte(p);

  /* unknown= */  packet.read_uint16(p);

  seq_num       = packet.read_uint16(p);
  job_num       = packet.read_uint16(p);
  payload_size  = packet.read_uint32(p);

  if (payload_size >= 4) {
    num_recvd = packet.read_uint32(p);
  }

  return type;
}



bool net_mobilewebprint::bjnp::parse_discover_response(buffer_view_i const & packet, string & mac_out, string & ip_out)
{
  char buffer[64];
  int i = 0;

  buffer_view_i::const_iterator p = packet.first() + DATA + 4;

  int mac_length = packet.read_byte(p);
  int ip_length  = packet.read_byte(p);

  for (i = 0; i < mac_length; ++i) {
    mwp_sprintf(buffer, "%02x:", packet.read_byte(p));
    mac_out += buffer;
  }
  mac_out = rtrim(mac_out, ':');

  for (i = 0; i < ip_length; ++i) {
    mwp_sprintf(buffer, "%d.", packet.read_byte(p));
    ip_out += buffer;
  }
  ip_out = rtrim(ip_out, '.');

  return true;
}

bool net_mobilewebprint::bjnp::parse_identity_response(buffer_view_i const & packet, string & device_str_out)
{
  buffer_view_i::const_iterator p = packet.first() + DATA;
  uint16 length = packet.read_uint16(p);

  printf("bjnp:parse_id_resp %d %s\n", length, device_str_out.c_str());
  device_str_out = packet.read_string_nz(p, length - sizeof(uint16));
  printf("bjnp:parse_id_resp %d %s\n", length, device_str_out.c_str());

  return true;
}

bool net_mobilewebprint::bjnp::parse_print_details_response(buffer_view_i const & packet, uint16 & bjnp_short_job_id, uint32 & bjnp_job_num)
{
  buffer_view_i::const_iterator p = packet.first() + DATA;
  bjnp_job_num = packet.read_uint32(p);

  bjnp_short_job_id = packet.uint16_at(ID);

  return true;
}

bool net_mobilewebprint::bjnp::parse_payload_size_response(buffer_view_i const & packet, uint32 & payload_size)
{
  buffer_view_i::const_iterator p = packet.first() + DATA;
  payload_size = packet.read_uint32(p);

  return true;
}

/**
*  Build a packet.
*
*  Sets the proper bytes within the packet for the 'command';
*  Allocates 'extra_byte_count' bytes, to be filled in by the caller.  But this function
*  sets the packet-size field, so the caller does not need to.
*/
buffer_t net_mobilewebprint::bjnp::packet(byte command, uint16 & job_num, uint16 & seq_num, uint32 extra_byte_count)
{
  buffer_t result(packet_proto, sizeof(packet_proto), extra_byte_count, 0);

  result.set(COMMAND, command);
  result.set(SEQUENCE, (uint16)0x9999);
  result.set(ID, job_num);
  result.set(LENGTH, extra_byte_count);

  return result;
}

// For the StartJob packet, here are the pertinent offsets
#define PRINT_DETAILS_HOST    8
#define PRINT_DETAILS_USER    (PRINT_DETAILS_HOST + 64)
#define PRINT_DETAILS_NAME    (PRINT_DETAILS_USER + 64)

/**
*  Build a StartJob packet
*/
buffer_t net_mobilewebprint::bjnp::start_job_pkt(string host, string user, string name, uint16 & job_num, uint16 & seq_num)
{
  buffer_t result = packet(PRINT_DETAILS, job_num, seq_num, 8 + 64 + 64 + 256);
  result.data_length = result.mem_length;

  *(result.bytes + DATA + PRINT_DETAILS_HOST + 1) = 'h';
  *(result.bytes + DATA + PRINT_DETAILS_USER + 1) = 'u';
  *(result.bytes + DATA + PRINT_DETAILS_NAME + 1) = 'j';

  return result;
}

/**
*  Build a Discover packet.
*/
buffer_t net_mobilewebprint::bjnp::discover_pkt(uint16 & job_num, uint16 & seq_num)
{
  return packet(PRINTER_DISCOVERY, job_num, seq_num);
}

/**
*  Build a Get1284Id packet.
*/
buffer_t net_mobilewebprint::bjnp::get_id_pkt(uint16 & job_num, uint16 & seq_num)
{
  buffer_t result = packet(PRINTER_IDENTITY, job_num, seq_num, 4);
  result.set(DATA, (uint32)0);
  return result;
}

/**
*  Build a GetStatus packet.
*/
buffer_t net_mobilewebprint::bjnp::status_pkt(uint16 & job_num, uint16 & seq_num)
{
  return packet(GET_STATUS, job_num, seq_num);
}

/**
*  Build a "whats the right size to send print-bits payloads?" packet.
*/
buffer_t net_mobilewebprint::bjnp::payload_size_pkt(uint16 & job_num, uint16 & seq_num)
{
  return packet(GET_PAYLOAD_SIZE, job_num, seq_num);
}

uint16 net_mobilewebprint::bjnp::get_seq_num(buffer_view_i const & packet)
{
  return packet.uint16_at(SEQUENCE);
}

net_mobilewebprint::byte net_mobilewebprint::bjnp::get_command_id(buffer_view_i const & packet)
{
  return packet.at(COMMAND);
}

/**
*  Get the payload from a packet.
*/
net_mobilewebprint::buffer_range_view_t net_mobilewebprint::bjnp::payload_of(buffer_view_i const & that)
{
  return buffer_range_view_t(that.const_begin() + DATA, that.const_end());
}

/**
*  Is the buffer pointed to by 'p' XML?
*/
bool is_xml(net_mobilewebprint::byte const * p, size_t len) {
  if (len < 6) { return false; }
  return (*p++ == '<' && *p++ == '?' && *p++ == 'x' && *p++ == 'm' && *p++ == 'l');
}

/**
*  Is the buffer_t XML?
*/
bool is_xml(buffer_view_i const & buffer) {
  return is_xml(buffer.const_begin(), buffer.dsize());
}

/**
*  Is the buffer pointed to by 'p' the final part of an XML chunk?
*/
bool is_xml_end(net_mobilewebprint::byte const * p, size_t len) {
  if (len < 6) { return false; }
  return (*p++ == '<' && *p++ == '/' && *p++ == 'c' && *p++ == 'm' && *p++ == 'd' && *p++ == '>');
}

/**
*  Is the buffer pointed to by 'p' a BJNP 'binary' command?
*/
bool is_bjnp_cmd(net_mobilewebprint::byte const * p, size_t len) {
  if (len < 6) { return false; }
  return (*p++ == 0x1b && *p++ == 0x5b && *p++ == 0x4b && *p++ == 2 && *p++ == 0 && *p++ == 0);
}

/**
*  Is the buffer_t a BJNP 'binary' command?
*/
bool is_bjnp_cmd(buffer_view_i const & buffer) {
  return is_bjnp_cmd(buffer.const_begin(), buffer.dsize());
}

/**
*  If there is XML data buried in the buffer, find it.
*/
int find_xml(buffer_view_i const & buffer) {
  net_mobilewebprint::byte const * end = buffer.const_end();
  for (net_mobilewebprint::byte const * p = buffer.const_begin(); p < end; ++p) {
    if (is_xml(p, buffer.dsize() - (p - buffer.const_begin()))) {
      return (int)(p - buffer.const_begin());
    }
  }

  return (int)(buffer.dsize() + 1);
}

/**
*  If there is an end-to-XML data buried in the buffer, find it.
*/
int find_xml_end(buffer_view_i const & buffer) {
  for (size_t offset = 0; offset < buffer.dsize(); ++offset) {
    if (is_xml_end(buffer.const_begin() + offset, buffer.dsize() - offset)) {
      return (int)(offset + 6);
    }
  }

  return -1;
}

/**
*  If ther is a BJNP 'binary' command buried in the buffer, find it.
*/
int find_bjnp_cmd(buffer_view_i const & buffer, int offset_) {
  for (int offset = offset_; offset < (int)buffer.dsize(); ++offset) {
    if (is_bjnp_cmd(buffer.const_begin() + offset, buffer.dsize()- offset)) {
      return offset;
    }
  }

  return (int)(buffer.dsize() + 1);
}


using std::string;
using namespace net_mobilewebprint;

/**
*  A helper for the implementation to check if the XML-ish thing that was
*  read from the stream ("chunk") matches the XML-ish thing we got as the
*  response from a status query ("payload_of(response)").
*
*/
bool xml_match(buffer_view_i const & chunk, buffer_view_i const & response) {
  string input_xml = bjnp::payload_of(chunk).to_lower();
  string output_xml = bjnp::payload_of(response).to_lower();

  string operation = get_xml_value(input_xml, "operation");
  string operationResp = get_xml_value(output_xml, "operation");

  return operationResp == operation + "response";
}

/**
*  A helper for the implementation to tell if the current XML-ish chunk/response
*  is a given operation (like "StartJob" or "EndJob").
*/
bool xml_is(buffer_view_i const & chunk, buffer_view_i const & response, char const * operation_type) {
  string input_xml = bjnp::payload_of(chunk).to_lower();
  string operation = get_xml_value(input_xml, "operation");
  if (operation != operation_type) {
    return false;
  }

  string output_xml = bjnp::payload_of(response).to_lower();
  string operationResp = get_xml_value(output_xml, "operation");

  return operationResp == operation + "response";
}

/**
*  Helper to get a value from an XML chunk.
*
*  Could be much better - doesn't actually parse XML.
*/
std::string get_xml_value(string const & xml, char const * ctag) {
  string tag, cdata_tag("<![cdata[");
  size_t pos = 0, end_pos = 0, cdata_pos = 0, next = 0;

  tag = ":";
  tag += ctag;
  tag += ">";

  if ((pos = xml.find(tag)) != string::npos) {
    pos += tag.length();

    cdata_pos = xml.find(cdata_tag, pos);
    next      = min(cdata_pos, xml.find("<", pos));

    if (cdata_pos == next) {
      pos += cdata_tag.length();
      end_pos = xml.find("]]", pos);
    } else {
      end_pos = next;
    }
    return string(xml, pos, end_pos - pos);
  }

  return "";
}

