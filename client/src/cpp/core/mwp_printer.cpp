
#include "mwp_printer.hpp"
#include "mwp_controller.hpp"
#include "mwp_utils.hpp"

#include <cstdlib>

using namespace net_mobilewebprint::msg;
using net_mobilewebprint::strmap;
using net_mobilewebprint::_upper;
using std::make_pair;

uint32 net_mobilewebprint::printer_t::status_interval_normal    = 1000;
uint32 net_mobilewebprint::printer_t::status_interval_printing  = 200;

static char           status_oid_num[] = "1.3.6.1.4.1.11.2.3.9.1.1.3.0";
static char universal_status_oid_num[] = "1.3.6.1.2.1.25.3.5.1.1.1";

static char const * universal_status(int n);

static bool _is_hp(net_mobilewebprint::printer_t const &);
static bool _is_epson(net_mobilewebprint::printer_t const &);
static bool _is_brother(net_mobilewebprint::printer_t const &);
static bool _is_canon(net_mobilewebprint::printer_t const &);
//static bool _is_snmp(net_mobilewebprint::printer_t const &);

static char const * _snmp_device_id_oid(net_mobilewebprint::printer_t const &);
static char const * _snmp_status_oid(net_mobilewebprint::printer_t const &);

static net_mobilewebprint::printer_list_t * g_printer_list = NULL;

net_mobilewebprint::printer_t::printer_t(controller_base_t & controller_)
  : controller(controller_), port(-1), connection(NULL), connection_id(0), bjnp_connection(NULL),
    num_soft_network_errors(0), max_status_arrival_delay(0), last_status_arrival(0), status_request_pending(false), num_status_misses(0), max_num_status_misses(0),
    status_interval(status_interval_normal), status_time(0),
    is_supported(NULL), num_is_supported_asks(0)
{
}

net_mobilewebprint::printer_t::printer_t(controller_base_t & controller_, string const & ip_)
  : controller(controller_), ip(ip_), port(-1), connection(NULL), connection_id(0), bjnp_connection(NULL),
    num_soft_network_errors(0), max_status_arrival_delay(0), last_status_arrival(0), status_request_pending(false), num_status_misses(0), max_num_status_misses(0),
    status_interval(status_interval_normal), status_time(0),
    is_supported(NULL), num_is_supported_asks(0)
{
}

net_mobilewebprint::printer_t::~printer_t()
{
  if (is_supported) {
    delete is_supported;
  }
}


/**
 *  What port should be used for the given "protocol"
 */
int net_mobilewebprint::printer_t::port_for_proto(int udp_tcp, int proto)
{
  // Print-bits port
  if (proto == 9100) {
    if (port != -1) {
      return port;
    }
    return -1;
  }

  /* otherwise -- SNMP */
  if (proto == 161) {
    if (port == 9100) {   // HP and Epson are both 9100 and 161
      return 161;
    }
    return -1;
  }

  return -1;
}

std::string net_mobilewebprint::printer_t::name() const
{
  strmap::const_iterator it = attrs_lc.find("name");
  if (it != attrs_lc.end()) { return value(it); }

  /* otherwise */
  return _1284_device_id;
}

std::string net_mobilewebprint::printer_t::status() const
{
  strmap::const_iterator it = attrs_lc.find("status");
  if (it != attrs_lc.end()) { return value(it); }

  /* otherwise */
  return "";
}

int net_mobilewebprint::printer_t::score() const
{
  // If someone has set the score, use that
  if (_has(attrs, "score")) {
    return _lookup(attrs, "score", 0);
  }

  /* otherwise -- compute */

  // Compute the base number
  int result = 1;     // The lowest practical score (0 is worse, and negative means don't show)

  if (_is_hp(*this)) {
    result = 100;

    if (_1284_device_id.length() > 0) {
      if (_1284_device_id.find("otosmart") != std::string::npos) {
        result += 10;
      }

      if (_1284_device_id.find("309") != std::string::npos) {
        result += 5;
      }

    }
  }

  return result;
}

void net_mobilewebprint::printer_t::dump() {
  log_d(attrs, "-----------------", "-----------------");
}

bool net_mobilewebprint::printer_t::has_ip() const {
  return ip.length() > 0;
}

bool net_mobilewebprint::printer_t::has_mac() const {
  return mac.length() > 0;
}

void net_mobilewebprint::printer_t::set_mac(string const & mac_)
{
  if (mac_.length() == 12) {
    // No ':' separators
    mac  = mac_.substr(0, 2) + ':';
    mac += mac_.substr(2, 2) + ':';
    mac += mac_.substr(4, 2) + ':';
    mac += mac_.substr(6, 2) + ':';
    mac += mac_.substr(8, 2) + ':';
    mac += mac_.substr(10, 2);
    return;
  }

  mac = mac_;
}

net_mobilewebprint::printer_t const & net_mobilewebprint::printer_t::merge(printer_t const & that) {
  if (this != &that) {
    if (that.has_ip())  { ip  = that.ip; }
    if (that.has_mac()) { mac = that.mac; }
    if (port == -1)     { port = that.port; }

    if (_1284_device_id.length() == 0) { _1284_device_id = that._1284_device_id; }

    node = that.node;

    _extend(attrs, that.attrs);
    _extend(attrs_lc, that.attrs_lc);
    _extend(_1284_attrs, that._1284_attrs);
    _extend(_1284_attrs_lc, that._1284_attrs_lc);

    status_interval         = max(min(status_interval, that.status_interval), status_interval_printing);
    num_soft_network_errors = max(num_soft_network_errors, that.num_soft_network_errors);

    last_status_arrival     = get_tick_count();
    status_request_pending  = status_request_pending || that.status_request_pending;
    num_status_misses      += that.num_status_misses;

    // Fixup the status attributes -- if that has a status, this is a status arrival
    if (_has(that.attrs_lc, "status")) {
      last_status_arrival     = get_tick_count();
      status_request_pending  = false;
      num_status_misses       = 0;
    }

    if (that.status_time != 0) {
      status_time     = min(status_time, that.status_time);
    }
  }

  return *this;
}

void net_mobilewebprint::printer_t::set_attr(string const & key, string const & key_lc, string const & value)
{
  // Is this a special name?
  if (key_lc == "ip") {
    ip = value;
  } else if (key_lc == "1284_device_id" || key_lc == "_1284_device_id") {
    _1284_device_id = value;
  } else if (key_lc == "port") {
    port = mwp_atoi(value);
  } else {
    attrs[key] = value;
    attrs_lc[key_lc] = value;
  }

  if (key_lc == "status") {

    last_status_arrival     = get_tick_count();
    status_request_pending  = false;
    num_status_misses       = 0;

    // Inform controller of status
    if (connection_id != 0) {
      controller.job_stat(connection_id, "status", value);
    }
  }

}

bool net_mobilewebprint::printer_t::from_attrs(strmap const & attrs_)
{
  strlist tags;
  return from_attrs(attrs_, tags);
}

bool net_mobilewebprint::printer_t::from_attrs(strmap const & attrs_, strlist const & tags_)
{
  fixup_snapshot_t snapshot(*this);

  append_to(tags, tags_);

  // Loop over the attrs; find the special ones; make a lower-case version
  for (strmap::const_iterator it = attrs_.begin(); it != attrs_.end(); ++it) {
    string const & key = it->first;
    string const & value = it->second;

    string key_lc = _lower(key);

    set_attr(key, key_lc, value);
  }

  last_status_arrival     = get_tick_count();

  snapshot.fixup(*this);
  return true;
}

bool net_mobilewebprint::printer_t::from_1284_attrs(strmap const & attrs_)
{
  strlist tags;
  return from_1284_attrs(attrs_, tags);
}

