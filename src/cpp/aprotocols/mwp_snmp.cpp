
#include "mwp_snmp.hpp"
#include "mwp_types.hpp"
#include "mwp_controller.hpp"
#include <vector>
#include <string>

#define HP_MWP_SNMP_PORT 161

using std::vector;
using std::string;
using net_mobilewebprint::byte;
using net_mobilewebprint::buffer_t;
using net_mobilewebprint::udp2_t;
using net_mobilewebprint::network_node_t;
using net_mobilewebprint::split_v;

using namespace net_mobilewebprint::msg;
using namespace net_mobilewebprint::mq_enum;

static char device_id_oid_num[] = "1.3.6.1.4.1.11.2.3.9.1.1.7.0";
static char    status_oid_num[] = "1.3.6.1.4.1.11.2.3.9.1.1.3.0";

static char work_message_id[]   = "_on_snmp_work";

static uint32 device_id_work_id = 1000;
static uint32 status_work_id    = 1001;

enum e_field_triple
{
  integer_field_id        = 0x02,
  octet_string_field_id   = 0x04,
  null_field_id           = 0x05,
  oid_field_id            = 0x06,
  sequence_field_id       = 0x30,
  get_request_field_id    = 0xa0,
  get_response_field_id   = 0xa2,
  set_request_id          = 0xa3
};

/**
*  snmp: snmp_t ctor.
*/
net_mobilewebprint::snmp_t::snmp_t(controller_base_t & controller_)
  : controller(controller_), mq(net_mobilewebprint::get_mq(controller_)), socket(NULL),
    device_id_pkt(snmp::get_1284_pkt()), status_pkt(snmp::get_status_pkt()),
    cleanup_time(0), cleanup_interval(5000)
{
  mq.on(this);
  mq.on_selected(this);

  get_request_pkt(device_id_oid_num);
  get_request_pkt(status_oid_num);

  _init();
}

std::string net_mobilewebprint::snmp_t::mod_name()
{
  return "snmp_t";
}

/**
 *  Sends a message to MQ that will cause this module to send a request for the
 *  DeviceID to the ip via SNMP.
 *
 *  Uses _send_work_packet to do the actual queueing.
 */
void net_mobilewebprint::snmp_t::send_device_id_request(string const & ip, char const * oid)
{
  if (controller.printers.port_for_proto(ip, SOCK_DGRAM, HP_MWP_SNMP_PORT) == -1) { return; }
  if (oid == NULL) { return; }

  _send_work_packet(device_id_work_id, ip, oid);
}

/**
 *  Sends a message to MQ that will cause this module to send a request for status
 *  of this ip via SNMP.
 *
 *  Uses _send_work_packet to do the actual queueing.
 */
void net_mobilewebprint::snmp_t::send_status_request(string const & ip, char const * oid)
{
  if (controller.printers.port_for_proto(ip, SOCK_DGRAM, HP_MWP_SNMP_PORT) == -1) { return; }
  if (oid == NULL) { return; }

  _send_work_packet(status_work_id, ip, oid);
}

/**
 *  Sends a message to MQ that will cause this module to send the OID via SNMP.
 */
net_mobilewebprint::snmp_t & net_mobilewebprint::snmp_t::_send_work_packet(uint32 work_id, string const & ip, char const * oid)
{
  buffer_t * message = NULL;

  if ((message = mq.message(work_message_id, work_id, 0)) != NULL) {
    message->appendT(ip);
    message->appendT(oid);
    mq.send(message);
  }

  return *this;
}

/**
 *  Informs MQ of our needs regarding which FDs to watch.
 */
int net_mobilewebprint::snmp_t::pre_select(mq_pre_select_data_t * pre_select_data)
{
  if (socket == NULL) { return 0; }

  if (requests.size() > 0) {
    mq.check_udp_write(*socket);
  }

  return 0;
}

/**
 *  Handles reading from the socket.
 */