bool net_mobilewebprint::printer_t::from_1284_attrs(strmap const & attrs_, strlist const & tags_)
{
  fixup_snapshot_t snapshot(*this);

  append_to(tags, tags_);

  // Loop over the attrs; find the special ones; make a lower-case version
  for (strmap::const_iterator it = attrs_.begin(); it != attrs_.end(); ++it) {
    string const & key = it->first;
    string const & value = it->second;

    string key_lc = _lower(key);

    // Is this a special name?
    if (key_lc == "ip") {
      ip = value;
    } else {

      if (key_lc == "mdl") {
        _1284_device_id = value;
      } else if (key_lc == "mfg") {
        attrs[key] = value;
        attrs_lc[key_lc] = value;
      }

      _1284_attrs[key] = value;
      _1284_attrs_lc[key_lc] = value;
    }
  }

  last_status_arrival     = get_tick_count();

  snapshot.fixup(*this);
  return true;
}

bool net_mobilewebprint::printer_t::from_snmp(string ip_, map<string, buffer_view_i const *> const & snmp_attrs)
{
  strmap attrs;

  ip = ip_;

  for (map<string, buffer_view_i const *>::const_iterator it = snmp_attrs.begin(); it != snmp_attrs.end(); ++it) {
    string const &                   key   = it->first;
    buffer_view_i const * const &    value = it->second;

    if (key == universal_status_oid_num) {
      buffer_view_i::const_iterator p = value->const_begin();

      if (value->is_valid(p, sizeof(byte))) {
        add_kv(attrs, "status", universal_status((int)value->read_byte(p)));
      }
    }
  }

  //net_mobilewebprint::dump(attrs);
  return from_attrs(attrs);
}

bool net_mobilewebprint::printer_t::from_slp(slp_t & slp, buffer_view_i const & payload)
{
  strmap attrs_local, attrs_lc_local;

  if (!mwp_assert(parse_slp(slp, payload, attrs_local, attrs_lc_local, tags), "printer_t::from_slp parse_slp")) { return false; }

  fixup_snapshot_t snapshot(*this);

  // Fixup
  strmap::const_iterator it;
  strlist list, list2;
  strlist::const_iterator lit;

  serialization_json_t json;

  if ((it = attrs_lc_local.find("mwp-sender")) != attrs_lc_local.end()) {
    ip = it->second;
    json.set("mwpIp", ip);
  }

  if ((it = attrs_lc_local.find("mwp-port")) != attrs_lc_local.end()) {
    port = mwp_atoi(it->second);
    json.set("mwpPort", port);
  }

  if ((it = attrs_lc_local.find("x-hp-mac")) != attrs_lc_local.end()) {

    string m(it->second), realMac;
    set_mac(m);

    if (m.length() >= 12) {
      realMac = realMac + m[0] + m[1] + ":" + m[2] + m[3] + ":" + m[4] + m[5] + ":" + m[6] + m[7] + ":" + m[8] + m[9] + ":" + m[10] + m[11];
      json.set("mac", realMac);
    }
  }

  if ((it = attrs_lc_local.find("x-hp-p1")) != attrs_lc_local.end()) {
    strmap _1284_attrs_local;
    if (split_kv(_1284_attrs_local, it->second, ';', ':', NULL)) {
      from_1284_attrs(_1284_attrs_local);

      for (strmap::const_iterator it = _1284_attrs_local.begin(); it != _1284_attrs_local.end(); ++it) {
        string const & key = _lower(it->first);
        string const & value = it->second;
        json.set(key, value);
      }
    }
  }

  string slp_ip;
  if ((it = attrs_lc_local.find("x-hp-ip")) != attrs_lc_local.end()) {
    list2 = strlist();
    list = split(it->second, '.');
    for (lit = list.begin(); lit != list.end(); ++lit) {
      // TODO: FIXME this will turn "010.000.005.049" into "10..5.49"
      list2.push_back(skip_char(*lit, '0'));
    }

    slp_ip = join(list2, ".");
    json.set("ip", slp_ip);
    if (!has_ip()) {
      ip = slp_ip;
    }
  }

  controller.sendTelemetry("printerScan", "slpResponse", json);

  last_status_arrival     = get_tick_count();

  snapshot.fixup(*this);
  return true;
}

void net_mobilewebprint::fixup_snapshot_t::fixup(printer_t & that)
{
  if (that.ip != ip || that.port != port) {
    if (that.ip.length() > 0 && that.port != -1) {
      that.node = network_node_t(that.ip, that.port);
    }
  }
}

void net_mobilewebprint::printer_t::send_print_job(uint32 & connection_id_)
{
  status_interval = status_interval_printing;

  // Canon is a bit harder
  if (_is_canon(*this)) {

    bjnp_connection = controller.bjnp.send_to_printer(connection_id_, ip, port);
    connection_id = connection_id_;
//    bjnp_send_to_printer_start = get_tick_count();
    return;
  }

  /* otherwise -- the other printers just use TCP to 9100 */
  connection = controller.job_flow.send_to_printer(connection_id_, ip, port);
  connection_id = connection_id_;

  if (connection == NULL) {
    attrs_lc.insert(make_pair("status", "NETWORK_ERROR"));
    controller.job_stat(connection_id, "status", "NETWORK_ERROR");
    controller.job_stat(connection_id, "jobStatus", "NETWORK_ERROR");
    return;
  }

  /* otherwise */
  controller.job_stat(connection_id, "status", status());
  controller.job_stat(connection_id, "has_started", false);
//  controller.job_stat(connection_id, "jobStatus", STATUS_WAITING0);
}

bool net_mobilewebprint::printer_t::request_updates()
{
  bool result = false;

  char const * oid = NULL;

  if (_1284_device_id.length() == 0) {
    //log_d(1, "", "Asking for device_id for %s\n", ip.c_str());
    if ((oid = _snmp_device_id_oid(*this)) != NULL) {
      controller.snmp.send_device_id_request(ip, oid);
    } else {
      // This is a printer that doesn't use SNMP?
      if (_is_canon(*this)) {
        uint32 connection_id = controller._unique();
        controller.bjnp.discover(connection_id, ip, port);
      }
    }
  }

  if (status().length() == 0) {
    if ((oid = _snmp_status_oid(*this)) != NULL) {
    }
  }

#if 1
  if (connection_id != 0) {
    if (_is_canon(*this)) {
      if (!controller.bjnp.is_connection_alive(connection_id)) {
        bjnp_connection = NULL;
        connection_id = 0;
      }
    }
  }
#else
  // If we have started a bjnp job, they rely on a few UDP packets working. Since UDP
  // is not reliable, if the job hasn't actually started (tcp opening), try again
  if (bjnp_connection != NULL && get_tick_count() - bjnp_send_to_printer_start > 500) {
    bjnp_connection->send_to_printer();
    bjnp_send_to_printer_start = get_tick_count();
  }
#endif

  return result;
}

std::string net_mobilewebprint::printer_t::attr_list()
{
  strlist list;

  list.push_back(string("ip:") + ip);
  list.push_back(string("name:") + _1284_device_id);
  list.push_back(string("1284_device_id:") + _1284_device_id);
  list.push_back(string("mac:") + mac);

  strmap::const_iterator it;
  for (it = attrs.begin(); it != attrs.end(); ++it) {
    list.push_back(it->first + ":" + it->second);
  }

  for (it = _1284_attrs.begin(); it != _1284_attrs.end(); ++it) {
    list.push_back(it->first + ":" + it->second);
  }

  log_d(join(list, ";"));
  return join(list, ";");
}

strmap net_mobilewebprint::printer_t::filtered_attrs(std::set<std::string> names)
{
  strmap result;

  if (names.find("ip") != names.end() && has_ip()) {
    result.insert(make_pair("ip", ip));
  }

  if (names.find("name") != names.end() && name().length() > 0) {
    result.insert(make_pair("name", name()));
  }

  if (names.find("1284_device_id") != names.end() && _1284_device_id.length() > 0) {
    result.insert(make_pair("1284_device_id", _1284_device_id));
  }

  strmap::const_iterator it;
  for (it = attrs.begin(); it != attrs.end(); ++it) {
    if (names.find(it->first) != names.end()) {
      result.insert(make_pair(it->first, it->second));
    }
  }

  for (it = _1284_attrs.begin(); it != _1284_attrs.end(); ++it) {
    if (names.find(it->first) != names.end()) {
      result.insert(make_pair(it->first, it->second));
    }
  }

  result.insert(make_pair("score", mwp_itoa(score(), 8)));    // Adding leading zeros means string compare is same as number compare

  if (names.find("mac") != names.end() && has_mac()) {
    result.insert(make_pair("mac", mac));
  }

  // Special processing.  If we don't have is_supported, we also will not send MAC.
  // This will make the app wait until we have both
  if (is_supported != NULL) {
//    if (names.find("is_supported") != names.end()) {
      result.insert(make_pair("is_supported", (*is_supported) ? "1" : "0"));
//    }

  }

  return result;
}

net_mobilewebprint::mq_enum::e_handle_result net_mobilewebprint::printer_t::on_select_loop_start(mq_select_loop_start_data_t const & data)
{
  char const * oid = NULL;

  //log_d(1, "", "printer loop start %d %d %d  %s", status_time, status_interval, data.current_loop_start, ip.c_str());
  if (_has_elapsed(status_time, status_interval, data.current_loop_start)) {
    if ((oid = _snmp_status_oid(*this)) != NULL) {

      if (status_request_pending) {
        num_status_misses += 1;
        max_num_status_misses = max(max_num_status_misses, num_status_misses);
      }

      controller.snmp.send_status_request(ip, oid);
      status_request_pending  = true;
    }
  }

  return handled;
}

std::string net_mobilewebprint::printer_t::to_json(bool for_debug)
{
  strmap xattrs;

  xattrs["ip"] = ip;

  if (has_mac())                      { xattrs["mac"]       = mac; }
  if (_1284_device_id.length() > 0)   { xattrs["deviceId"]  = _1284_device_id; }

  strlist kv_pairs;

  add_kvs(kv_pairs, xattrs);
  add_kvs(kv_pairs, attrs);
  add_kvs(kv_pairs, _1284_attrs);

  std::string result = "{";
  if (for_debug) { result += "\n"; }

  result += join(kv_pairs, for_debug ? ",\n" : ",");
  if (for_debug) { result += "\n"; }

  result += "}";
  return result;
}

void net_mobilewebprint::printer_t::make_server_json(serialization_json_t & json, bool forFilterPrinters)
{
  json.set("ip", ip);

  if (has_mac())                      { json.set("mac", mac); }
  if (_1284_device_id.length() > 0)   { json.set("deviceId", _1284_device_id); }

  json.set(attrs);

  if (!forFilterPrinters) {
    if (is_supported != NULL)         { json.set("is_supported", *is_supported); }

    json.set("num_is_supported_asks", num_is_supported_asks);
    json.set("score", score());

    json.set("num_soft_network_errors", num_soft_network_errors);
    json.set("num_status_misses", num_status_misses);
    json.set("max_num_status_misses", max_num_status_misses);
    json.set("last_status_arrival_ago", _time_since(last_status_arrival));
    json.set("max_status_arrival_delay", max_status_arrival_delay);
  }
}

bool net_mobilewebprint::printer_t::is_unknown(char const * purpose) const
{

  if (::strcmp(purpose, "ip") == 0)              { return !has_ip(); }
  if (::strcmp(purpose, "port") == 0)            { return port == 0; }
  if (::strcmp(purpose, "deviceId") == 0)        { return _1284_device_id.length() == 0; }
  if (::strcmp(purpose, "mac") == 0)             { return !has_mac(); }
  if (::strcmp(purpose, "is_supported") == 0)    { return is_supported == NULL; }
  if (::strcmp(purpose, "score") == 0)           { return score() < 0; }

  if (::strcmp(purpose, "name") == 0)            { return name().length() == 0; }
  if (::strcmp(purpose, "status") == 0)          { return !_has(attrs_lc, purpose); }

  return !_has(attrs, purpose);
}

bool net_mobilewebprint::printer_t::is_missing()
{
  // We can only tell for HP, Epson, and Brother
  if (!_is_hp(*this) && !_is_epson(*this) && !_is_brother(*this)) { return false; }

  int missing_threshold = controller.arg("printerMissingThreshold", /* default= */ 10500);   // 10.5 seconds

  // Log when the printer is "almost" missing
  if (num_status_misses > 2 && _time_since(last_status_arrival) < missing_threshold + 5000) {
    log_v(4, "ttt", "is the printer going missing? (%15s) %d %d", ip.c_str(), num_status_misses, _time_since(last_status_arrival));
  }

  max_status_arrival_delay= max(max_status_arrival_delay, _time_since(last_status_arrival));

  // Default 10.5 seconds of no status messages == the-printer-is-missing
  if (_time_since(last_status_arrival) > missing_threshold) {
    //log_v(2, "ttt", "printer missing(%15s): %d %d", ip.c_str(), num_status_misses, _time_since(last_status_arrival));
    return true;
  }

  return false;
}

bool net_mobilewebprint::printer_t::is_epson(){
  return _is_epson(*this);
}

//---------------------------------------------------------------------------------------
//
// printer_list_t
//
//---------------------------------------------------------------------------------------

std::set<std::string> net_mobilewebprint::printer_list_t::app_attribute_names;

static net_mobilewebprint::mq_manual_timer_t * watchdog  = NULL;
static net_mobilewebprint::mq_manual_timer_t * silencer  = NULL;

static int num_printer_lists = 0;
net_mobilewebprint::printer_list_t::printer_list_t(controller_base_t &controller_)
  : controller(controller_), mq(net_mobilewebprint::get_mq(controller_)), update_time(get_tick_count()), printer_enum_id(0),
    printer_list_in_flight(false),
    start_time(0),
    heartbeat(0, 100),
    printer_list_histo_timer(NULL),
    printer_list_histo_bucket(0),
    printer_list_telemetry_timer(0, 5000),
    sending_printer_list(0, 2000, false),
    send_scan_done(NULL),
    send_scan_done_zero_printers(NULL),
    send_scan_done_last_resort(NULL)
{
  log_d("%d printer lists %s", ++num_printer_lists, "");
  app_attribute_names.insert("ip");
  app_attribute_names.insert("name");
  app_attribute_names.insert("1284_device_id");
  app_attribute_names.insert("MFG");
  app_attribute_names.insert("status");
  app_attribute_names.insert("score");

  app_attribute_names.insert("is_supported");
  app_attribute_names.insert("mac");

  g_printer_list = this;
}

net_mobilewebprint::printer_t * net_mobilewebprint::printer_list_t::_get_printer(string const & ip_or_mac)
{
  plist_t::const_iterator it = by_ip.find(ip_or_mac);
  if (it != by_ip.end()) {
    return it->second;
  }

  /* otherwise -- maybe its a mac address */
  it = by_ip.find(get_ip(ip_or_mac));
  if (it != by_ip.end()) {
    return it->second;
  }

  /* otherwise */
  return NULL;
}