e_handle_result net_mobilewebprint::snmp_t::_mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra)
{
  if (!mwp_assert(socket)) { return unhandled; }

  e_handle_result result = unhandled;

  int num_bytes = 0;

  // Should we send a packet?
  if (requests.size() > 0) {
    if (extra.is_udp_writable(*socket)) {
      buffer_to_dest_t * packet = NULL;
      if ((packet = pull(requests)) != NULL) {
        if ((num_bytes = socket->send_udp_to(*packet->buffer, packet->ip, packet->port)) <= 0) {
          log_v(2, "", "Error: snmp-sending-to %s:%d result:%d, errno: %d\n", packet->ip.c_str(), packet->port, num_bytes, socket->last_error);
          controller.printers.network_error(packet->ip, socket->last_error);
        }
        delete packet;
        result = handled;
      }
    }
  }

  // Should we read from the socket?
  if (extra.is_udp_readable(*socket)) {
    network_node_t    sender;

    buffer_t * packet = mq.message("_on_snmp_packet", 0, extra.txn_id);
    size_t ip_offset = packet->data_length;
    packet->append((uint32)0);
    packet->append((uint16)0);

    if ((num_bytes = socket->recv_udp_from(*packet, sender, 2 * 1024)) > 0) {
      result = handled;
      log_v("received from: %s:%d", sender.ip.c_str(), sender.port);

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

e_handle_result net_mobilewebprint::snmp_t::handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  if (name == "_on_snmp_packet") { return _on_raw_packet(name, payload, data, extra); }
  if (name == work_message_id)   { return _on_work(name, payload, data, extra); }

  return not_impl;
}

e_handle_result net_mobilewebprint::snmp_t::_on_raw_packet(string const & name, buffer_view_i const & message_payload, buffer_t * data, mq_handler_extra_t & extra)
{
  buffer_view_i::const_iterator p = message_payload.first();

  string source_ip = message_payload.read_ip(p);
  uint16 source_port = message_payload.read_uint16(p);

  // The SNMP payload starts here
  buffer_range_view_t payload(p, message_payload.const_end());

  string key;
  buffer_t value = snmp::decode(payload, key);

  strmap attrs, attrs_lc;
  add_kv(attrs, "ip", source_ip);

  if (key == device_id_oid_num) {
    split_kv(attrs, value.str(), ';', ':', &attrs_lc);
    controller.from_1284_attrs(attrs);
  } else if (key == status_oid_num) {
    //log_d(1, "", "snmp status: %s %s", source_ip.c_str(), value.str().c_str());
    add_kv(attrs, "status", value.str());
    controller.from_attrs(attrs);
  } else {

    map<string, buffer_view_i const *> snmp_attrs;
    snmp_attrs.insert(make_pair(key, &value));

    controller.printers.from_snmp(source_ip, snmp_attrs);
  }

  return handled;
}

e_handle_result net_mobilewebprint::snmp_t::_on_work(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  buffer_view_i::const_iterator p = payload.first();

  string ip = payload.read_string(p);
  string oid = payload.read_string(p);

  buffer_t const * pkt = get_request_pkt(oid);

  if (extra.id == device_id_work_id) {
    requests.push_back(new buffer_to_dest_t(pkt, ip, HP_MWP_SNMP_PORT));
    return handled;
  }

  /* otherwise */
  if (extra.id == status_work_id) {
    requests.push_back(new buffer_to_dest_t(pkt, ip, HP_MWP_SNMP_PORT));
    return handled;
  }

  return unhandled;
}

net_mobilewebprint::snmp_t & net_mobilewebprint::snmp_t::_init()
{
  if (socket == NULL) {
    (socket = new network_node_t())->udp();
    mq.register_udp_for_select(*socket);
  }

  return *this;
}

net_mobilewebprint::buffer_t const * net_mobilewebprint::snmp_t::get_request_pkt(string const & oid)
{
  return get_request_pkt(oid.c_str());
}

net_mobilewebprint::buffer_t const * net_mobilewebprint::snmp_t::get_request_pkt(char const * oid)
{
  std::map<string, buffer_t>::const_iterator it;

  // If we already have it, return it
  if ((it = request_pkts.find(oid)) != request_pkts.end()) { return &(it->second); }

  /* otherwise, build it and then return it */
  return &(request_pkts[oid] = snmp::get_oid_pkt(oid));
}












bool net_mobilewebprint::snmp::snmp_t::_1284_device_id(network_node_t & network_node, string & result)
{
  return get_snmp(udp, get_1284_pkt(), network_node, result, NULL);
}

bool net_mobilewebprint::snmp::snmp_t::_1284_device_id(network_node_t & network_node, string & result, string & key)
{
  return get_snmp(udp, get_1284_pkt(), network_node, result, &key);
}

bool net_mobilewebprint::snmp::snmp_t::status(network_node_t & network_node, string & result)
{
  return get_snmp(udp, get_status_pkt(), network_node, result, NULL);
}

bool net_mobilewebprint::snmp::snmp_t::status(network_node_t & network_node, string & result, string & key)
{
  return get_snmp(udp, get_status_pkt(), network_node, result, &key);
}

bool net_mobilewebprint::snmp::get_snmp(udp2_t udp, buffer_t pkt, network_node_t & network_node, string & result, string * pkey)
{
  string key;
  if (pkey == NULL) { pkey = &key; }

  buffer_t buffer(4096);

  if (!udp.blast(buffer, network_node, pkt)) { return false; }

  /* otherwise */
  buffer_t value = snmp::decode(buffer, *pkey);

  result = value.str();

  return true;
}

int skip(byte const *& p)   // I can change p, but not what p points to
{
  int result = 0;
  if ((*p & 0x80) == 0) { return *p++; }

  for (int num_bytes = (*p++ & 0x7f); num_bytes > 0; --num_bytes) {
    result <<= 8;
    result += *p++;
  }

  return result;
}

buffer_t net_mobilewebprint::snmp::decode(buffer_view_i const & response, string & key_str)
{
  buffer_t result;

  byte const * p = response.const_begin(), *end = response.const_end();
  int count = 0;

  if (p >= end || *p++ != sequence_field_id) { return result; }
  count = skip(p);
  end = min(end, p + count);

  if (p >= end || *p++ != integer_field_id) { return result; }     // Version
  p += skip(p);

  if (p >= end || *p++ != octet_string_field_id) { return result; }
  p += skip(p);

  if (p >= end || *p++ != get_response_field_id) { return result; }
  count = skip(p);
  end = min(end, p + count);

  if (p >= end || *p++ != integer_field_id) { return result; }     // Request Id
  p += skip(p);

  if (p >= end || *p++ != integer_field_id) { return result; }     // Error
  p += skip(p);

  if (p >= end || *p++ != integer_field_id) { return result; }     // Error Index
  p += skip(p);

  if (p >= end || *p++ != sequence_field_id) { return result; }          // Varbind List
  count = skip(p);
  end = min(end, p + count);

  if (p >= end || *p++ != sequence_field_id) { return result; }          // Varbind
  count = skip(p);
  end = min(end, p + count);

  if (p >= end || *p++ != oid_field_id) { return result; }
  buffer_t key = buffer_t(p + 1, (size_t)*p, 0);
  p += skip(p);

  buffer_view_i::const_iterator pkey = key.const_begin();
  int x = key.read_byte(pkey);
  key_str = mwp_itoa(x / 40) + '.';
  key_str += mwp_itoa(x % 40) + '.';
  while (pkey != key.const_end()) {
    key_str += mwp_itoa(key.read_byte(pkey)) + '.';
  }
  key_str = rtrim(key_str, '.');

  if (p++ >= end) { return result; }
  count = skip(p);
  result = buffer_t(p, count, 0);

  return result;
}

static byte _the_null_field[] = { null_field_id, 0 };
static buffer_t the_null_field(_the_null_field, 2, 0);

buffer_t net_mobilewebprint::snmp::oid_field(int const * pn, int count)
{
  buffer_t result(1 + count);

  // TODO: Note, not handling numbers > 127
  result.set(0, (byte)oid_field_id);
  result.set(1, (byte)(count - 1));

  result.set(2, (byte)(40 * pn[0] + pn[1]));
  for (int i = 2; i < count; ++i) {
    result.set(i+1, (byte)pn[i]);
  }

  return result;
}

buffer_t net_mobilewebprint::snmp::octet_string(char const * s)
{
  buffer_t result(2 + strlen(s));
  result.set(0, (byte)octet_string_field_id);
  result.set(1, (byte)strlen(s));

  for (int i = 0; i < (int)strlen(s); ++i) {
    result.set(i+2, (byte)s[i]);
  }

  return result;
}

buffer_t integer_value(int n)
{
  byte b[3];
  b[0] = integer_field_id;
  b[1] = 1;
  b[2] = (byte)n;
  return buffer_t(b, 3, 0);
}

buffer_t net_mobilewebprint::snmp::binary_oid(char const * str_oid)
{
  vector<string> parts = split_v(str_oid, '.');
  buffer_t result(parts.size() * sizeof(int));

  int * pn = (int*)result.bytes;

  for (size_t i = 0; i < parts.size(); ++i) {
    *(pn + i) = atoi(parts[i].c_str());
  }

  return result;
}

buffer_t net_mobilewebprint::snmp::get_1284_pkt()
{
  return get_oid_pkt(device_id_oid_num);
}

buffer_t net_mobilewebprint::snmp::get_status_pkt()
{
  return get_oid_pkt(status_oid_num);
}

buffer_t net_mobilewebprint::snmp::get_oid_pkt(char const * oid)
{
  buffer_t boid = binary_oid(oid);
  return get_oid_pkt((int*)boid.bytes, (int)(boid.data_length / sizeof(int)));
}

buffer_t net_mobilewebprint::snmp::get_oid_pkt(int const *pn, int count)
{
  buffer_t varbind = sequence(oid_field(pn, count), the_null_field);
  buffer_t varbind_list = sequence(varbind);
  buffer_t pdu = get_request(
    integer_value(1), // request-id
    integer_value(0), // Error
    integer_value(0), // Error-index
    varbind_list
  );

  buffer_t message = sequence(
    integer_value(0), // Version
    octet_string("public"),
    pdu
  );

  return message;
}

buffer_t net_mobilewebprint::snmp::sequence(vector<buffer_t> const & seq)
{
  vector<buffer_t>::const_iterator i;
  int byte_count = 0;
  for (i = seq.begin(); i != seq.end(); ++i) {
    byte_count += i->data_length;
  }

  byte type = sequence_field_id;
  buffer_t result(&type, 1, 0);

  result.append(encode(byte_count));
  for (i = seq.begin(); i != seq.end(); ++i) {
    result.append(*i);
  }

  return result;
}

buffer_t net_mobilewebprint::snmp::encode(int n)
{
  buffer_t result(4);
  result.data_length = 0;

  do {
    result.fill_back(n & 0x7f);
    n &= ~0x7f;
    n >>= 7;
  } while (n != 0);

  return result;
}

buffer_t net_mobilewebprint::snmp::sequence(buffer_t a)
{
  vector<buffer_t> v(1);
  v[0] = a;

  return sequence(v);
}

buffer_t net_mobilewebprint::snmp::sequence(buffer_t a, buffer_t b)
{
  vector<buffer_t> v(2);
  v[0] = a;
  v[1] = b;

  return sequence(v);
}

buffer_t net_mobilewebprint::snmp::sequence(buffer_t a, buffer_t b, buffer_t c)
{
  vector<buffer_t> v(3);
  v[0] = a;
  v[1] = b;
  v[2] = c;

  return sequence(v);
}

buffer_t net_mobilewebprint::snmp::get_request(buffer_t a, buffer_t b, buffer_t c, buffer_t d)
{
  buffer_t result = sequence(a, b, c, d);
  result.set(0, (byte)get_request_field_id);
  return result;
}

buffer_t net_mobilewebprint::snmp::sequence(buffer_t a, buffer_t b, buffer_t c, buffer_t d)
{
  vector<buffer_t> v(4);
  v[0] = a;
  v[1] = b;
  v[2] = c;
  v[3] = d;

  return sequence(v);
}

buffer_t net_mobilewebprint::snmp::sequence(buffer_t a, buffer_t b, buffer_t c, vector<buffer_t> seq)
{
  vector<buffer_t> v(seq);

  v.insert(v.begin(), a);
  v.insert(v.begin(), b);
  v.insert(v.begin(), c);

  return sequence(v);
}