static int num_printer_list_sends = 0;
net_mobilewebprint::mq_enum::e_handle_result net_mobilewebprint::printer_list_t::on_select_loop_start(mq_select_loop_start_data_t const & loop_start_data)
{
  uint32 now = loop_start_data.current_loop_start;
  printer_t * printer = NULL;

  // BBB - Best breakpoint to watch the timing of the printer scan, and PRINTER_SCAN_DONE
  if (controller.flag("scanWatchdog", true) && watchdog != NULL && watchdog->has_elapsed(now)) {
    log_v(2, "ttt", "-------------- %8d: %8d %8d _Z %8d _X %8d until PRINTER_SCAN_DONE, printer_list in flight: %d time until send list: %d, unknowns %d/%d",
                  now - start_time,
                  send_scan_done                == NULL ? -999 : send_scan_done->time_remaining(now),
                  send_scan_done_zero_printers  == NULL ? -999 : send_scan_done_zero_printers->time_remaining(now),
                  send_scan_done_last_resort    == NULL ? -999 : send_scan_done_last_resort->time_remaining(now),
                  silencer                      == NULL ? -999 : silencer->time_remaining(now),
                  (int)printer_list_in_flight,
                  sending_printer_list.time_remaining(now),
                  unknown_is_supported_count(),
                  by_ip.size());
  }

  if (silencer != NULL && silencer->has_elapsed(now)) {
    log_v(3, "ttt", "-----------------------------------------------------------QUIET-------------------------------------");

    delete watchdog;
    watchdog = NULL;

    delete silencer;
    silencer = NULL;
  }

  int unk_is_supported_count = -1;
  if (heartbeat.has_elapsed(now)) {
    if (send_scan_done != NULL) {
      unk_is_supported_count = unknown_is_supported_count();
      if (unk_is_supported_count > 0) {
        //send_scan_done->delay(1000 + (25 * unk_is_supported_count));
        scan_activity_happened(now);
      }
    }
  }

  plist_t::const_iterator it;
  for (it = by_ip.begin(); it != by_ip.end(); ++it) {
    if ((printer = it->second) != NULL) {
      printer->on_select_loop_start(loop_start_data);
    }
  }

  // Collect histogram for printer discovery rate
  if (printer_list_histo_timer != NULL && printer_list_histo_timer->has_elapsed(now)) {
    printer_list_histo_bucket += 1;

    if (unk_is_supported_count < 0) {
      unk_is_supported_count = unknown_is_supported_count();
    }
    log_v(4, "", "printer_list-histo-timer %d %d", printer_list_histo_bucket, unk_is_supported_count);

    controller.sendTelemetry("printerScan", "discoveryHistogram",
        "count",                    by_ip.size(),
        "unknownIsSupportedCount",  unk_is_supported_count,
        "histoBucketTime",          printer_list_histo_timer->time_,
        "histoBucket",              printer_list_histo_bucket);

    // 35 seconds is enough
    if (printer_list_histo_bucket >= 35) {
      delete printer_list_histo_timer;
      printer_list_histo_timer = NULL;
    }
  }

  if (printer_list_telemetry_timer.has_elapsed(now)) {
    serialization_json_t json;
    serialization_json_t & sub_json = json.getObject("printers");

    int count = make_server_json(sub_json);
    controller.sendTelemetry("printers", "printerReport", json);
  }

  //log_d(1, "", "--------**********-----------------------------------------just checking %d", num_printer_list_sends);
  if (num_printer_list_sends < 100) {

    serialization_json_t json;
    int                  count              = 0;
    bool                 send_printer_list  = false;
    bool                 check_for_unknowns = false;

    if ((check_for_unknowns = sending_printer_list.has_elapsed(now))) {

      // The action happens below

    } else if (sending_printer_list.time_remaining() > 0x00ffffff) {
      send_printer_list = (has_unknown_is_supported() != NULL);
      if (send_printer_list) {
        log_v(2, "ttt", "------------- have %s unknown is_supported printer", has_unknown_is_supported()->ip.c_str());
        check_for_unknowns = true;
      }
    }

    if (check_for_unknowns) {

      serialization_json_t & sub_json = json.getObject("printers");
      if ((count = make_server_json(sub_json, "is_supported")) > 0) {
        send_printer_list = true;
        log_v(2, "ttt", "------------- have %d unknown is_supported printers", count);
      }
    }

    if (send_printer_list) {

      scan_activity_happened(now);

      if (printer_list_in_flight) {
        // Was supposed to send the list, but couldn't... Try again in a few.
        log_v(4, "ttt", "-----------------------------------------------------------Delay -- check send filtered printers later #%d", num_printer_list_sends);
        sending_printer_list.time = get_tick_count();
      } else {
        log_v(3, "ttt", "-----------------------------------------------------------sending filterPrinters %d/%d: #%d", count, by_ip.size(), num_printer_list_sends);

        string pathname = string("/filterPrinters?count=") + mwp_itoa(count) + "&total=" + mwp_itoa(by_ip.size());
        string stream_name = controller.send_upstream("info_for_printers", pathname, json, new printer_list_response_t());

        if (send_scan_done != NULL) { send_scan_done->delay(250); }
        printer_list_in_flight = true;
        num_printer_list_sends += 1;
      }

      // Make sure silencer has enough time to complete this
      if (silencer) {
        silencer->time = now;
      }
    }

    //log_v(3, "", "-----------------------------------------------------------just checking %d: #%d", count, num_printer_list_sends);
  }

  if (send_scan_done != NULL && send_scan_done->has_elapsed(now)) {

    int sendScanDoneDelay = 0;

    if ((printer = has_unknown_is_supported()) != NULL) {

      // Try again next time
      int count = unknown_is_supported_count();
      sendScanDoneDelay = 2000 + (20 * count);

      log_v(3, "ttt", "--------------------- delaying PRINTER_SCAN_DONE(%d): printer (of %d) with unknown is_supported |%s|", sendScanDoneDelay, count, printer->ip.c_str());

    } else if (printer_list_in_flight) {

      // Try again next time
      sendScanDoneDelay = 500;

      log_v(3, "ttt", "--------------------- delaying PRINTER_SCAN_DONE(%d): printer list is in fight", sendScanDoneDelay);

    } else {

      // Send the scan done message
      log_v(2, "ttt", "-++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++----------------------------- sending PRINTER_SCAN_DONE");
      controller.send_to_app(HP_MWP_PRINT_PROGRESS_MSG, -1, 0, "", "", "", "", "PRINTER_SCAN_DONE", (int)100, (int)1);
      controller.sendTelemetry("printerScan", "PRINTER_SCAN_DONE", "type", "normal");

      delete send_scan_done;
      send_scan_done = NULL;

      delete send_scan_done_last_resort;
      send_scan_done_last_resort = NULL;

      if (send_scan_done_zero_printers != NULL) {
        delete send_scan_done_zero_printers;
        send_scan_done_zero_printers = NULL;
      }

      silencer = new mq_manual_timer_t(get_tick_count(), 4500, false);
    }

    if (sendScanDoneDelay > 0) {
      send_scan_done->revert();
      send_scan_done->delay(sendScanDoneDelay);
    }
  }

  if (send_scan_done_last_resort != NULL) {

//    if (printer_list_in_flight) {
//
//      // Try again next time
//      send_scan_done_last_resort->revert();
//      send_scan_done_last_resort->delay(6500);
//
//    } else {
      if (send_scan_done_last_resort->has_elapsed(now)) {

        log_v(2, "ttt", "-++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++----------------------------- sending last resort PRINTER_SCAN_DONE");
        controller.send_to_app(HP_MWP_PRINT_PROGRESS_MSG, -1, 0, "", "", "", "", "PRINTER_SCAN_DONE", (int)100, (int)1);
        controller.sendTelemetry("printerScan", "PRINTER_SCAN_DONE", "type", "lastResort");

        delete send_scan_done;
        send_scan_done = NULL;

        delete send_scan_done_last_resort;
        send_scan_done_last_resort = NULL;

        if (send_scan_done_zero_printers != NULL) {
          delete send_scan_done_zero_printers;
          send_scan_done_zero_printers = NULL;
        }

        silencer = new mq_manual_timer_t(get_tick_count(), 4500, false);
      }
//    }
  }

  if (send_scan_done_zero_printers != NULL && send_scan_done_zero_printers->has_elapsed(now)) {
    if (by_ip.size() == 0) {
      log_v(2, "ttt", "-++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++----------------------------- sending zero-printer PRINTER_SCAN_DONE");
      controller.send_to_app(HP_MWP_PRINT_PROGRESS_MSG, -1, 0, "", "", "", "", "PRINTER_SCAN_DONE", (int)100, (int)1);
      controller.sendTelemetry("printerScan", "PRINTER_SCAN_DONE", "type", "zeroPrintersTimeout");

      delete send_scan_done;
      send_scan_done = NULL;

      delete send_scan_done_last_resort;
      send_scan_done_last_resort = NULL;

      silencer = new mq_manual_timer_t(get_tick_count(), 4500, false);
    }

    // We only check this timer once
    delete send_scan_done_zero_printers;
    send_scan_done_zero_printers = NULL;
  }

  return handled;
}

void net_mobilewebprint::printer_list_t::re_scan()
{
  if (send_scan_done != NULL) {
    log_v(2, "", "!!!!!!!!!!!!!! %s", "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    log_v(2, "", "!!!!!!!!!!!!!! %s", "Error -- reScan called during scan -- ignoring");
    log_v(2, "", "!!!!!!!!!!!!!! %s", "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    controller.sendTelemetry("printerScan", "invalidReScan");
    controller.sendTelemetry("anomaly", "ERROR_API", "fn", "re_scan", "reason", "already_scanning");
    return;
  }

  /* otherwise */
  mq.send("re_scan_for_printers");
  _start_timers();
  controller.sendTelemetry("printerScan", "reScan");
}

void net_mobilewebprint::printer_list_t::scan_activity_happened(uint32 now)
{
  if (now == 0) {
    now = get_tick_count();
  }

  if (send_scan_done != NULL) {
    uint32 new_time = (now - (send_scan_done->interval / 2));
    //uint32 new_time = now;
    if (send_scan_done->time < new_time) {
      send_scan_done->time = new_time;
    }
  }
}

void net_mobilewebprint::printer_list_t::_start_timers()
{
  // (Re-)start the timers
  if (watchdog == NULL)                       { watchdog                      = new mq_manual_timer_t(get_tick_count(), 100); }
  if (send_scan_done == NULL)                 { send_scan_done                = new mq_manual_timer_t(get_tick_count(), 4500, false); }
  if (send_scan_done_zero_printers == NULL)   { send_scan_done_zero_printers  = new mq_manual_timer_t(get_tick_count(), 15000, false); }
  if (send_scan_done_last_resort == NULL)     { send_scan_done_last_resort    = new mq_manual_timer_t(get_tick_count(), 35000, false); }
  if (printer_list_histo_timer == NULL)       { printer_list_histo_timer      = new mq_manual_timer_t(get_tick_count(), 1000); }

  // Restart timeouts
  start_time =
    sending_printer_list.time =
    send_scan_done->time =
    send_scan_done_zero_printers->time =
    printer_list_histo_timer->time =
    get_tick_count();

  //log_d(1, "", "------------------------------ (re)starting timers at %d", start_time);
}

net_mobilewebprint::e_handle_result net_mobilewebprint::printer_list_t::handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  if (name == scan_for_printers) {

    // Start the timers
    _start_timers();

  } else if (name == "WIFI_STATE_CHANGED") {

    byte const * p = payload.const_begin();
    string value = payload.read_string(p);
    controller.sendTelemetry("network", "WIFI_STATE_CHANGED", "state", value);
    if (value != "WIFI_STATE_DISABLED") {
      return handled;
    }

    /* otherwise -- The WiFi has gone away!!! */
    log_v(2, "ttt", "+++++++++++++++++++++++++++++++++++++++++++++++++++WIFI_STATE_CHANGED: |%s|", value.c_str());

    on_lost_wifi();

  } else if (name == "CONNECTIVITY_ENABLED") {

    byte const * p = payload.const_begin();
    string value_ = payload.read_string(p);

    controller.sendTelemetry("network", "CONNECTIVITY_ENABLED", "state", value_);
    log_v(2, "ttt", "+++++++++++++++++++++++++++++++++++++++++++++++++++CONNECTIVITY_ENABLED(%s): %s", controller.ssid.c_str(), value_.c_str());

    // How much changed?  (Did we switch networks?)
    string  value;
    bool    switching_networks = false;
    string  old_ssid           = controller.ssid;

    if (parse_network_change(value_, value, controller.ssid, controller.bssid) > 2) {

      // Are we switching?
      if (value == "true" && old_ssid.length() > 0 && controller.ssid.length() > 0) {
        switching_networks = (old_ssid != controller.ssid);
      }
      log_v(2, "", "----------------------------switching? %d %s %s %s", (int)switching_networks, value.c_str(), old_ssid.c_str(), controller.ssid.c_str());
    }

    if (switching_networks) {
      on_wifi_switch();
    } else if (value == "true") {
      on_gained_wifi();
    }

  } else if (name == "LOCAL_REACHABILITY_CHANGED") {

    byte const * p = payload.const_begin();
    string value_ = payload.read_string(p);

    log_v(2, "ttt", "+++++++++++++++++++++++++++++++++++++++++++++++++++LOCAL_REACHABILITY_CHANGED(%s): %s", controller.ssid.c_str(), value_.c_str());
    controller.sendTelemetry("network", "LOCAL_REACHABILITY_CHANGED", "state", value_);

    // How much changed?  (Did we switch networks?)
    string  value;
    bool    switching_networks = false;
    string  old_ssid           = controller.ssid;

    if (parse_network_change(value_, value, controller.ssid, controller.bssid) > 2) {

      // Are we switching?
      if (value == "REACHABLE" && old_ssid.length() > 0 && controller.ssid.length() > 0) {
        switching_networks = (old_ssid != controller.ssid);
      }
    }

    if (switching_networks) {
      on_wifi_switch();
    } else if (value == "REACHABLE") {
      on_gained_wifi();
    } else {
      on_lost_wifi();
    }

  } else if (name == "getSortedPrinterList_notification") {

    if (controller.flag("telemetryGSPL", /* default= */ true)) {
      // The app is asking for the printer list
      int count = 0;

      serialization_json_t json;
      serialization_json_t & sub_json = json.getObject("printers");

      count = make_server_json(sub_json);
      json.set("count", count);

      controller.sendTelemetry("app", "getSortedPrinterList", json);
    }
  }

  return handled;
}

void net_mobilewebprint::printer_list_t::handle_filter_printers(int code, std::string const & http_version, strmap const & headers, json_array_t const & json, stats_t const & stats_out)
{
  printer_list_in_flight = false;
  scan_activity_happened();

  if (code > 299) { return; }

  /* otherwise */
  bool have_sent_begin_msg = false;
  for (int i = 0; json.has(i); ++i) {
    json_t const * sub_json = json.get(i);
    log_v(4, "ttt", "----> /filterPrinter %s", sub_json->debug_string(A(string("MFG"), string("name")), A(string("mac"), string("is_supported"), string("ip"), string("status"))).c_str());
    if (sub_json && sub_json->has("ip")) {

      string const & ip             = sub_json->lookup("ip");
      bool           is_supported   = false;

      if (sub_json->has_bool("is_supported")) {
        is_supported = sub_json->lookup_bool("is_supported");
      }

//      // ---------------------------- TODO: remove brain damage -----------------------------------
//      if ((random() % 10) > 8) {
//        continue;
//      }

      if (by_ip.find(ip) != by_ip.end()) {
        printer_t * printer = (*by_ip.find(ip)).second;
        if (printer) {

          bool has_changed = false;

          if (printer->is_supported == NULL) {
            printer->is_supported = new bool(is_supported);
            has_changed = true;
          } else {
            has_changed = (*printer->is_supported != is_supported);
          }
          *printer->is_supported = is_supported;

          if (has_changed) {
            send_begin_msg(have_sent_begin_msg);
            controller.send_to_app(HP_MWP_PRINTER_ATTRIBUTE_MSG, -1, printer_enum_id, ip.c_str(), "is_supported", is_supported ? "1" : "0");
          }

          printer->num_is_supported_asks += 1;
        }
      }
    }
  }

  if (have_sent_begin_msg) {
    controller.send_to_app(HP_MWP_END_PRINTER_ENUM_MSG, -1, printer_enum_id);
  }

  // Now loop over the printers and see if any still need an is-supported check
  printer_t const * printer = NULL;
  for (plist_t::const_iterator it = by_ip.begin(); it != by_ip.end(); ++it) {
    if ((printer = it->second) != NULL) {
      if (printer->is_unknown("is_supported") && !printer->is_unknown("deviceId")) {
        sending_printer_list.trigger();

//        // ------------------------------ TODO: remove brain damage --------------------------
//        sending_printer_list.time = (sending_printer_list.time + get_tick_count()) / 2;
      }
    }
  }
}

net_mobilewebprint::printer_t * net_mobilewebprint::printer_list_t::has_unknown_is_supported()
{
  printer_t * printer = NULL;
  for (plist_t::iterator it = by_ip.begin(); it != by_ip.end(); ++it) {
    if ((printer = it->second) != NULL) {
      if (printer->is_unknown("is_supported")) {
        return printer;
      }
    }
  }

  return NULL;
}

int net_mobilewebprint::printer_list_t::unknown_is_supported_count()
{
  int count = 0;

  printer_t * printer = NULL;
  for (plist_t::iterator it = by_ip.begin(); it != by_ip.end(); ++it) {
    if ((printer = it->second) != NULL) {
      if (printer->is_unknown("is_supported")) {
        count += 1;
      }
    }
  }

  return count;
}

void net_mobilewebprint::printer_list_response_t::handle(int code, std::string const & http_version, strmap const & headers, string const & body, json_t const & json, json_array_t const & json_array, stats_t const & stats_out)
{
  log_v(3, "", "-----------------------------------------------------------response from filterPrinters %d", code);
  //log_vs(3, "", "-----------------------------------------------------------response from filterPrinters %s", json_array.stringify());

  if (g_printer_list) {
    g_printer_list->handle_filter_printers(code, http_version, headers, json_array, stats_out);
  }
}

net_mobilewebprint::e_handle_result net_mobilewebprint::printer_list_t::on_select_loop_idle(mq_select_loop_idle_data_t const & data)
{
  strmap            not_handled;
  strmap_entry      item;

  for (strmap::iterator it = new_mac_addresses.begin(); it != new_mac_addresses.end(); ++it) {
    item = *it;
    if (by_ip.find(item.first) != by_ip.end()) {
      printer_t * printer = new printer_t(controller, item.first);
      printer->set_mac(item.second);
      assimilate_printer_stats(printer);
    } else {
      not_handled.insert(item);
    }
  }

  new_mac_addresses = not_handled;

  return handled;
}

bool net_mobilewebprint::printer_list_t::from_attrs(strmap const & attrs)
{
  strlist tags;
  return from_attrs(attrs, tags);
}

bool net_mobilewebprint::printer_list_t::from_attrs(strmap const & attrs, strlist const & tags)
{
  printer_t * printer = new printer_t(controller);
  if (printer->from_attrs(attrs, tags)) {
    bool caused_change = assimilate_printer_stats(printer);

    //if (printer->ip == "10.7.5.35") {
    //  dump(attrs);

    //  if (caused_change) {
    //    log_d(1, "", "Attrs changed");
    //  }
    //}

    return caused_change;
  }

  return false;
}

bool net_mobilewebprint::printer_list_t::from_1284_attrs(strmap const & attrs)
{
  strlist tags;
  return from_1284_attrs(attrs, tags);
}

bool net_mobilewebprint::printer_list_t::from_1284_attrs(strmap const & attrs, strlist const & tags)
{
  printer_t * printer = new printer_t(controller);
  if (printer->from_1284_attrs(attrs, tags)) {
    return assimilate_printer_stats(printer);
  }

  return false;
}

void net_mobilewebprint::printer_list_t::soft_network_error(string const & ip, int error_number)
{
//  log_v(2, "", "Encountered network error (%d) for %s", error_number, ip.c_str());
//  return;

  controller.sendTelemetry("network", "softNetworkError", "ip", ip, "errno", error_number);
  controller.sendTelemetry("anomaly", "WARNING_NETWORK", "errno", error_number, "reason", "soft_network_error");

  printer_t * printer = _get_printer(ip);
  if (printer == NULL) {
    return;
  }

  printer->num_soft_network_errors += 1;
  log_v(2, "", "Encountered network error (%d) for %s, count: %d", error_number, ip.c_str(), printer->num_soft_network_errors);

  if (printer->num_soft_network_errors >= 5) {

    printer_enum_id += 1;
    controller.send_to_app(HP_MWP_BEGIN_PRINTER_CHANGES_MSG, -1, printer_enum_id);

    remove_printer(printer);

    controller.send_to_app(HP_MWP_END_PRINTER_ENUM_MSG, -1, printer_enum_id);
  }
}

void net_mobilewebprint::printer_list_t::network_error(string const & ip, int error_number)
{
  controller.sendTelemetry("network", "networkError", "ip", ip, "errno", error_number);
  controller.sendTelemetry("anomaly", "ERROR_NETWORK", "errno", error_number, "reason", "network_error");

  printer_t * printer = _get_printer(ip);
  if (printer == NULL) {
    return;
  }

  /* otherwise */
  printer->attrs_lc.insert(make_pair("status", "NETWORK_ERROR"));
  controller.job_stat(printer->connection_id, "status", "NETWORK_ERROR");
  controller.job_stat(printer->connection_id, "jobStatus", "NETWORK_ERROR");

}

void net_mobilewebprint::printer_list_t::upstream_error(string const & ip, int error_number)
{
  controller.sendTelemetry("network", "upstreamError", "ip", ip, "errno", error_number);

  printer_t * printer = _get_printer(ip);
  if (printer == NULL) {
    return;
  }

  log_d(1, "curl", "curl upstream error printer");

  /* otherwise */
  printer->attrs_lc.insert(make_pair("status", "UPSTREAM_ERROR"));
  controller.job_stat(printer->connection_id, "status", "UPSTREAM_ERROR");
  controller.job_stat(printer->connection_id, "jobStatus", "UPSTREAM_ERROR");

}

void net_mobilewebprint::printer_list_t::remove_printer(printer_t *& printer)
{
  if (printer && printer->has_ip()) {

    string ip = printer->ip;

    controller.send_to_app(HP_MWP_RM_PRINTER_MSG, -1, printer_enum_id, ip.c_str(), NULL, NULL);
    controller.sendTelemetry("printerScan", "removePrinter", "ip", ip);

    by_ip.erase(ip);
    mac_addresses.erase(ip);

    delete printer;
    printer = NULL;
  }
}

int net_mobilewebprint::printer_list_t::parse_network_change(string const & info, string & value, string & ssid, string & bssid)
{
  strvlist parts;
  int result = splitv(parts, info, '/');

  if (result > 0) {
    value = parts[0];
  } else {
    value = info;
  }

  if (result > 2) {
    bssid = parts[2];

    ssid = trim(parts[1], '"');
  }

  log_v(4, "", "---------- parse network change(%d): %s -> /%s/%s/%s/", result, info.c_str(), value.c_str(), ssid.c_str(), bssid.c_str());
  return result;
}

void net_mobilewebprint::printer_list_t::on_lost_wifi()
{
  log_v(2, "ttt", "lost wifi");

  remove_all_printers();

  // Reset various things that rely on the wifi -- TODO: any PCL downloads
  printer_list_in_flight = false;
}

void net_mobilewebprint::printer_list_t::on_gained_wifi()
{
  log_v(2, "ttt", "gained wifi: %s %s", controller.ssid.c_str(), controller.bssid.c_str());

  printer_enum_id += 1;
  controller.send_to_app(HP_MWP_BEGIN_NEW_PRINTER_LIST_MSG, -1, printer_enum_id);

  mq.send("re_scan_for_printers");
}

void net_mobilewebprint::printer_list_t::on_wifi_switch()
{
  log_v(2, "ttt", "switching networks: %s %s", controller.ssid.c_str(), controller.bssid.c_str());

  remove_all_printers();

  // Reset various things that rely on the wifi -- TODO: any PCL downloads
  printer_list_in_flight = false;

  printer_enum_id += 1;
  controller.send_to_app(HP_MWP_BEGIN_NEW_PRINTER_LIST_MSG, -1, printer_enum_id);

  mq.send("re_scan_for_printers");
}

void net_mobilewebprint::printer_list_t::remove_all_printers()
{
  printer_t * printer = NULL;

  // Remove all the printers in the list
  printer_enum_id += 1;
  controller.send_to_app(HP_MWP_BEGIN_PRINTER_CHANGES_MSG, -1, printer_enum_id);

  while(by_ip.size() > 0) {
    printer = by_ip.begin()->second;
    remove_printer(printer);
  }

  controller.send_to_app(HP_MWP_END_PRINTER_ENUM_MSG, -1, printer_enum_id);
}

bool net_mobilewebprint::printer_list_t::from_snmp(string ip, map<string, buffer_view_i const *> const & attrs)
{
  printer_t * printer = new printer_t(controller);
  if (printer->from_snmp(ip, attrs)) {
    return assimilate_printer_stats(printer);
  }

  return false;
}

bool net_mobilewebprint::printer_list_t::from_slp(slp_t & slp, buffer_view_i const & payload)
{
  printer_t * printer = new printer_t(controller);
  if (printer->from_slp(slp, payload)) {
    return assimilate_printer_stats(printer);
  }

  return false;
}

bool net_mobilewebprint::printer_list_t::assimilate_printer_stats(printer_t * printer)
{
  bool caused_change = false;

  if (printer == NULL) { return caused_change; }
  if (printer->has_ip() && _starts_with(printer->ip, "169.254")) {
    log_v("Ignoring: %s", printer->ip.c_str());
    return caused_change;
  }

  plist_t::const_iterator it;

  // For testing -- randomly remove MFG, so attr removal is sent
  //if (random() % 10 > 4) {
  //  printer->_1284_attrs.erase("MFG");
  //}

  if (printer->has_ip()) {

    strmap old_attrs;
    strmap new_attrs = printer->filtered_attrs(app_attribute_names);

    string ip = printer->ip;

    // We have the printer object that was created from the packet.  See
    // if we already have a printer for this packet
    if ((it = by_ip.find(printer->ip)) != by_ip.end()) {
      //if (printer->ip == "10.7.5.35") {
      //  log_d(1, "", "%p: *******Merging %s %s vs %s\n", this, printer->ip.c_str(), printer->status().c_str(), it->second->status().c_str());
      //}

      // The printer is already in our list
      old_attrs = it->second->filtered_attrs(app_attribute_names);

      it->second->merge(*printer);
      delete printer;

      printer = it->second;
      //if (printer->ip == "10.7.5.35") {
      //  log_d(1, "", "%p: *******Merged %s %s\n", this, printer->ip.c_str(), printer->status().c_str());
      //}
      new_attrs = printer->filtered_attrs(app_attribute_names);
    } else {

      // The printer was not in the list, insert it
      //if (printer->ip == "10.7.5.35") {
      //  log_d(1, "", "%p: *******Inserting %s %s\n", this, printer->ip.c_str(), printer->status().c_str());
      //}
      by_ip.insert(make_pair(printer->ip, printer));
    }

    bool have_sent_begin_msg = false;

    // Determine changed attributes and send to the app
    strmap::const_iterator old_smit, new_smit;
    for (old_smit = old_attrs.begin(); old_smit != old_attrs.end(); ++old_smit) {
      //if (printer->ip == "10.7.5.35" && old_smit->first == "status") {
      //  log_d(1, "", "old: %s: %s", old_smit->first.c_str(), old_smit->second.c_str());
      //}

      if ((new_smit = new_attrs.find(old_smit->first)) != new_attrs.end()) {
        //if (printer->ip == "10.7.5.35" && new_smit->first == "status") {
        //  log_d(1, "", "new: %s: %s", new_smit->first.c_str(), new_smit->second.c_str());
        //}
        if (old_smit->second == new_smit->second) {
          // Old and new versions are the same... move on to the next one.
          continue;
        }

        // Attribute exists in both, but are different
        string const & key    = new_smit->first;
        string const & value  = new_smit->second;

        caused_change = true;
        send_begin_msg(have_sent_begin_msg);
        controller.send_to_app(HP_MWP_PRINTER_ATTRIBUTE_MSG, -1, printer_enum_id, ip.c_str(), key.c_str(), value.c_str());
        scan_activity_happened();

        if (key == "status") {
          uint32 tick = get_tick_count();
          //log_v(1, "", "status change(%d/%3d): %3d.%2d %12s %s", printer->connection_id, by_ip.size(), tick/1000, tick % 1000, ip.c_str(), value.c_str());
          if (printer->connection_id != 0) {
            controller.job_stat(printer->connection_id, "status", value);
          }
        }
      } else {
        // Attribute exists in old, but not in new
        caused_change = true;
        send_begin_msg(have_sent_begin_msg);
        controller.send_to_app(HP_MWP_RM_PRINTER_ATTRIBUTE_MSG, -1, printer_enum_id, ip.c_str(), old_smit->first.c_str(), NULL);
        scan_activity_happened();
      }
    }

    for (new_smit = new_attrs.begin(); new_smit != new_attrs.end(); ++new_smit) {
      //if (printer->ip == "10.7.5.35" && new_smit->first == "status") {
      //  log_d(1, "", "newB: %s: %s", new_smit->first.c_str(), new_smit->second.c_str());
      //}
      if (old_attrs.find(new_smit->first) == old_attrs.end()) {
        //if (printer->ip == "10.7.5.35" && old_smit->first == "status") {
        //  log_d(1, "", "oldB: %s: %s", old_smit->first.c_str(), old_smit->second.c_str());
        //}
        // Attribute exists on new, not on old
        string const & key    = new_smit->first;
        string const & value  = new_smit->second;

        caused_change = true;
        send_begin_msg(have_sent_begin_msg);
        controller.send_to_app(HP_MWP_PRINTER_ATTRIBUTE_MSG, -1, printer_enum_id, ip.c_str(), key.c_str(), value.c_str());
        scan_activity_happened();

        if (key == "status") {
          uint32 tick = get_tick_count();

          //log_v(2, "", "new status(%d/%3d): %3d.%2d %12s %s", printer->connection_id, by_ip.size(), tick/1000, tick % 1000, ip.c_str(), value.c_str());
          if (printer->connection_id != 0) {
            controller.job_stat(printer->connection_id, "status", value);
          }
        }
      }
    }

    if (have_sent_begin_msg) {
      controller.send_to_app(HP_MWP_END_PRINTER_ENUM_MSG, -1, printer_enum_id);
    }

    //log_d("%s: %s", printer->ip.c_str(), printer->to_json(true).c_str());
  }

  if (printer->has_mac()) {
    by_mac.insert(make_pair(printer->mac, printer));
  }

  printer->request_updates();

  return caused_change;
}

bool net_mobilewebprint::printer_list_t::send_begin_msg(bool & have_sent_begin_msg)
{
  if (!have_sent_begin_msg) {
    printer_enum_id += 1;
    controller.send_to_app(HP_MWP_BEGIN_PRINTER_CHANGES_MSG, -1, printer_enum_id);
  }
  return (have_sent_begin_msg = true);
}

bool net_mobilewebprint::printer_list_t::cleanup()
{
  printer_t * printer = NULL;

  for (plist_t::const_iterator it = by_ip.begin(); it != by_ip.end(); ++it) {
    if ((printer = it->second) != NULL) {
      if (printer->is_missing()) {

        if (controller.flag("featurePrinterMissing", /* default= */ false)) {
          if (printer->connection_id != 0) {
            printer->attrs_lc.insert(make_pair("status", "NETWORK_ERROR"));
            controller.job_stat(printer->connection_id, "status", "NETWORK_ERROR");
            controller.job_stat(printer->connection_id, "jobStatus", "NETWORK_ERROR");
          }

          printer_enum_id += 1;
          controller.send_to_app(HP_MWP_BEGIN_PRINTER_CHANGES_MSG, -1, printer_enum_id);

          remove_printer(printer);

          controller.send_to_app(HP_MWP_END_PRINTER_ENUM_MSG, -1, printer_enum_id);
        }
      }
    }
  }

  return request_updates();
}

/**
 *  Interrogate the printers, and make requests to get additional information.
 */
bool net_mobilewebprint::printer_list_t::request_updates()
{
  // Rate-limit update requests
  uint32 now = get_tick_count();
  if (now - update_time < 1000) { return true; }
  update_time = now;

  bool result = false;
  printer_t * printer = NULL;

  plist_t::const_iterator it;
  for (it = by_ip.begin(); it != by_ip.end(); ++it) {
    if ((printer = it->second) != NULL) {
      if (printer->request_updates()) {
        result = true;
      }
    }
  }

  return result;
}

int net_mobilewebprint::printer_list_t::send_list_to_app()
{
  int result = 0;

  plist_t::const_iterator pit;
  strmap::const_iterator  attr_it;

  printer_enum_id += 1;
  controller.send_to_app(HP_MWP_BEGIN_NEW_PRINTER_LIST_MSG, -1, printer_enum_id);

  int score = 1000001;
  while (next_best_score(score)) {

    for (pit = by_ip.begin(); pit != by_ip.end(); ++pit) {
      printer_t * printer = pit->second;

      // On this iteration of the loop, only send printers with the now-current best score
      if (printer != NULL && printer->score() == score) {
        strmap attrs = printer->filtered_attrs(app_attribute_names);
        for (attr_it = attrs.begin(); attr_it != attrs.end(); ++attr_it) {
          result += 1;
          controller.send_to_app(HP_MWP_PRINTER_ATTRIBUTE_MSG, -1, printer_enum_id, printer->ip.c_str(), attr_it->first.c_str(), attr_it->second.c_str());
        }
      }
    }
  }

  //controller.send_to_app(HP_MWP_END_PRINTER_ENUM_MSG, -1, printer_enum_id);

  return result;
}

void net_mobilewebprint::printer_list_t::send_print_job(uint32 & connection_id, string const & ip)
{
  plist_t::iterator it = by_ip.find(ip);
  if (it == by_ip.end()) { return; }

  /* otherwise */
  printer_t * printer = it->second;
  printer->send_print_job(connection_id);
}

bool net_mobilewebprint::printer_list_t::next_best_score(int & score)
{
  int max = 0;

  plist_t::const_iterator pit;
  printer_t * printer = NULL;
  for (pit = by_ip.begin(); pit != by_ip.end(); ++pit) {
    if ((printer = pit->second) != NULL) {
      if (printer->score() < score && printer->score() > max) {
        max = printer->score();
      }
    }
  }

  // Were any found?
  if (max == 0) {
    score = -1;
    return false;
  }

  /* otherwise */
  score = max;
  return true;
}

std::string net_mobilewebprint::printer_list_t::get_device_id(string const & ip)
{
  plist_t::const_iterator it = by_ip.find(ip);
  if (it == by_ip.end()) { return ""; }

  /* otherwise */
  printer_t * const & printer = it->second;
  return printer->_1284_device_id;
}

int net_mobilewebprint::printer_list_t::port_for_proto(string const & ip, int udp_tcp, int proto)
{
  //printf("%p: 1 -- %s\n", this, ip.c_str());
  plist_t::const_iterator it = by_ip.find(ip);
  //printf("2\n");
  if (it == by_ip.end()) { return -1; }
  //printf("3\n");

  /* otherwise */
  printer_t * const & printer = it->second;
  //printf("4\n");
  return printer->port_for_proto(udp_tcp, proto);
}

std::string net_mobilewebprint::printer_list_t::to_json(bool for_debug)
{
  strlist printer_json_strs;

  printer_t * printer = NULL;

  plist_t::const_iterator it;
  for (it = by_ip.begin(); it != by_ip.end(); ++it) {
    if ((printer = it->second) != NULL) {
      printer_json_strs.push_back(printer->to_json(for_debug));
    }
  }

  string result = "{\"printers\":[";
  if (for_debug) { result += "\n"; }

  result += join(printer_json_strs, ",");

  result += "]}";

  return result;
}

int net_mobilewebprint::printer_list_t::make_server_json(serialization_json_t & json, char const * purpose)
{
  int count = 0;

  printer_t * printer = NULL;

  plist_t::const_iterator it;
  for (it = by_ip.begin(); it != by_ip.end(); ++it) {
    if ((printer = it->second) != NULL) {
      if (purpose != NULL) {
        if (printer->is_unknown(purpose) && !printer->is_unknown("deviceId")) {

          // Do not ask forever
          if (printer->num_is_supported_asks < 4) {
            printer->make_server_json(json.getObject(dashify_key(printer->ip)));
            count += 1;
          } else {
            printer->is_supported = new bool(false);
          }
        }
      } else {
        printer->make_server_json(json.getObject(dashify_key(printer->ip)), false);
        count += 1;
      }
    }
  }

  return count;
}

std::string net_mobilewebprint::printer_list_t::get_ip(string const & mac)
{
  printer_t * printer = NULL;

  for (plist_t::const_iterator it = by_ip.begin(); it != by_ip.end(); ++it) {
    if ((printer = it->second) != NULL) {
      if (printer->has_mac() && printer->mac == mac) {
        if (printer->has_ip()) {
          return printer->ip;
        }

        /* otherwise */
        return "";
      }
    }
  }

  return "";
}

std::string net_mobilewebprint::printer_list_t::get_pml_status(string const & status, bool & is_universal_status)
{
  is_universal_status = true;
  if (status == universal_status(3))    { return _upper(status); }
  if (status == universal_status(4))    { return _upper(status); }

  is_universal_status = false;
  return status;
}

net_mobilewebprint::fixup_snapshot_t::fixup_snapshot_t(printer_t const & printer)
  : ip(printer.ip), port(printer.port)
{
}

using namespace net_mobilewebprint;

static bool _is_hp(net_mobilewebprint::printer_t const & printer)
{
  if (_has(printer.attrs_lc, "mfg")) {
    if (eq(_lookup(printer.attrs_lc, "mfg"), "hp") || eq(_lookup(printer.attrs_lc, "mfg"), "hewlett-packard")) {
      return true;
    }
  }

  return false;
}

static bool _is_epson(net_mobilewebprint::printer_t const & printer)
{
  if (_has(printer.attrs_lc, "mfg")) {
    if (eq(_lookup(printer.attrs_lc, "mfg"), "epson")) {
      return true;
    }
  }

  return false;
}

static bool _is_brother(net_mobilewebprint::printer_t const & printer)
{
  if (_has(printer.attrs_lc, "mfg")) {
    if (eq(_lookup(printer.attrs_lc, "mfg"), "brother")) {
      return true;
    }
  }

  return false;
}

static bool _is_canon(net_mobilewebprint::printer_t const & printer)
{
  return (printer.port == 8611);
}

#if 0
static bool _is_snmp(net_mobilewebprint::printer_t const & printer)
{
  return _is_hp(printer) || _is_epson(printer);
}
#endif

static char       device_id_oid_num[] = "1.3.6.1.4.1.11.2.3.9.1.1.7.0";
static char epson_device_id_oid_num[] = "1.3.6.1.4.1.11.2.3.9.1.1.7.0";

// another universal? device_id?         1.3.6.1.2.1.25.3.2.1.3.1

static char const * _snmp_device_id_oid(net_mobilewebprint::printer_t const & printer)
{
  if (_is_hp(printer))    { return device_id_oid_num; }
  if (_is_epson(printer)) { return epson_device_id_oid_num; }

  // HPs and Epsons use it, but we might not know, yet of the model mfg
  if (printer.port != 8611) { return device_id_oid_num; }

  return NULL;
}

static char const * _snmp_status_oid(net_mobilewebprint::printer_t const & printer)
{
  // TODO: If we do not know the MFG, return NULL
  if (_is_hp(printer))      { return status_oid_num; }
  if (_is_epson(printer))   { return universal_status_oid_num; }
  if (_is_brother(printer)) { return universal_status_oid_num; }

  if (_is_canon(printer))   { return NULL; }

  //return universal_status_oid_num;
  return NULL;
}

static char const * universal_status_[] = {
  "unk-unknown",
  "other",
  "unknown",
  "idle",
  "printing",
  "warmup"
};

static char const * universal_status(int n)
{
  if (n < 0)                             { return universal_status_[0]; }
  if (n >= sizeof(universal_status_))    { return universal_status_[0]; }

  return universal_status_[n];
}

// See RFC 1514:
//
// hrPrinterStatus
//
// other(1)
// unknown(2)
// idle(3)
// printing(4)
// warmup(5)
//
//


