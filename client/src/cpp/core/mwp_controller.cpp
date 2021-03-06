
#include "mwp_core_config.h"
#include "hp_mwp.h"
#include "mwp_controller.hpp"
#include "mwp_slp.hpp"
#include "mwp_mdns.hpp"
#include "mwp_printer.hpp"
#include "mwp_mq.hpp"

#include <time.h>


#define HP_MWP_SEND_FULL_PRINTER_LIST     "send_full_printer_list"
#define HP_MWP_HARD_CODE_PRINTER          "add_printer"

#define PML_STATUS_IDLE                   "IDLE"
#define PML_STATUS_PRINTING               "PRINTING"
#define PML_STATUS_VERY_LOW_ON_INK        "VERY LOW ON INK"
#define PML_STATUS_CANCELLING             "CANCELING"

net_mobilewebprint::controller_base_t * net_mobilewebprint::g_controller = NULL;

using namespace net_mobilewebprint::msg;
using net_mobilewebprint::num_allocations;

std::string net_mobilewebprint::msg::up_and_running("up_and_running");
std::string net_mobilewebprint::msg::scan_for_printers("scan_for_printers");
std::string net_mobilewebprint::msg::mq_selected("mq_selected");
std::string net_mobilewebprint::msg::__select__("__select__");

char const * net_mobilewebprint::tags::TAG_DATA_FLOW = "DATA_FLOW";

const int net_mobilewebprint::max_poll_timeout = 10000;         // 10 sec
const float net_mobilewebprint::poll_timeout_ratio = 0.5;       // Increase by 150%





namespace net_mobilewebprint {
};





//MWP_MQ_HANDLER_HELPER(controller_base_t, send_job)
MWP_MQ_HANDLER_HELPER(controller_base_t, app_timeout_handler)

/**
 *  controller_base_t ctor.
 */
static int num_controllers = 0;
net_mobilewebprint::controller_base_t::controller_base_t(mwp_app_callback_t *)
  : slp(*this), mdns(*this), snmp(*this), bjnp(*this), job_flow(*this), mini_curl(*this, MWP_SERVER_NAME, MWP_SERVER_PORT), curl(*this, MWP_SERVER_NAME, MWP_SERVER_PORT),
    printers(*this), upstream(*this), unique_number(1000),
    client_start_in_flight_txn_id(0),
    mwp_app_callbacks_(NULL), sap_app_callbacks_(NULL),
    mwp_app_hf_callbacks(NULL), sap_app_hf_callbacks(NULL),
    mwp_app_bootstraps_(new mwp_app_cb_list_t()), sap_app_bootstraps_(new sap_app_cb_list_t()),
    mq_report(0, 1000),
    //alloc_report(0, 500),
    alloc_report(0, 5000),
    packet_stats_report(0, 5000),
    //send_printer_list_time(0), send_printer_list_interval(2000),
    upload_job_stats(0, 500),
    //job_stats_upload_time(0), job_stats_upload_interval(2000),
    cleanup_time(0, 250),
    server_command_timer(0, 10000, false),
    network_ifaces_timer(0, 2000),
    delayed_http_requests(new std::deque<controller_http_request_t>()),
    scan_start_time(0),
    telemetry_report(0, 15000),
    telemetry_send_time(0),
    heartbeat_timer(0, 20000)
{
  mwp_app_callbacks_   = new mwp_app_cb_list_t();
  mwp_app_hf_callbacks = new mwp_app_hf_cb_list_t();

  // I'm here!
  g_controller = this;
  log_d(1, "", "%d controllers", ++num_controllers);

  get_tick_count();     // Starts the clock
  set_flag("is_debug", MWP_IS_DEBUG);

  mq.on(this);
}

net_mobilewebprint::controller_base_t::controller_base_t(sap_app_callback_t *)
  : slp(*this), mdns(*this), snmp(*this), bjnp(*this), job_flow(*this), mini_curl(*this, MWP_SERVER_NAME, MWP_SERVER_PORT), curl(*this, MWP_SERVER_NAME, MWP_SERVER_PORT),
    printers(*this), upstream(*this), unique_number(1000),
    client_start_in_flight_txn_id(0),
    mwp_app_callbacks_(NULL), sap_app_callbacks_(NULL),
    mwp_app_hf_callbacks(NULL), sap_app_hf_callbacks(NULL),
    mwp_app_bootstraps_(new mwp_app_cb_list_t()), sap_app_bootstraps_(new sap_app_cb_list_t()),
    mq_report(0, 1000),
    //alloc_report(0, 500),
    alloc_report(0, 5000),
    packet_stats_report(0, 5000),
    //send_printer_list_time(0), send_printer_list_interval(2000),
    upload_job_stats(0, 500),
    cleanup_time(0, 250),
    server_command_timer(0, 10000, false),
    network_ifaces_timer(0, 2000),
    delayed_http_requests(new std::deque<controller_http_request_t>()),
    scan_start_time(0),
    telemetry_report(0, 15000),
    telemetry_send_time(0),
    heartbeat_timer(0, 20000)
{
  sap_app_callbacks_   = new sap_app_cb_list_t();
  sap_app_hf_callbacks = new sap_app_hf_cb_list_t();

  // I'm here!
  g_controller = this;
  log_d(1, "", "%d controllers", ++num_controllers);

  get_tick_count();     // Starts the clock
  set_flag("is_debug", MWP_IS_DEBUG);

  mq.on(this);
}

std::string net_mobilewebprint::controller_base_t::mod_name()
{
  return "controller_base_t";
}

std::string net_mobilewebprint::controller_base_t::clientId()
{
  string result = arg("clientid", "");
  if (result.length() == 0) { result = arg("deviceid",              ""); }
  if (result.length() == 0) { result = arg("hardwareid",            ""); }
  if (result.length() == 0) { result = arg("last_resort_client_id", ""); }

  return result;
}

/**
 *  controller_base_t destructor
 */
net_mobilewebprint::controller_base_t::~controller_base_t()
{
  g_controller = NULL;
}

net_mobilewebprint::mwp_app_cb_list_t &     net_mobilewebprint::controller_base_t::mwp_app_callbacks()
{
  mwp_assert(mwp_app_callbacks_);
  return *mwp_app_callbacks_;
}

net_mobilewebprint::sap_app_cb_list_t &     net_mobilewebprint::controller_base_t::sap_app_callbacks()
{
  mwp_assert(sap_app_callbacks_);
  return *sap_app_callbacks_;
}

//------------------------------------------------------------------------------------------------
//
// Messages
//
//------------------------------------------------------------------------------------------------

net_mobilewebprint::e_handle_result net_mobilewebprint::controller_base_t::on_select_loop_start(mq_select_loop_start_data_t const & loop_start_data)
{
  printers.on_select_loop_start(loop_start_data);
  if (cleanup_time.has_elapsed(loop_start_data.current_loop_start)) {
    printers.cleanup();
  }

  if (mwp_app_hf_callbacks) {
    for (mwp_app_hf_cb_list_t::const_iterator it = mwp_app_hf_callbacks->begin(); it != mwp_app_hf_callbacks->end(); ++it) {
      hp_mwp_hf_callback_t app_cb   = it->second.callback;
      void *               app_data = it->second.app_data;
      string const &       app_name = it->second.name;

      if (app_cb) {
        app_cb(app_data, app_name.c_str(), loop_start_data.current_loop_num, loop_start_data.current_loop_start, NULL);
      }
    }
  }

  if (sap_app_hf_callbacks) {
    for (sap_app_hf_cb_list_t::const_iterator it = sap_app_hf_callbacks->begin(); it != sap_app_hf_callbacks->end(); ++it) {
      hp_sap_hf_callback_t app_cb   = it->second.callback;
      void *               app_data = it->second.app_data;
      string const &       app_name = it->second.name;

      if (app_cb) {
        app_cb(app_data, app_name.c_str(), loop_start_data.current_loop_num, loop_start_data.current_loop_num, NULL);
      }
    }
  }

  if (delayed_http_requests != NULL) {
    if (client_start_in_flight_txn_id == -1) {
      // Flush all delayed jobs
      for (std::deque<controller_http_request_t>::const_iterator it = delayed_http_requests->begin(); it != delayed_http_requests->end(); ++it) {
        log_v(2, "", "Finally sending %d - %s |%s|", it->txn_id, it->verb.c_str(), it->url.c_str());
        curl_http_post(*it);
      }

      delete delayed_http_requests;
      delayed_http_requests = NULL;
    }
  }

  //log_d("report? %d %d %d", (int)mq_report_time, (int)mq_report_interval, (int)loop_start_data.current_loop_start);
  if (mq_report.has_elapsed(loop_start_data.current_loop_start)) {
    mq.report_and_restart_stats("");
  }

  if (server_command_timer.has_elapsed(loop_start_data.current_loop_start)) {
    serialization_json_t json;
    json.set("stackName", arg("stackName", ""));
    //string url = formats("netapp::/command/clientId-%s", clientId());
    string url = "netapp::/command";
    send_upstream("servercommand", url, json, new server_command_response_t());
  }

  if (network_ifaces_timer.has_elapsed(loop_start_data.current_loop_start)) {
    serialization_json_t json;
    if (read_arp_cache("/proc/net/arp", json)) {
      sendTelemetry("printerScan", "arpCache", json);
    }
  }

  // Push progress-telemetry up to the server
  if (upload_job_stats.has_elapsed(loop_start_data.current_loop_start)) {

    // Loop over job stats and upload any that are not complete (or have changed)
    for (map<uint32, stats_t>::const_iterator it = job_stats.begin(); it != job_stats.end(); ++it) {

      uint32              id = it->first;
      stats_t const &  stats = it->second;
      string          job_id = _lookup(stats.attrs, "job_id");

      // Are we already getting progress for this?
      if (_lookup(stats.bool_attrs, "getting_progress", false) == true) {
        //upload_job_stats.revert();
        continue;
      }

      bool need_to_upload    = false;
      if (_lookup(stats.bool_attrs, "done", false) == false || _lookup(stats.bool_attrs, "changed", true) == true) {
        need_to_upload = true;
      }

      if (need_to_upload) {
        stats_t data;
        data.int_attrs["txn_id"] = id;

        serialization_json_t json;
        stats.make_server_json(json);

        log_v(4, "", "/poll/getJobProgress -- %d: |%s| |%s|", (int)id, job_id.c_str(), json.stringify().c_str());

        job_stat(id, "getting_progress", true, true);
        upstream.send(string("/poll/getJobProgress/") + job_id, json, "_progress_response", data);
        job_stat_changed(id, false, "reset");
      }
    }
  }

  if (alloc_report.has_elapsed(loop_start_data.current_loop_start)) {
    log_v(4, "controller_t", "MQ-length: %4d; Allocations: %d; Chunks: %d; Buffers: %d; buffer-mems: %d; buffer-mem: %d", mq.mq_normal.size(), num_allocations, num_chunk_allocations, num_buffer_allocations, num_buf_bytes_allocations, num_buf_bytes);
  }

  if (heartbeat_timer.has_elapsed(loop_start_data.current_loop_start)) {
    log_v(3, "", "mwp %s", "up");
  }

  if (telemetry_report.has_elapsed(loop_start_data.current_loop_start)) {

    serialization_json_t json;

    // See if there are any telemetry items to send
    int     count = 0;
    strset  sent;
    for (map<string, jsonlist>::const_iterator it = bucketData.begin(); it != bucketData.end(); ++it) {
      string const & bucketName = it->first;
      map<string, serialization_json_t>::const_iterator itMap;
      if ((itMap = buckets.find(bucketName)) != buckets.end()) {

        serialization_json_t const &     bucket = itMap->second;
        jsonlist const &                   data = it->second;

        log_v(4, "", "----------------- sending bucket %s %d", bucketName.c_str(), data.size());

        serialization_json_t & subjson = json.getObject(bucketName);
        subjson << bucket;

        serialization_json_list_t & sublist = subjson.getList("items");
        for (jsonlist::const_iterator itItem = data.begin(); itItem != data.end(); ++itItem) {
          sublist.push_back(*itItem);
          sent.insert(bucketName);

          count += 1;
        }
      }
    }

    //log_v(2, "", "Sending telemetry? size: %d", sent.size());
    if (sent.size() > 0) {
      if (flag("telemetry", TELEMETRY_DEFAULT)) {

        telemetry_send_time = get_tick_count();

        string pathname = string("/telemetry?count=") + mwp_itoa(count) + "&uptime=" + mwp_itoa(loop_start_data.current_loop_start) /* + "&clientId=" + clientId() */;
        send_upstream("telemetry", pathname, json, new telemetry_response_t(*this));
      }

      // We have sent it, now reset the data
      for (strset::const_iterator it = sent.begin(); it != sent.end(); ++it) {
        string const & bucketName = *it;
        bucketData.erase(bucketName);
      }
    }
  }

  //// Log packet stats
  //if (packet_stats_report.has_elapsed(loop_start_data.current_loop_start)) {
  //  for (map<string, stats_t>::const_iterator it = packet_stats.begin(); it != packet_stats.end(); ++it) {
  //    log_d("controller_t", it->first);
  //    log_d("controller_t", it->second.to_json());
  //  }
  //}

  return not_impl;
}

net_mobilewebprint::e_handle_result net_mobilewebprint::controller_base_t::on_select_loop_idle(mq_select_loop_idle_data_t const & data)
{
  return printers.on_select_loop_idle(data);
}

net_mobilewebprint::e_handle_result net_mobilewebprint::controller_base_t::handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  printers.handle(name, payload, data, extra);

  // Dont care
  if (name == mq_selected) { return unhandled; }
  if (name.substr(0, 8) == "_on_raw_") { return unhandled; }

  log_v("Controller handling %s", name.c_str());

  if (name == HP_MWP_SEND_FULL_PRINTER_LIST)  { printers.send_list_to_app(); return handled; }
  if (name == "_allocate_job_id")             { return _allocate_job_id(name, payload, data, extra); }
  if (name == "_progress_response")           { return _on_progress_response(name, payload, data, extra); }
  if (name == "_send_job")                    { return _send_job(name, payload, data, extra); }

  if (name.substr(0, 7) == "__app__")         { return app_handler(name, extra.id, extra.txn_id, payload, data); }
  if (name == up_and_running)                 { return _up_and_running(name, payload, data, extra); }
  if (name == "_on_http_headers")             { return _on_http_headers(name, payload, data, extra); }
  if (name == "_on_http_payload")             { return _on_http_payload(name, payload, data, extra); }
  if (name == "_on_txn_close")                { return _on_txn_close(name, payload, data, extra); }

  if (name == "_upstream_response")           { return process_upstream_response(name, payload, data, extra); }

  if (_starts_with(name, "telemetry/")) {

    string bucket   = "jBucket";
    string event    = "jEvent";
    strvlist parts  = splitv(name, '/');

    if (parts.size() >= 2) { bucket   = parts[1]; }
    if (parts.size() >= 3) { event    = parts[2]; }

    byte const * p = payload.const_begin();
    string value = payload.read_string(p);
    sendTelemetryJson(bucket, event, value);

    return handled;
  }

  return not_impl;
}

/**
 *  Start up.
 *
 *  Starts the MWP module.
 *
 *  @param[in]  start_scanning  MWP will start scanning the network for printer immediately.
 *  @param[in]  block           MWP will not return.
 *
 *  MWP needs to run in its own non-UI thread.  The host has a few choices on how to give MWP
 *  this thread.  It can call this start function in a thread that the host has already allocated.
 *  Alternatively, the host can implement start_thread.  Similarly, the host can use it's own
 *  run-loop to keep things alive, or it can leverage MWP's.  Calling start with block set to true
 *  will use MWP run-loop.
 *
 *  Apps (mobile apps or similar) usually have their own run-loops, so would set block to false.
 *  Depending on the platform, it may be easier to call start in its own thread (Android), or to
 *  implement start_thread (POSIX-like) [start_thread is very similar to the various POSIX thread
 *  creation functions.]
 */
bool net_mobilewebprint::controller_base_t::start(bool start_scanning, bool block)
{
  // The app wants to control the verbosity?
  if (!flag("app_verbosity", false)) {
    set_flag("log_api", true);
    set_flag(MWP_VERBOSITY, true);
  }

  // Seed the PRNG
  uint32 tm = 0;
  uint32 hash = tm = (int)time(NULL);
  char const * p = clientId().c_str();
  for (;*p; ++p) {
    hash = *p + ((hash << 9) | (hash >> (32 - 9)));
  }
  srandom(hash);

  log_v(2, "", "Seeding: %d, %d", tm, hash);

  // Set any options
  set_arg("last_resort_client_id", (string("FFFFFFFF")+random_string(56)).c_str());

  // Do platform specific setup of options
  platform_bootstrap();
  interop_bootstrap();

  // Do registered bootstrapping
  if (mwp_app_bootstraps_) {
    for (mwp_app_cb_list_t::const_iterator it = mwp_app_bootstraps_->begin(); it != mwp_app_bootstraps_->end(); ++it) {
      hp_mwp_callback_t app_cb   = it->second.callback;
      void *            app_data = it->second.app_data;

      //printf("-------------------------------------------- here i am about to call a mwp bootstrap\n");
      if (app_cb) {
        app_cb(app_data, "bootstrap", 0, 0, NULL, NULL);
      }
    }
  }
  delete mwp_app_bootstraps_;
  mwp_app_bootstraps_ = NULL;

  if (sap_app_bootstraps_) {
    for (sap_app_cb_list_t::const_iterator it = sap_app_bootstraps_->begin(); it != sap_app_bootstraps_->end(); ++it) {
      hp_sap_callback_t app_cb   = it->second.callback;
      void *            app_data = it->second.app_data;

      //printf("-------------------------------------------- here i am about to call a sap bootstrap\n");
      if (app_cb) {
        app_cb(app_data, "bootstrap", 0, 0, NULL, NULL);
      }
    }
  }
  delete sap_app_bootstraps_;
  sap_app_bootstraps_ = NULL;

  serialization_json_t json;
  if (read_arp_cache("/proc/net/arp", json)) {
    sendTelemetry("printerScan", "arpCache0", json);
  }

  log_api("start(scan=%d, block=%d)", start_scanning, block);
  show_options();

  bool result = true;
  result = result && mq.run();

  result = mq.send(up_and_running) && result;
  if (start_scanning) {

    scan_start_time = get_tick_count();
    sessionId       = random_string(64);

    startBucket("printerScan", scan_start_time);

    result = mq.send(scan_for_printers) && result;
  }

  if (!block) {
    log_d("Not blocking in startup");
    return result;
  }

  /* otherwise, wait for mq to finish */
  while (!mq.is_done()) {
    interruptable_sleep(1000);
  }

  return result;
}

void net_mobilewebprint::controller_base_t::show_options()
{

  log_v(3, "", "Starting; options:");
  for (strmap::const_iterator it = ARGS.args.begin(); it != ARGS.args.end(); ++it) {
    log_v(3, "", "    %s: |%s|", it->first.c_str(), it->second.c_str());
  }
  log_v(3, "", "            flags:");
  for (set<string>::const_iterator it = ARGS.flags.begin(); it != ARGS.flags.end(); ++it) {
    log_v(3, "", "    %s", (*it).c_str());
  }
  for (set<string>::const_iterator it = ARGS.antiFlags.begin(); it != ARGS.antiFlags.end(); ++it) {
    log_v(3, "", "   !%s", (*it).c_str());
  }

}

net_mobilewebprint::e_handle_result net_mobilewebprint::controller_base_t::_up_and_running(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  log_d(1, "controller_t", "Controller is up and running");

  serialization_json_t json;
  client_start_in_flight_txn_id = _make_http_post("/clientStart", D("clientId", clientId(), "v", BUILD_NUMBER_STR), json);
  return handled;
}

uint32 net_mobilewebprint::controller_base_t::curl_http_post(string const & url, serialization_json_t & json)
{
  uint32 txn_id = _unique();

  return curl_http_post(url, json, txn_id);
}

uint32 net_mobilewebprint::controller_base_t::curl_http_post(string const & url, string const & body, string content_type)
{
  uint32 txn_id = _unique();

  return curl_http_post(url, body, content_type, txn_id);
}

uint32 net_mobilewebprint::controller_base_t::curl_http_get(string const & url)
{
  uint32 txn_id = _unique();

  curl_http_get(url, txn_id);
  return txn_id;
}

uint32 net_mobilewebprint::controller_base_t::curl_http_post(string const & url, serialization_json_t & json, uint32 txn_id)
{
  //log_v(4, "controller_t", "Controller POSTING to %s", url.c_str());

  // If we are still resolving /clientStart, then just remember this request
  if (delayed_http_requests == NULL) {
    log_v(4, "", "Not Queueing request %d - %s |%s|", txn_id, "POST", url.c_str());
    return curl_http_post(controller_http_request_t(txn_id, "POST", url, json));
  }

  /* otherwise */
  log_v(4, "", "Queueing request %d - %s |%s|", txn_id, "POST", url.c_str());
  delayed_http_requests->push_back(controller_http_request_t(txn_id, "POST", url, json));
  return txn_id;
}

uint32 net_mobilewebprint::controller_base_t::curl_http_post(string const & url, string const & body, string content_type, uint32 txn_id)
{
  //log_v(4, "controller_t", "Controller POSTING to %s", url.c_str());

  // If we are still resolving /clientStart, then just remember this request
  if (delayed_http_requests == NULL) {
    log_v(4, "", "Not Queueing request %d - %s |%s|", txn_id, "POST", url.c_str());
    return curl_http_post(controller_http_request_t(txn_id, "POST", url, body, content_type));
  }

  /* otherwise */
  log_v(4, "", "Queueing request %d - %s |%s|", txn_id, "POST", url.c_str());
  delayed_http_requests->push_back(controller_http_request_t(txn_id, "POST", url, body, content_type));
  return txn_id;
}

uint32 net_mobilewebprint::controller_base_t::curl_local_send(string ip, int port, string path, string const & body, string content_type, string verb)
{
  curl.fetch_local(ip, port, path, body, content_type, _unique(), verb);
}

uint32 net_mobilewebprint::controller_base_t::curl_http_post(controller_http_request_t const & request)
{
  if (request.body.length() == 0) {
    serialization_json_t json(request.json_body);

    json.set("clientId", clientId());
    json.set("sessionId", sessionId);

    // TODO: this does not belong on every POST, move it to those that make sense
    json.set("provider", arg("providerName", "HP_CP"));

    json.set("meta.platform",   platform_name());
    json.set("meta.version",    MAJOR_VERSION);
    json.set("meta.build",      BUILD_NUMBER);
    json.set("meta.sha1",       SOURCE_GIT_SHA1);
    json.set("meta.sha1_short", SOURCE_GIT_SHA1_SHORT);
    json.set("meta.branch",     SOURCE_GIT_BRANCH_NAME);
    json.set("meta.dataFormat", DATA_FORMAT);

    if (arg("stackName", "").length() > 0) {
      json.set("meta.stackName",  arg("stackName"));
    }

    if (arg("username", "").length() > 0) {
      json.set("meta.username", arg("username", "noname@example.com"));
    }

    if (flag("zdebug", false)) {
      json.set("meta.zdebug", true);
    }

    // TODO: Remove this after netapp_command.js recognizes ".clientId" on stg/prod
    if (request.url == "netapp::/command") {
      json.set("meta.deviceid", clientId());
    }

    log_vs(5, "controller_t", "Controller POSTING to (%s_%s) %s", curl.server_name, request.url, json.stringify());

    if (curl.post_mwp_server(json, request.url, request.txn_id) == NULL) {
      mini_curl.post_mwp_server(json, request.url, request.txn_id);
    }
    return request.txn_id;
  }

  /* otherwise */
  if (curl.post_mwp_server(request.body, request.url, request.content_type, request.txn_id) == NULL) {
    //mini_curl.post_mwp_server(request.body, request.url, request.txn_id);
  }
  return request.txn_id;
}

uint32 net_mobilewebprint::controller_base_t::curl_http_get(string const & url, uint32 txn_id)
{
  log_v(3, "controller_t", "Controller GETTING from %s", url.c_str());

  if (curl.fetch_from_mwp_server("GET", url, txn_id) == NULL) {
    mini_curl.fetch_from_mwp_server("GET", url, txn_id);
  }

  return txn_id;
}

std::string net_mobilewebprint::controller_base_t::send_upstream(string const & mod_name, string const & endpoint, serialization_json_t & json, upstream_handler_t * handler)
{
  string stream_name = upstream.send(mod_name, endpoint, json);

  upstream_messages.insert(make_pair(stream_name, handler));

  return stream_name;
}

std::string net_mobilewebprint::controller_base_t::send_local(string ip, int port, string path, string const & body, string content_type, upstream_handler_t * handler)
{
  string stream_name = upstream.send_local(ip, port, path, body, content_type, "localsend", stats_t());

  upstream_messages.insert(make_pair(stream_name, handler));

  return stream_name;
}

std::string net_mobilewebprint::controller_base_t::send_upstream(string const & mod_name, string const & endpoint, serialization_json_t & json)
{
  return upstream.send(mod_name, endpoint, json);
}

uint32 net_mobilewebprint::controller_base_t::_make_http_post(char const * url, strmap const & query, serialization_json_t & json)
{
  uint32 txn_id = _unique();
  chunkses.insert(make_pair(txn_id, deque<chunk_t*>()));

#if 0
  string url(url_);

  string search;
  for (strmap::const_iterator it = query.begin(); it != query.end(); ++it) {
    if (search.length() > 0) {
      search += "&";
    }

    search += it->first + "=" + it->second;
  }

  if (search.length() > 0) {
    url = url + "?" + search;
  }
#endif

  //log_v(2, "controller_t", "Controller trying to POST to %s", url);
  return curl_http_post(controller_http_request_t(txn_id, "POST", url, query, json));
}

net_mobilewebprint::e_handle_result net_mobilewebprint::controller_base_t::_on_http_headers(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  if (chunkses.find(extra.txn_id) == chunkses.end()) { return unhandled; }
  //log_d(1, "", "controller took http headers(%d): %d bytes", extra.txn_id, (int)payload.dsize());

  return handled;
}

net_mobilewebprint::e_handle_result net_mobilewebprint::controller_base_t::_on_http_payload(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  if (chunkses.find(extra.txn_id) == chunkses.end()) { return unhandled; }

  //log_d(1, "controller", "controller took http payload(%d): %d bytes", extra.txn_id, (int)payload.dsize());
  chunkses[extra.txn_id].push_back(new chunk_t(data, payload)); /**/ num_chunk_allocations += 1;

  return handled_and_took_message;    // We now have ownership of the data memory
}

net_mobilewebprint::e_handle_result net_mobilewebprint::controller_base_t::_on_txn_close(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  if (chunkses.find(extra.txn_id) == chunkses.end()) { return unhandled; }

  string json_str = join(chunkses[extra.txn_id], "");
  log_v(3, "controller_t", "_on_txn_close body: |%s|", json_str.c_str());

  json_t json;
  JSON_parse(json, json_str);

  if (extra.txn_id == client_start_in_flight_txn_id) {

    strset str_keys, int_keys, bool_keys;
    if (json.keys(str_keys, int_keys, bool_keys)) {

      for (strset::const_iterator it = str_keys.begin(); it != str_keys.end(); ++it) {
        string parent, key = *it;
        if (_normalize_keys(parent, key) && parent == "preference") {
          set_arg(key, json.lookup(*it));
        }
      }

      for (strset::const_iterator it = int_keys.begin(); it != int_keys.end(); ++it) {
        string parent, key = *it;
        if (_normalize_keys(parent, key) && parent == "preference") {
          set_arg(key, json.lookup_int(*it));
        }
      }

      for (strset::const_iterator it = bool_keys.begin(); it != bool_keys.end(); ++it) {
        string parent, key = *it;

        if (_normalize_keys(parent, key) && parent == "preference") {
          set_flag(key, json.lookup_bool(*it));
        }
      }
    }

    if (json.lookup_bool("ok") || json.lookup_bool("final")) {

      // singleNodeName means to stop looking, and just use it
      if (json.has("pclHost.singleNodeName")) {
        curl.server_name      = json.lookup("pclHost.pclServerName");

        set_arg("pcl.servername", json.lookup("pclHost.singleNodeName"));

      // pclServerName means to ask that server permission
      } else if (json.has("pclHost.pclServerName")) {

        if (json.lookup_bool("final")) {
          curl.server_name      = json.lookup("pclHost.pclServerName");

        } else {

          string new_server_name = json.lookup("pclHost.pclServerName");

          if (curl.server_name != new_server_name) {
            curl.server_name = new_server_name;
            serialization_json_t json;
            client_start_in_flight_txn_id = _make_http_post("/clientStart", D("clientId", clientId(), "v", BUILD_NUMBER_STR), json);
            return handled;
          }
        }
      }

    } else {

      // We got a response from the server, but not an OK.  Depending on things, we may try another server, or we may just use
      // this server, anyway.
      string new_server_name;

      // dev --> qa
      if (curl.server_name.find("dev.mobiledevprint.net") != string::npos) {
        // We got rejected from the dev server, switch to the qa server
        if (curl.server_name[0] == 'h' && curl.server_name[1] == 'q') {
          new_server_name = "hqqa.mobiledevprint.net";
        } else {
          new_server_name = "qa.mobiledevprint.net";
        }
        set_arg("domainName", ".mobiledevprint.net");

      // qa --> stg
      } else if (curl.server_name.find("qa.mobiledevprint.net") != string::npos) {
        if (curl.server_name[0] == 'h' && curl.server_name[1] == 'q') {
          new_server_name = "hqstg.mobiledevprint.net";
        } else {
          new_server_name = "stg.mobiledevprint.net";
        }
        set_arg("domainName", ".mobiledevprint.net");

//      // stg --> prod
//      } else if (curl.server_name.find("stg.mobiledevprint.net") != string::npos) {
//        if (curl.server_name[0] == 'h' && curl.server_name[1] == 'q') {
//          new_server_name = "hqpub.mobilewebprint.net";
//        } else {
//          new_server_name = "pub.mobilewebprint.net";
//        }
//        set_arg("domainName", ".mobilewebprint.net");

      }

      if (new_server_name.length() > 0) {
        curl.server_name = new_server_name;
        serialization_json_t json;
        client_start_in_flight_txn_id = _make_http_post("/clientStart", D("clientId", clientId(), "v", BUILD_NUMBER_STR), json);
        return handled;
      }
    }

    log_d(1, "controller_t", "Using %s as upstream server", curl.server_name.c_str());

    set_arg("upstreamServer",     curl.server_name);
    set_arg("fallbackServiceOne", curl.server_name);
    set_arg("fallbackServiceTwo", curl.server_name);

    set_arg("upstreamServerEx",   replace(curl.server_name, ".mobile", "ex.mobile"));

    size_t end_of_stack_name = curl.server_name.find_first_of("-.");
    if (end_of_stack_name != string::npos) {
      ARGS.set_arg("stackName", curl.server_name.substr(0, end_of_stack_name));
    }

//    string stack_name = arg("stackName", "");
//    if (stack_name[0] == 'h' && stack_name[1] == 'q') {
//      stack_name = stack_name.substr(2);
//    }
//    set_arg("stackName", stack_name);

    show_options();

    client_start_in_flight_txn_id = -1;

    server_command_timer.trigger();

    return handled;
  }

  return handled;
}

net_mobilewebprint::e_handle_result net_mobilewebprint::controller_base_t::process_upstream_response(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  int http_resp_code = 0;
  string http_version;
  strmap headers;
  string body;
  json_t json;
  json_array_t json_array;
  stats_t stats;
  string content_type;

  content_type = upstream.parse_response(payload, http_resp_code, http_version, headers, body, json, json_array, stats);
  log_vs(4, "", "upstream response type: |%s|, version: |%s|", content_type, http_version);
  if (content_type.length() > 0) {

    upstream_handler_map_t::iterator it = upstream_messages.find(extra.txn_name);
    if (it != upstream_messages.end()) {

      log_v(2, "controller_t", "processing upstream response for: %s (%d)", extra.txn_name.c_str(), http_resp_code);

      upstream_handler_t * p = it->second;
      p->handle(http_resp_code, http_version, headers, body, json, json_array, stats);
      delete p;
    } else {
      log_w("controller_t", "processing upstream response. CANNOT_FIND_HANDLER");
    }
  } else {
    log_w("controller_t", "processing upstream response. PARSE_FAILED type: |%s|, version: |%s|", content_type.c_str(), http_version.c_str());
    log_vs(2, "", "PARSE_FAIL body: |%s|", body);
  }

  return handled;
}

net_mobilewebprint::server_command_response_t::~server_command_response_t()
{
}

void net_mobilewebprint::server_command_response_t::handle(int code, std::string const & http_version, strmap const & headers, string const & body, json_t const & json, json_array_t const & json_array, stats_t const & stats_out)
{
  log_v(3, "", "-----------------------------------------------------------response from handleServerCommandRequest %d", code);
  if (g_controller) {
    g_controller->handle_server_command(code, http_version, headers, body, json, json_array, stats_out);
  }
}

net_mobilewebprint::http_reverse_upload_t::http_reverse_upload_t(controller_base_t & controller_)
  : controller(controller_)
{
}

net_mobilewebprint::http_reverse_upload_t::~http_reverse_upload_t()
{
}

void net_mobilewebprint::http_reverse_upload_t::handle(int code, std::string const & http_version, strmap const & headers, string const & body, json_t const & json, json_array_t const & json_array, stats_t const & stats_out)
{
  log_v(2, "", "-----------------------------------------------------------rup %d", code);

  // TODO or octet stream?
  string content_type = _lookup(headers, "context-type", "text/plain");

  string stream_name = controller.upstream.send("/fetchLocalResponse", body, content_type, "_fetch_local_response_resp", stats_t());
}

void net_mobilewebprint::controller_base_t::handle_server_command(int code, std::string const & http_version, strmap const & headers, string const & body, json_t const & json, json_array_t const & json_array, stats_t const & stats_out)
{
  bool next_has_been_scheduled = false;

  if (code < 400) {
    for (int i = 0; json_array.has(i); ++i) {
      json_t const * item = json_array.get(i);

      if (flag("featureDumpCommand", /* default= */ false)) {
        log_v(4, "", "++++++++++++++++++++++++++++++++++++++++++++++++++++++ handle_server_command");
        item->dump(true);
      }

      // Print to a mac address
      if (item->has("$print.mac") && item->has("$print.url")) {
        string const & mac      = item->lookup("$print.mac");
        string const & url      = item->lookup("$print.url");

        send_job(url, mac);

      // Print to an IP address
      } else if (item->has("$print.ip") && item->has("$print.url")) {
        string const & ip       = item->lookup("$print.ip");
        string const & url      = item->lookup("$print.url");

        send_job(url, ip);

      // Retry a command in 'in' msec
      } else if (item->has_int("$retry.in")) {
        server_command_timer.time = get_tick_count() + item->lookup_int("$retry.in");
        next_has_been_scheduled = true;

      } else if (item->has("$addprinter.ip") && item->has_int("$addprinter.port") && item->has("$addprinter.deviceId")) {

        string const & ip         = item->lookup("$addprinter.ip");
        string const & device_id  = item->lookup("$addprinter.deviceId");
        int            port       = item->lookup_int("$addprinter.port");

        log_v(2, "", "adding printer: %s %d %s", ip.c_str(), port, device_id.c_str());

        buffer_t * msg = mq.message("_on_slp_payload");
        msg->append_strs_sans_null("(mwp-sender=", ip.c_str(), ")");
        msg->append_strs_sans_null("(mwp-port=", mwp_itoa(port).c_str(), ")");
        msg->append_strs_sans_null("(x-hp-p1=MFG:HP;MDL:", device_id.c_str(), ";)");
        mq.send(msg);

      // Halt the app
      } else if (item->has_int("$halt.in")) {
        if (item->lookup_int("$halt.in") == 73) {
          string halt_msg = item->lookup("$halt.msg");
          if (halt_msg.length() == 0) {
            halt_msg = "succa";
          }

          send_to_app("Halt_Mario", -1, 0, halt_msg.c_str(), NULL, NULL, NULL, NULL, item->lookup("$halt.exit", 0));
        }

      // Fetch HTTP from local
      } else if (item->has("$fetch.ip")) {

        string  ip   = item->lookup("$fetch.ip");
        int     port = item->lookup("$fetch.port", 80);
        string  path = item->lookup("$fetch.path", "/");

        // TODO: handle $fetch.json
        if (item->has("$fetch.body")) {

          string verb           = item->lookup("$fetch.verb", "POST");
          string content_type   = item->lookup("$fetch.content_type", "text/plain");

          curl.fetch_local(ip, port, path, item->lookup("$fetch.body"), content_type, _unique(), verb);
          send_local(ip, port, path, item->lookup("$fetch.body"), content_type, new http_reverse_upload_t(*this));

        } else {
          curl.get_local(ip, port, path, _unique());
        }
      }
    }

    // Ask for another command
    if (!next_has_been_scheduled) {
      server_command_timer.trigger();
      next_has_been_scheduled = true;
    }
    return;
  }

  /* otherwise - wait before the next one */
  log_v(3, "", "++++++++++++++++++++++++++++++++++++++++++++++++++++++ >= 400 (%d), handle_server_command, next_has_been_scheduled? %d", code, (int)next_has_been_scheduled);
  if (!next_has_been_scheduled) {
    if (code != 403) {
      server_command_timer.trigger();
      next_has_been_scheduled = true;
    }
  }
}

// -----------------------------------------------------------------------------------------------------------------------------------------
//                                                                       Telemetry
// -----------------------------------------------------------------------------------------------------------------------------------------
void net_mobilewebprint::controller_base_t::sendTelemetry(string bucketName, char const * eventType, serialization_json_t const & data_)
{
  //log_v(2, "", "sendTelemetry(%s)", eventType);

  bool telemetry_preference = flag("telemetry", TELEMETRY_DEFAULT);

  uint32 now = get_tick_count();
  if (now - scan_start_time > 5 * 60 * 1000) {
    if (bucketName == "printerScan") { return; }
  }

  startBucket(bucketName, now);

  serialization_json_t data(data_);
  data.set("eventTime", (int)now);
  data.set("eventType", eventType);
  log_vs(5 + (telemetry_preference ? 0 : -1), "", "accumulating telemetry to send: %s: %s", bucketName, data.stringify());
  bucketData[bucketName].push_back(data);
}

void net_mobilewebprint::controller_base_t::sendTelemetry(string bucketName, char const * eventType)
{
  serialization_json_t json;
  sendTelemetry(bucketName, eventType, json);
}

void net_mobilewebprint::controller_base_t::sendTelemetryJson(string bucketName, string eventType, string const & json_str)
{
  // TODO: Be able to send pre-json to the server as JSON
  sendTelemetry(bucketName.c_str(), eventType.c_str(), "preJson", json_str);
}

void net_mobilewebprint::controller_base_t::startBucket(string bucketName, uint32 start_time /* = 0 */)
{
  if (start_time == 0) {
    start_time = get_tick_count();
  }

  // Insert a list for the data, if necessary
  if (bucketData.find(bucketName) == bucketData.end()) {
    bucketData.insert(make_pair(bucketName, jsonlist()));
  }

  // Insert a map for the bucket attributes, if necessary
  if (buckets.find(bucketName) != buckets.end()) {
    return;
  }

  /* otherwise */
  buckets.insert(make_pair(bucketName, serialization_json_t()));

  serialization_json_t & common = buckets[bucketName];

  common.set("startTime", (int)start_time);
  common.set("bucket", bucketName);
  common.set("bucketId", random_string(32));
}

net_mobilewebprint::telemetry_response_t::telemetry_response_t(controller_base_t & controller_)
  : controller(controller_)
{
}

net_mobilewebprint::telemetry_response_t::~telemetry_response_t()
{
}

void net_mobilewebprint::telemetry_response_t::handle(int code, std::string const & http_version, strmap const & headers, string const & body, json_t const & json, json_array_t const & json_array, stats_t const & stats_out)
{
  uint32 latency = _time_since(g_controller->telemetry_send_time);

  log_v(2, "", "telemetry response: %d, latency: %d", code, latency);

  controller.sendTelemetry("telemetry", "serverResponse", "code", code, "latency", latency);

  if (latency > 1200) {
    controller.sendTelemetry("anomaly", latency > 3000 ? "ERROR_LATENCY" : "WARNING_LATENCY", "latency", latency, "path", "/telemetry");
  }
  return;
}

bool net_mobilewebprint::controller_base_t::mq_is_done()
{
  return mq.is_done();
}

/**
 *  Puts a message into the MQ to start the ball rolling on a job.
 *
 *  The message is to allocate a job id.
 */
bool net_mobilewebprint::controller_base_t::send_job(string const & asset_url_, string const & printer_ip_)
{
  string printer_ip = printer_ip_;
  string asset_url  = asset_url_;

  log_api("send_job(asset_url=%s, ip=%s)", asset_url.c_str(), printer_ip.c_str());

  // Remove the launcher sessionId
  string launchSessionId;
  size_t tildePos = asset_url.find("~");
  if (tildePos != string::npos) {
    asset_url = asset_url.substr(0, tildePos);
    launchSessionId = asset_url_.substr(tildePos + 1);
    sendTelemetry("session", "launch", "launchSessionId", launchSessionId, "sessionId", sessionId);
  }

  // An empty printer_ip is a heads-up of what the URL will be (the print asset). We can get a head-start.
  if (printer_ip_.length() == 0) {

    // Pre-allocate job
    stats_t stats("txn_id", _unique());

    serialization_json_t json;
    json.set("launchSessionId", launchSessionId);
    json.set("assetUrl", asset_url);

    string stream_name = upstream.send("/preAllocateJobId", json, "_pre_alloced_job", stats);

    return true;
  }

  /* otherwise */

  // Convert mac address into IP
  if (printer_ip.length() == 17) {
    printer_ip = printers.get_ip(printer_ip);
  }

  log_api("No, really I mean send_job(asset_url=%s, ip=%s)", asset_url.c_str(), printer_ip.c_str());

  uint32 txn_id    = _unique();
  string device_id = replace_chars(printers.get_device_id(printer_ip), " ", "+");

  buffer_t * msg = mq.message("_allocate_job_id", 0, txn_id);
  msg->appendT(asset_url);
  msg->appendT(printer_ip);

  return mq.send(msg);
}

/**
 *
 */
bool net_mobilewebprint::controller_base_t::print(string const & asset_url)
{
  return false;
}

/**
 *  Sends an HTTP request to /allocateJobId, to get a job id.
 *
 *
 */
net_mobilewebprint::e_handle_result net_mobilewebprint::controller_base_t::_allocate_job_id(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  uint32 http_txn_id = extra.txn_id;
  buffer_view_i::const_iterator p = payload.const_begin();

  string asset_url = payload.read_string(p);
  string ip        = payload.read_string(p);

  log_v(2, "controller_t", "controller::_send_job  %d to: %s |%s|", http_txn_id, ip.c_str(), asset_url.c_str());

  job_stats[http_txn_id].attrs["ip"]         = ip;
  job_stats[http_txn_id].attrs["asset_url"]  = asset_url;
  job_stats[http_txn_id].attrs["jobStatus"]  = STATUS_WAITING0;

  serialization_json_t json;
  stats_t stats("txn_id", http_txn_id);
  string stream_name = upstream.send("/allocateJobId", json, "_send_job", stats);

  return handled;
}

net_mobilewebprint::e_handle_result net_mobilewebprint::controller_base_t::_send_job(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  int http_resp_code = 0;
  json_t json;
  stats_t stats;
  if (upstream.parse_response(payload, http_resp_code, json, stats)) {

    uint32 txn_id     = stats.int_attrs["txn_id"];
    string job_id     = job_stats[txn_id].attrs["job_id"] = json.lookup("jobId");


    log_v(3, "", "controller::allocated: |%s|", job_id.c_str());
    log_v(4, "", "controller::allocated: |%s| |%s|", name.c_str(), extra.txn_name.c_str());
    log_v(3, "", "controller::allocated: %s", json.stringify().c_str());

    strvlist parts = compact(splitv(extra.txn_name, '/'));
    if (parts.size() >= 2) {

      string ip         = job_stats[txn_id].attrs["ip"];
      string asset_url  = job_stats[txn_id].attrs["asset_url"];

      string device_id  = replace_chars(printers.get_device_id(ip), " ", "+");
      string url        = string("/pcl/jobId-") + job_id + "/typeofprint-unknown/" + _lower(device_id) + "/" + replace(asset_url, "://", "/");

      log_v(2, "controller_t", "controller::_send_job  %d to: %s jobId: %s\n\t|%s|", txn_id, ip.c_str(), job_id.c_str(), url.c_str());

      printers.send_print_job(txn_id, ip);
      if (curl.fetch_from_mwp_server("GET", url, txn_id) == NULL) {
        mini_curl.fetch_from_mwp_server("GET", url, txn_id);
      }
    }
  }

  return handled;
}

net_mobilewebprint::e_handle_result net_mobilewebprint::controller_base_t::_on_progress_response(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  int http_resp_code = 0;
  json_t json;
  stats_t stats, http_stats;
  uint32 pcl_txn_id = 0;

  bool parse_response_success = upstream.parse_response(payload, http_resp_code, json, stats);

  string json_str = json.stringify();
  log_v(4, "", "_on_progress_response resp: |%s|", json_str.c_str());

  // Note: we are dealing with legacy code, and the names aren't that great -- there is
  // confusion between the "status" of the job, and the "current" "state" of the printer.
  // Ideally, those are the names: "jobStatus" and "printerState";  however, a ton of code
  // and mind-share wants the the printerState to be called "status".

  //set defaults
  string state          = "x";
  uint32 numerator      = 25;
  uint32 denominator    = 200;
  string message        = "x";
  string printerState   = "x";
  string job_id         = "x";
  string jobStatus      = "x";

  // Get the id of the "real" transaction -- the PCL transaction
  if (_has(stats.int_attrs, "txn_id")) {
    pcl_txn_id = _lookup(stats.int_attrs, "txn_id", (int)pcl_txn_id);

    http_stats = job_stats[pcl_txn_id];
    log_v(4, "", "_on_progress_response http_job_stats: |%s|", http_stats.debug_to_json().c_str());
  }

  if(parse_response_success){
    //do json lookup...
    state = json.lookup("current");
    numerator = json.lookup("numerator", 25);
    denominator = json.lookup("denominator", 200);

    //save into job_stats
    job_stat(pcl_txn_id, "state", state.c_str());
    job_stat(pcl_txn_id, "numerator", (int)numerator);
    job_stat(pcl_txn_id, "denominator", (int)denominator);
  } else{
    state = _lookup(http_stats.attrs, "state", state);
    numerator    = _lookup(http_stats.int_attrs, "numerator", (int)numerator);
    denominator  = _lookup(http_stats.int_attrs, "denominator", (int)denominator);
  }

  // Grab the job ID, and send the message to the app
  job_id = http_stats.attrs["job_id"];

  // Lookup printer status ("state") and job status
  if ((printerState   = http_stats.attrs["status"]) == "")    { printerState = ""; }
  if ((jobStatus      = http_stats.attrs["jobStatus"]) == "") { jobStatus    = "y"; }

  // Fixup status -- we might have gotten it from the "universal" SNMP status
  bool is_universal_status = false;
  printerState = printer_list_t::get_pml_status(printerState, is_universal_status);

  int totalSent = -1;
  if (_has(http_stats.int_attrs, "totalSent")) {
    totalSent = _lookup(http_stats.int_attrs, "totalSent", 0);
  }

  int num_downloaded = -1;
  if (_has(http_stats.int_attrs, "numDownloaded")) {
    num_downloaded = _lookup(http_stats.int_attrs, "numDownloaded", 0);
  }

  bool is_done = false;
  if (_has(http_stats.bool_attrs, "byte_stream_done")) {
    is_done = _lookup(http_stats.bool_attrs, "byte_stream_done", false);
  }

  bool is_error_condition = false;
  bool is_finishing       = false;

  if (state == "NETWORK_ERROR" || printerState == "NETWORK_ERROR" || jobStatus == "NETWORK_ERROR") {

    printerState = job_stat(pcl_txn_id, "status", "NETWORK_ERROR");
    jobStatus = job_stat(pcl_txn_id, "jobStatus", "NETWORK_ERROR");
    state = "NETWORK_ERROR";
    message = HP_MWP_MESSAGE_NETWORK_ERROR;
    is_error_condition = true;

  } else if (state == "UPSTREAM_ERROR" || printerState == "UPSTREAM_ERROR" || jobStatus == "UPSTREAM_ERROR"){

    printerState = job_stat(pcl_txn_id, "status", "UPSTREAM_ERROR");
    jobStatus = job_stat(pcl_txn_id, "jobStatus", "UPSTREAM_ERROR");
    state = "UPSTREAM_ERROR";
    message = HP_MWP_MESSAGE_UPSTREAM_ERROR;
    is_error_condition = true;
  }

  // Handle error, or continue with the job
  if (is_error_condition) {
    is_finishing = true;

    // Get a better message, if possible
    message = _lookup(http_stats.attrs, "result_json", message);
    if (_has(http_stats.attrs, "result_json")) {

      json_t json;
      if (JSON_parse(json, message)) {
        if      (json.has_string("error_message"))  { message = json.lookup("error_message"); }
        else if (json.has_string("message"))        { message = json.lookup("message"); }
        else if (json.has_string("msg"))            { message = json.lookup("msg"); }
        //else if (json.has_string("error"))          { message = json.lookup("error"); }
        //else if (json.has_string("err"))            { message = json.lookup("err"); }
      }
    }

  } else {

    log_v(5, "", "progress0(%04d): |%s| printerState: |%s|, jobStatus: |%s| totalSent: %d/%d, all-sent: %s", pcl_txn_id, job_id.c_str(), printerState.c_str(), jobStatus.c_str(), totalSent, num_downloaded, is_done ? "true" : "false");

    // If the printer does not support getting its status, simulate it (IDLE -> PRINTING -> IDLE)
    if (printerState.length() == 0) {

      // First, assume IDLE -- this is the 'safe' choice.
      printerState = PML_STATUS_IDLE;

      // Then, see if the connection to the printer is closed
      if (_has(http_stats.bool_attrs, "byte_stream_done") && _lookup(http_stats.bool_attrs, "byte_stream_done", false) == true) {
        printerState = PML_STATUS_IDLE;

      // Then, see if we have sent any bytes to the printer
      } else if (_has(http_stats.int_attrs, "totalSent") && _lookup(http_stats.int_attrs, "totalSent", 0) > 0) {
        printerState = PML_STATUS_PRINTING;
      }

    }
    log_v(4, "", "progress1(%04d): |%s| printerState: |%s|, jobStatus: |%s| totalSent: %d/%d, all-sent: %s", pcl_txn_id, job_id.c_str(), printerState.c_str(), jobStatus.c_str(), totalSent, num_downloaded, is_done ? "true" : "false");

    // ---------- Messaging
    // Determine the message to show to the user.  There are two phases to this.  The simpler statuses (while
    // the printer status is something obvious, like "printing", just show that to the user (making it more
    // pretty), however, that is lower in the code.
    //
    // However, the various states must be tracked, to determine the "real" job status as the print progresses,
    // so that is done first.



    // Track the complex state of the print -- This also sets a message, that might be clobbered below.
    if (jobStatus == STATUS_WAITING0) {
      message = HP_MWP_MESSAGE_FORMATTING_JOB;
      if (_has(http_stats.int_attrs, "numDownloaded") && _lookup(http_stats.int_attrs, "numDownloaded", 0) > 0) {
        jobStatus = job_stat(pcl_txn_id, "jobStatus", STATUS_WAITING1);
      }
    }

    if (jobStatus == STATUS_WAITING1) {
      message = HP_MWP_MESSAGE_WAITING_START;
    }

    if (jobStatus == STATUS_PRINTING) {
      if (printerState != PML_STATUS_PRINTING && printerState != PML_STATUS_IDLE) {
        message = HP_MWP_MESSAGE_WAITING_RESUME;
        jobStatus = job_stat(pcl_txn_id, "jobStatus", STATUS_WAITING2);
      }
    }

    if (jobStatus == STATUS_WAITING2) {
      message = HP_MWP_MESSAGE_WAITING_RESUME;
      if (printerState == PML_STATUS_PRINTING) {
        jobStatus = job_stat(pcl_txn_id, "jobStatus", STATUS_PRINTING);
      }
    }


    // Track the simpler states -- maybe overriding the "message" from above
    if (printerState == PML_STATUS_PRINTING) {
      jobStatus = job_stat(pcl_txn_id, "jobStatus", STATUS_PRINTING);
      message = HP_MWP_MESSAGE_PRINTING;

      // If the entire byte stream has been sent, tell the user we are waiting for finish
      if (_has(http_stats.bool_attrs, "byte_stream_done") && _lookup(http_stats.bool_attrs, "byte_stream_done", false) != false) {
        message = HP_MWP_MESSAGE_FINISHING;
      }
    } else if (_starts_with(printerState, PML_STATUS_CANCELLING)) {
      jobStatus = job_stat(pcl_txn_id, "jobStatus", STATUS_CANCELLING);
      message = HP_MWP_MESSAGE_CANCELLING;
    }


    if(printers._get_printer(http_stats.attrs["ip"])->is_epson()){
      epson_progress_handle(printerState, http_stats, pcl_txn_id, message, jobStatus, is_finishing);
    } else {
      other_progress_handle(printerState, http_stats, pcl_txn_id, message, jobStatus, is_finishing);
    }

  }

  // As long as we are not done, send status
  state = jobStatus;
  if (!_lookup_job_stat(pcl_txn_id, "frozen", false)) {

    log_v(4, "", "progress(%04d):  |%s| printerState: |%s|, jobStatus: |%s| totalSent: %d/%d, all-sent: %s, state: |%s|, message: |%s|", pcl_txn_id, job_id.c_str(), printerState.c_str(), jobStatus.c_str(), totalSent, num_downloaded, is_done ? "true" : "false", state.c_str(), message.c_str());
    send_to_app(HP_MWP_PRINT_PROGRESS_MSG, -1, 0, state.c_str(), message.c_str(), printerState.c_str(), job_id.c_str(), jobStatus.c_str(), (int)numerator, (int)denominator);

  } else {
    log_v(4, "", "progress(%04d): |%s| ------------- frozen; not sending: printerState: |%s|, jobStatus: |%s| totalSent: %d/%d, all-sent: %s, state: |%s|, message: |%s|", pcl_txn_id, job_id.c_str(), printerState.c_str(), jobStatus.c_str(), totalSent, num_downloaded, is_done ? "true" : "false", state.c_str(), message.c_str());
  }

  job_stat(pcl_txn_id, "getting_progress", false, true);

  if (is_finishing) {
    job_stat(pcl_txn_id, "done", true);
    job_stat(pcl_txn_id, "frozen", true);
  }

  return handled;
}

void net_mobilewebprint::controller_base_t::epson_progress_handle(string printerState, stats_t http_stats, uint32 pcl_txn_id, string & message, string & jobStatus, bool & is_finishing)
{  
    if (printerState != PML_STATUS_IDLE) {
      if(http_stats.bool_attrs["has_started"] && _lookup(http_stats.attrs, "previous_state") == PML_STATUS_IDLE && printerState == "other"){
        int currTick = (int)get_tick_count();
        sendTelemetry("network", "EPSON_PRINT", "epson_idle_to_other", currTick - _lookup(http_stats.int_attrs, "idle_starting_tick_count", currTick));
        log_d(1, "controller_t", "sending %d", currTick - _lookup(http_stats.int_attrs, "idle_starting_tick_count", currTick));
      }
      job_stat(pcl_txn_id, "has_started", true);
      job_stat(pcl_txn_id, "idle_starting_tick_count", (int)get_tick_count());
    } else if(http_stats.bool_attrs["has_started"]){
      int idleTickCount = _lookup(http_stats.int_attrs, "idle_starting_tick_count", -1);
      job_stat(pcl_txn_id, "previous_state", printerState);

      if(_time_since(idleTickCount) <= 10000){
        message = HP_MWP_MESSAGE_FINISHING;
        log_d(1, "printer", "printing has not finished sending... TICK COUNT: %d", _time_since(idleTickCount)/1000);
      } else {
        message = HP_MWP_MESSAGE_DONE;
        if (http_stats.attrs["jobStatus"] == STATUS_CANCELLING) {
          jobStatus = job_stat(pcl_txn_id, "jobStatus", STATUS_CANCELLED);
        } else {
          jobStatus = job_stat(pcl_txn_id, "jobStatus", STATUS_SUCCESS);
        }
        is_finishing = true;
      }
    }
}

void net_mobilewebprint::controller_base_t::other_progress_handle(string printerState, stats_t http_stats, uint32 pcl_txn_id, string & message, string & jobStatus, bool & is_finishing)
{  
    if (printerState != PML_STATUS_IDLE) {
      job_stat(pcl_txn_id, "has_started", true);
    } else if(http_stats.bool_attrs["has_started"]){
      message = HP_MWP_MESSAGE_DONE;
      if (http_stats.attrs["jobStatus"] == STATUS_CANCELLING) {
        jobStatus = job_stat(pcl_txn_id, "jobStatus", STATUS_CANCELLED);
      } else {
        jobStatus = job_stat(pcl_txn_id, "jobStatus", STATUS_SUCCESS);
      }
      is_finishing = true;
    }
}

//------------------------------------------------------------------------------------------------
//
// Options to control MWP run-time behavior.
//
//------------------------------------------------------------------------------------------------

/**
 *  Get the named option, if not present, returns the null string: ""
 */
std::string const & net_mobilewebprint::controller_base_t::arg(char const * key)
{
  return ARGS[key];
}

/**
 *  Get the named option, if not present, use the default.
 */
std::string const & net_mobilewebprint::controller_base_t::arg(char const * key, string const & def)
{
  return ARGS.get(key, def);
}

int net_mobilewebprint::controller_base_t::arg(char const * key, int def)
{
  string value = ARGS.get(key, "");
  if (value.length() == 0) { return def; }

  /* otherwise */
  return mwp_atoi(value);
}

/**
 *  Get the named flag (true or false), if not present, returns false.
 */
bool net_mobilewebprint::controller_base_t::flag(char const * key)
{
  return ARGS.get_flag(key);
}

/**
 *  Get the named flag (true or false), if not present, returns the default.
 */
bool net_mobilewebprint::controller_base_t::flag(char const * key, bool def)
{
  return ARGS.get_flag(key, def);
}

net_mobilewebprint::controller_base_t & net_mobilewebprint::controller_base_t::set_arg(string const & name, string const & value)
{
  return set_arg(name.c_str(), value.c_str());
}

net_mobilewebprint::controller_base_t & net_mobilewebprint::controller_base_t::set_arg(char const *name, char const *value)
{
  log_v(2, "", "setOption(%s, \"%s\")", name, value);

  // Some keys do not set an ARGS
  if (::strcmp(name, HP_MWP_HARD_CODE_PRINTER) == 0) {
    strlist ip_and_deviceid = split(value, ';');

    strlist::const_iterator it = ip_and_deviceid.begin();
    if (it == ip_and_deviceid.end()) { return *this; }

    string ip = *it;
    if (++it == ip_and_deviceid.end()) { return *this; }

    string port = *it;
    if (++it == ip_and_deviceid.end()) { return *this; }

    string device_id = *it;

    buffer_t * msg = mq.message("_on_raw_payload");
    msg->append_strs_sans_null("(mwp-sender=", ip.c_str(), ")");
    msg->append_strs_sans_null("(mwp-port=", port.c_str(), ")");
    msg->append_strs_sans_null("(x-hp-p1=MFG:HP;MDL:", device_id.c_str(), ";)");
    mq.send(msg);
    return *this;
  } else if (::strcmp(name, "serverName") == 0 || ::strcmp(name, "stackName") == 0) {
    string svalue(value);
    string stack_name;

    if (svalue.find("pub") != string::npos)           { stack_name = "pub"; curl.server_name = "hqpub.mobilewebprint.net"; }
    else if (svalue.find("prod") != string::npos)     { stack_name = "pub"; curl.server_name = "hqpub.mobilewebprint.net"; }
    else if (svalue.find("dev") != string::npos)      { stack_name = "dev"; curl.server_name = "hqdev.mobiledevprint.net"; }
    else if (svalue.find("qa") != string::npos)       { stack_name =  "qa"; curl.server_name =  "hqqa.mobiledevprint.net"; }
    else if (svalue.find("stg") != string::npos)      { stack_name = "ext"; curl.server_name = "hqext.mobiledevprint.net"; }
    else if (svalue.find("ext") != string::npos)      { stack_name = "ext"; curl.server_name = "hqext.mobiledevprint.net"; }
    else                                              { stack_name = "pub"; curl.server_name = "hqpub.mobilewebprint.net"; }

    ARGS.set_arg("requestedStackName", value);
    ARGS.set_arg("stackName", stack_name);
    return *this;
  }

  /* otherwise */
  ARGS.set_arg(name, value);
  return *this;
}

net_mobilewebprint::controller_base_t & net_mobilewebprint::controller_base_t::set_arg(string const & name, int value)
{
  return set_arg(name.c_str(), value);
}

net_mobilewebprint::controller_base_t & net_mobilewebprint::controller_base_t::set_arg(char const *name, int value)
{
  log_v(2, "", "setIntOption(%s, %d)", name, value);
  ARGS.set_arg(name, mwp_itoa(value));
  return *this;
}

net_mobilewebprint::controller_base_t & net_mobilewebprint::controller_base_t::set_flag(string const & name, bool value)
{
  return set_flag(name.c_str(), value);
}

net_mobilewebprint::controller_base_t & net_mobilewebprint::controller_base_t::set_flag(char const *name, bool value)
{
  log_v(2, "", "setFlag(%s, %s)", name, value ? "true" : "false");
  ARGS.set_flag(name, value);

  // Flags that are known to affect others
  if (::strcmp("log_api", name) == 0) {
    set_flag("quiet", false);
  }

  return *this;
}

net_mobilewebprint::controller_base_t & net_mobilewebprint::controller_base_t::clear_flag(char const *name)
{
  log_v(2, "", "setFlag(%s)", name);
  return set_flag(name, false);
}

net_mobilewebprint::controller_base_t & net_mobilewebprint::controller_base_t::parse_cli(int argc, void const * argv[])
{
  ARGS.merge(args_t(argc, argv));

  // Must special-process some args
  if (_has(ARGS.args, "stackName")) {
    set_arg("stackName", _lookup(ARGS.args, "stackName"));
  }

  if (_has(ARGS.args, "serverName")) {
    set_arg("serverName", _lookup(ARGS.args, "serverName"));
  }

  return *this;
}

/**
 *  Free function to get an option for the key.
 *
 *  @return a std::string for the option, or the null string ("") if the
 *  key is not present.
 */
std::string const & net_mobilewebprint::get_option(char const * key)
{
  if (g_controller == NULL) { return args_t::none; }

  /* otherwise */
  return g_controller->arg(key);
}

/**
 *  Free function to get an option for the key.
 *
 *  @return a std::string for the option, or the default if the key
 *  is not present.
 */
std::string const & net_mobilewebprint::get_option(char const * key, string const & def)
{
  if (g_controller == NULL) { return def; }

  /* otherwise */
  return g_controller->arg(key, def);
}

/**
 *  Free function to get an option for the key.
 *
 *  @return an int for the option, or the default if the key
 *  is not present.
 */
int net_mobilewebprint::get_option(char const * key, int def)
{
  if (g_controller == NULL) { return def; }

  /* otherwise */
  return g_controller->arg(key, def);
}

/**
*  Free function to get a flag (true or false) for the key.
*
*  @return a bool for the key (false if the key is not present.)
*/
bool net_mobilewebprint::get_flag(string const & key)
{
  if (g_controller == NULL) { return false; }

  /* otherwise */
  bool result = g_controller->flag(key.c_str());
  return result;
}

/**
*  Free function to get a flag (true or false) for the key.
*
*  @return a bool for the key (false if the key is not present.)
*/
bool net_mobilewebprint::get_flag_def(string const & key, bool def)
{
  if (g_controller == NULL) { return def; }

  /* otherwise */
  bool result = g_controller->flag(key.c_str(), def);
  return result;
}

//------------------------------------------------------------------------------------------------
//
// Messages to the app
//
//------------------------------------------------------------------------------------------------

net_mobilewebprint::e_handle_result net_mobilewebprint::controller_base_t::app_handler(string const & name, int id, int txn_id, buffer_view_i const & payload, buffer_t * data)
{
  send_to_app(name.substr(7), id, txn_id, payload, data);
  return handled;
}

net_mobilewebprint::e_handle_result net_mobilewebprint::controller_base_t::_app_timeout_handler(string const & name, int id, int txn_id, buffer_view_i const & payload, buffer_t * data)
{
  send_to_app(timer_table[id], id, txn_id, payload, data);
  return handled;
}

int net_mobilewebprint::controller_base_t::app_send(char const * name_, char const * payload)
{
  string name("__app__");
  name += name_;

  if (payload == NULL) {
    return mq.send(name);
  }

  /* otherwise */
  return mq.send(name, payload);
}

int net_mobilewebprint::controller_base_t::app_set_timeout(char const * message_to_send, int msecs_to_wait)
{
  handler_holder_by_id_t *  handler = NULL;
  int timer_id = mq.setTimeout(handler, "app", app_timeout_handler, this, msecs_to_wait, /*need_lock=*/true);

  timer_table[timer_id] = message_to_send;

  return timer_id;
}

/**
 *  Register a bootstrap function (callback) for an app (called once, when the app is loaded).
 *
 *  @param[in]  name      The name of the handler.
 *  @param[in]  app_data  Any data that the app wants MWP to send with each message.  Usually a this pointer.
 *  @param[in]  callback  The function to be called.
 */
bool net_mobilewebprint::controller_base_t::register_bootstrap(char const * name, void * app_data_, hp_mwp_callback_t callback)
{
  if (!mwp_app_bootstraps_) { return false; }

  printf("-------------------------------------------- here i am receiving a mwp bootstrap fn\n");

  (*mwp_app_bootstraps_)[name] = mwp_app_callback_t(name, app_data_, callback);

  bool result = false;
//  result = send_to_app("recd_register_bootstrap", 1, 1, (uint8 const *)name, (mwp_params*)NULL) != 0;
  return result;
}

/**
 *  Register a bootstrap function (callback) for an app (called once, when the app is loaded).
 *
 *  @param[in]  name      The name of the handler.
 *  @param[in]  app_data  Any data that the app wants MWP to send with each message.  Usually a this pointer.
 *  @param[in]  callback  The function to be called.
 */
bool net_mobilewebprint::controller_base_t::register_bootstrap(char const * name, void * app_data_, hp_sap_callback_t callback)
{
  if (!sap_app_bootstraps_) { return false; }

  /* otherwise */

  printf("-------------------------------------------- here i am receiving a sap bootstrap fn\n");
  (*sap_app_bootstraps_)[name] = sap_app_callback_t(name, app_data_, callback);

  bool result = false;
//  result = send_to_app("recd_register_bootstrap", 1, 1, (uint8 const *)name, (sap_params*)NULL) != 0;
  return result;
}

/**
 *  Register a handler (callback) for an app.
 *
 *  @param[in]  name      The name of the handler.
 *  @param[in]  app_data  Any data that the app wants MWP to send with each message.  Usually a this pointer.
 *  @param[in]  callback  The function to be called.
 */
bool net_mobilewebprint::controller_base_t::register_handler(char const * name, void * app_data_, hp_mwp_callback_t callback)
{
  mwp_app_callbacks()[name] = mwp_app_callback_t(name, app_data_, callback);

  bool result = false;
  result = send_to_app("recd_register_handler", 1, 1, (uint8 const *)name, (mwp_params*)NULL) != 0;
  return result;
}

/**
 *  Register a handler (callback) for an app.
 *
 *  @param[in]  name      The name of the handler.
 *  @param[in]  app_data  Any data that the app wants MWP to send with each message.  Usually a this pointer.
 *  @param[in]  callback  The function to be called.
 */
bool net_mobilewebprint::controller_base_t::register_handler(char const * name, void * app_data_, hp_sap_callback_t callback)
{
  sap_app_callbacks()[name] = sap_app_callback_t(name, app_data_, callback);

  bool result = false;
  result = send_to_app("recd_register_handler", 1, 1, (uint8 const *)name, (sap_params*)NULL) != 0;
  return result;
}

/**
 *  Deregister a handler (callback) for an app.
 *
 *  @param[in]  name      The name of the handler.
 */
bool net_mobilewebprint::controller_base_t::deregister_handler(char const * name)
{
  mwp_app_callbacks().erase(name);
  sap_app_callbacks().erase(name);

  bool result = false;
  if (mwp_app_callbacks_ != NULL) {
    result = send_to_app("recd_deregister_handler", 1, 1, (uint8 const *)name, (mwp_params const *)NULL) != 0;
  } else {
    result = send_to_app("recd_deregister_handler", 1, 1, (uint8 const *)name, (sap_params const *)NULL) != 0;
  }
  return result;
}

/**
 *  Register a handler (callback) for an app.
 *
 */
bool net_mobilewebprint::controller_base_t::register_hf_handler(char const * name, void * app_data_, hp_mwp_hf_callback_t callback)
{
  (*mwp_app_hf_callbacks)[name] = mwp_app_hf_callback_t(name, app_data_, callback);

  bool result = false;
  result = send_to_app("recd_register_hf_handler", 1, 1, (uint8 const *)name, (mwp_params*)NULL) != 0;
  return result;
}

/**
 *  Register a handler (callback) for an app.
 *
 */
bool net_mobilewebprint::controller_base_t::register_hf_handler(char const * name, void * app_data_, hp_sap_hf_callback_t callback, bool)
{
  (*sap_app_hf_callbacks)[name] = sap_app_hf_callback_t(name, app_data_, callback);

  bool result = false;
  result = send_to_app("recd_register_hf_handler", 1, 1, (uint8 const *)name, (sap_params*)NULL) != 0;
  return result;
}

/**
 *  Send a message to the app.
 *
 *  At the most basic level, MWP communicates with the app through two mechanisms.  Communication from
 *  the app to MWP travel through the core_api_t object.  Communication from MWP to the app travel
 *  through this send_to_app function.
 *
 *  @param[in]  message           The message type or category (not the payload/data) of the message.
 *  @param[in]  id                An id of the message / handler pair.  For example, set_timeout requires an id to identify which timer is happening.
 *  @param[in]  transaction_id    If the semantics of a message have the notion of "which one", an identifier should be sent in this transaction_id.
 *  @param[in]  p1                Payload data pointer.
 *  @param[in]  params            Extra payload data.  May be NULL.
 */
int net_mobilewebprint::controller_base_t::send_to_app(char const * message, int id, int32 transaction_id, uint8 const * p1, mwp_params const * params_)
{
  mwp_params local_params = {};
  mwp_params const * params = (params_ != NULL ? params_ : &local_params);

  // Dont log the logging commands
  if (!_starts_with(message, "log_")) {
    log_vt(2, "MWP_Message", "msg-to-app: %25s (%3d,%3d) %12s -- %20s -- %20s  (%s|%s)", message, id, transaction_id, or_blank(p1), or_blank(params->p2), or_blank(params->p3), or_blank(params->p4), or_blank(params->p5));
  }

  int result = 0;
  for (mwp_app_cb_list_t::const_iterator it = mwp_app_callbacks().begin(); it != mwp_app_callbacks().end(); ++it) {
    hp_mwp_callback_t app_cb   = it->second.callback;
    void *            app_data = it->second.app_data;

    if (app_cb) {
      result = app_cb(app_data, message, id, transaction_id, p1, params);
    }
  }

  return result;
}

/**
 *  Send a message to the app.
 *
 *  At the most basic level, MWP communicates with the app through two mechanisms.  Communication from
 *  the app to MWP travel through the core_api_t object.  Communication from MWP to the app travel
 *  through this send_to_app function.
 *
 *  @param[in]  message           The message type or category (not the payload/data) of the message.
 *  @param[in]  id                An id of the message / handler pair.  For example, set_timeout requires an id to identify which timer is happening.
 *  @param[in]  transaction_id    If the semantics of a message have the notion of "which one", an identifier should be sent in this transaction_id.
 *  @param[in]  p1                Payload data pointer.
 *  @param[in]  params            Extra payload data.  May be NULL.
 */
int net_mobilewebprint::controller_base_t::send_to_app(char const * message, int id, int32 transaction_id, uint8 const * p1, sap_params const * params_)
{
  sap_params local_params = {};
  sap_params const * params = (params_ != NULL ? params_ : &local_params);

  // Dont log the logging commands
  if (!_starts_with(message, "log_")) {
    log_vt(2, "MWP_Message", "msg-to-app: %25s (%3d,%3d) %12s -- %20s -- %20s  (%s|%s)", message, id, transaction_id, or_blank(p1), or_blank(params->p2), or_blank(params->p3), or_blank(params->p4), or_blank(params->p5));
  }

  int result = 0;
  for (sap_app_cb_list_t::const_iterator it = sap_app_callbacks().begin(); it != sap_app_callbacks().end(); ++it) {
    hp_sap_callback_t app_cb   = it->second.callback;
    void *            app_data = it->second.app_data;

    if (app_cb) {
      result = app_cb(app_data, message, id, transaction_id, p1, params);
    }
  }

  return result;
}

/**
 *  Send a message to the app.
 */
int net_mobilewebprint::controller_base_t::send_to_app(string const & name, int id, int32 transaction_id,
                                                       char const * p1, char const * p2, char const * p3, char const * p4, char const * p5,
                                                       uint32 n1, uint32 n2, uint32 n3, uint32 n4, uint32 n5)
{
  if (mwp_app_callbacks_ != NULL) {
    mwp_params params = {0};
    params.p2 = (uint8 const *)p2;
    params.p3 = (uint8 const *)p3;
    params.p4 = (uint8 const *)p4;
    params.p5 = (uint8 const *)p5;
    params.n1 = n1;
    params.n2 = n2;
    params.n3 = n3;
    params.n4 = n4;
    params.n5 = n5;
    return send_to_app(name.c_str(), id, transaction_id, (uint8 const *)p1, &params);
  }

  /* otherwise */
  sap_params params = {0};
  params.p2 = (uint8 const *)p2;
  params.p3 = (uint8 const *)p3;
  params.p4 = (uint8 const *)p4;
  params.p5 = (uint8 const *)p5;
  params.n1 = n1;
  params.n2 = n2;
  params.n3 = n3;
  params.n4 = n4;
  params.n5 = n5;
  return send_to_app(name.c_str(), id, transaction_id, (uint8 const *)p1, &params);
}

/**
 *  Send a message to the app.
 */
int net_mobilewebprint::controller_base_t::send_to_app(string const & name, int id, int32 transaction_id, buffer_view_i const & payload, buffer_t * data)
{
  if (mwp_app_callbacks_ != NULL) {
    mwp_params params = {0};
    params.p2 = payload.const_end();
    return send_to_app(name.c_str(), id, transaction_id, payload.const_begin(), &params);
  }

  /* otherwise */
  sap_params params = {0};
  params.p2 = payload.const_end();
  return send_to_app(name.c_str(), id, transaction_id, payload.const_begin(), &params);
}

/**
 *  Send a message to the app.
 */
int net_mobilewebprint::controller_base_t::send_to_app(string const & name, int id, int32 transaction_id)
{
  if (mwp_app_callbacks_ != NULL) {
    return send_to_app(name.c_str(), id, transaction_id, NULL, (mwp_params const *)NULL);
  }

  /* otherwise */
  return send_to_app(name.c_str(), id, transaction_id, NULL, (sap_params const *)NULL);
}

// Send a message to the app, takes the same args as the message that is sent
int  net_mobilewebprint::send_to_app(char const * name, int id, int32 transaction_id, uint8 const * p1, mwp_params const * params)
{
  if (g_controller == NULL) { return false; }

  /* otherwise */
  return g_controller->send_to_app(name, id, transaction_id, p1, params);
}

// Send a message to the app, takes the same args as the message that is sent
int  net_mobilewebprint::send_to_app(char const * name, int id, int32 transaction_id, uint8 const * p1, sap_params const * params)
{
  if (g_controller == NULL) { return false; }

  /* otherwise */
  return g_controller->send_to_app(name, id, transaction_id, p1, params);
}

bool net_mobilewebprint::controller_base_t::send_full_printer_list()
{
  return mq.send(HP_MWP_SEND_FULL_PRINTER_LIST);
}

bool net_mobilewebprint::controller_base_t::send_immediately(string const & msg_name, string const & payload)
{
  if (msg_name == "report_issue") {

    strmap query;
    if (arg("stackName", "").length() > 0)  { query = D("clientId", clientId(), "sessionId", sessionId, "stackName", arg("stackName")); }
    else                                    { query = D("clientId", clientId(), "sessionId", sessionId); }

    string search = make_search_string(query) + payload;
    string url    = formats("http://%s/%s", arg("upstreamServerEx", MWP_DEFAULT_FQDN), msg_name);
    send_to_app(msg_name, 0, 0, url.c_str(), search.c_str(), NULL);
    return true;
  }

  return mq.send_immediately(msg_name, payload);
}

std::string net_mobilewebprint::controller_base_t::job_stat(uint32 id, char const * stat_name, char const * value, bool silent)
{
  string orig_value = _lookup_job_stat(id, stat_name, string());

  // If we are frozen, do not allow updating the stat
  if (job_frozen(id, stat_name)) {
    log_v(4, "", "job %d is frozen -- not updating %s to %s", id, stat_name, value);
    return orig_value;
  }

  job_stats[id].attrs[stat_name] = value;
  log_v(4, "controller_t", "setting job_stat(%d): |%s|=|%s|\n%s", id, stat_name, value, job_stats[id].debug_to_json().c_str());

  if (!silent) { job_stat_changed(id, orig_value != value, stat_name); }

  // Set "current" when "jobStatus" is set
  if (string(stat_name) == "jobStatus") {
    job_stat(id, "current", value, silent);
  }

  return value;
}

std::string net_mobilewebprint::controller_base_t::job_stat(uint32 id, char const * stat_name, string const & value, bool silent)
{
  string orig_value = _lookup_job_stat(id, stat_name, string());

  // If we are frozen, do not allow updating the stat
  if (job_frozen(id, stat_name)) {
    log_v(4, "", "job %d is frozen -- not updating %s to %s", id, stat_name, value.c_str());
    return orig_value;
  }

  job_stats[id].attrs[stat_name] = value;
  log_v(4, "controller_t", "setting job_stat(%d): |%s|=|%s|\n%s", id, stat_name, value.c_str(), job_stats[id].debug_to_json().c_str());

  if (!silent) { job_stat_changed(id, value != orig_value, stat_name); }

  // Set "current" when "jobStatus" is set
  if (string(stat_name) == "jobStatus") {
    job_stat(id, "current", value, silent);
  }

  return value;
}

int net_mobilewebprint::controller_base_t::job_stat(uint32 id, char const * stat_name, int value, bool silent)
{
  int orig_value = _lookup_job_stat(id, stat_name, 0);

  // If we are frozen, do not allow updating the stat
  if (job_frozen(id, stat_name)) {
    log_v(4, "", "job %d is frozen -- not updating %s to %d", id, stat_name, value);
    return orig_value;
  }

  job_stats[id].int_attrs[stat_name] = value;
  //log_d(1, "controller_t", "%s", job_stats[id].to_json().c_str());

  if (!silent) { job_stat_changed(id, value != orig_value, stat_name); }
  return value;
}

int net_mobilewebprint::controller_base_t::job_stat_incr(uint32 id, char const * stat_name, int value, bool silent)
{
  // If we are frozen, do not allow updating the stat
  if (job_frozen(id, stat_name)) {
    log_v(4, "", "job %d is frozen -- not incrementing %s by %d", id, stat_name, value);
    return 0;
  }

  if (job_stats.find(id) == job_stats.end()) {
    return (job_stats[id].int_attrs[stat_name] = value);
  }

  if (!_has(job_stats[id].int_attrs, stat_name)) {
    return (job_stats[id].int_attrs[stat_name] = value);
  }

  job_stats[id].int_attrs[stat_name] += value;
  //log_d(1, "controller_t", "%s", job_stats[id].to_json().c_str());

  if (!silent) { job_stat_changed(id, value != 0, stat_name); }
  return job_stats[id].int_attrs[stat_name];
}

bool net_mobilewebprint::controller_base_t::job_stat(uint32 id, char const * stat_name, bool value, bool silent)
{
  bool orig_value = _lookup_job_stat(id, stat_name, false);

  // If we are frozen, do not allow updating the stat
  if (job_frozen(id, stat_name)) {
    log_v(4, "", "job %d is frozen -- not updating %s to %d", id, stat_name, (int)value);
    return orig_value;
  }

  job_stats[id].bool_attrs[stat_name] = value;
  //log_d(1, "controller_t", "%s", job_stats[id].to_json().c_str());

  if (!silent) { job_stat_changed(id, value != orig_value, stat_name); }

  return value;
}

bool net_mobilewebprint::controller_base_t::job_frozen(uint32 id, char const * stat_name)
{
  if (string(stat_name) == "getting_progress") {
    return false;
  }

  return _lookup_job_stat(id, "frozen", false);
}

void net_mobilewebprint::controller_base_t::job_stat_changed(uint32 id, bool value, char const * stat_name)
{
  //if (value) { log_d(1, "", "$$$$$$$$$$$$$$$ %s changed for %d", stat_name, id); }
  job_stats[id].bool_attrs["changed"] = value;
}

void net_mobilewebprint::controller_base_t::packet_stat(string const & ip, char const * stat_name, string const & value)
{
  packet_stats[ip].attrs[stat_name] = value;
}

void net_mobilewebprint::controller_base_t::packet_stat(string const & ip, char const * stat_name, int value)
{
  packet_stats[ip].int_attrs[stat_name] = value;
}

void net_mobilewebprint::controller_base_t::packet_stat_incr(string const & ip, char const * stat_name, int value)
{
  if (packet_stats.find(ip) == packet_stats.end()) {
    packet_stats[ip].int_attrs[stat_name] = value;
    return;
  }

  if (!_has(packet_stats[ip].int_attrs, stat_name)) {
    packet_stats[ip].int_attrs[stat_name] = value;
    return;
  }

  packet_stats[ip].int_attrs[stat_name] += value;
}

void net_mobilewebprint::controller_base_t::packet_stat(string const & ip, char const * stat_name, bool value)
{
  packet_stats[ip].bool_attrs[stat_name] = value;
}

bool net_mobilewebprint::controller_base_t::from_slp(buffer_view_i const & payload)
{
  return printers.from_slp(slp, payload);
}

bool net_mobilewebprint::controller_base_t::from_attrs(strmap const & attrs)
{
  strlist tags;
  return from_attrs(attrs, tags);
}

bool net_mobilewebprint::controller_base_t::from_attrs(strmap const & attrs, strlist const & tags)
{
  return printers.from_attrs(attrs, tags);
}

bool net_mobilewebprint::controller_base_t::from_1284_attrs(strmap const & attrs)
{
  strlist tags;
  return from_1284_attrs(attrs, tags);
}

bool net_mobilewebprint::controller_base_t::from_1284_attrs(strmap const & attrs, strlist const & tags)
{
  return printers.from_1284_attrs(attrs, tags);
}

uint32 net_mobilewebprint::controller_base_t::_unique()
{
  return unique_number++;
}

std::string net_mobilewebprint::controller_base_t::_unique(string prefix)
{
  return prefix + mwp_itoa(unique_number++);
}

net_mobilewebprint::mq_t & net_mobilewebprint::get_mq(controller_base_t & controller)
{
  return controller.mq;
}

bool net_mobilewebprint::controller_base_t::read_arp_cache(char const * arp_cache_filename, serialization_json_t & json)
{
#ifdef HAVE_PROC_NET_ARP
  char buf[1024];
  strvlist parts;
  FILE * pf = fopen(arp_cache_filename, "r");
  if (pf) {
    serialization_json_t & sub_json = json.getObject("items");

    int line = 0;
    while (fgets(buf, sizeof(buf), pf)) {

      if (line++ == 0) { continue; }          // Skip the first line, it has a table heading

      if (splitv(parts, buf, sizeof(buf), ' ') >= 4) {
        string ip  = parts[0];
        string mac = parts[3];

        log_v(4, "", "procnetarp: %15s %18s %s", ip.c_str(), mac.c_str(), buf);
        sub_json.set(dashify_key(ip), mac);

        // Inform printer objects of these found mac addresses
        if (mac_addresses.find(ip) == mac_addresses.end()) {
          if (printers._get_printer(mac) != NULL) {
            log_vs(2, "", "Found MAC for printer: %s -> %s", mac, ip);
            mac_addresses    .insert(std::make_pair(ip, mac));
            new_mac_addresses.insert(std::make_pair(ip, mac));
          }
        }
      }
    }

    fclose(pf);
    return true;
  }
#endif

  return false;
}

net_mobilewebprint::mwp_app_callback_t::mwp_app_callback_t(std::string name_, void * app_data_, hp_mwp_callback_t callback_)
  : name(name_), app_data(app_data_), callback(callback_)
{
}

net_mobilewebprint::mwp_app_callback_t::mwp_app_callback_t()
  : app_data(NULL), callback(NULL)
{
}

net_mobilewebprint::sap_app_callback_t::sap_app_callback_t(std::string name_, void * app_data_, hp_sap_callback_t callback_)
  : name(name_), app_data(app_data_), callback(callback_)
{
}

net_mobilewebprint::sap_app_callback_t::sap_app_callback_t()
  : app_data(NULL), callback(NULL)
{
}

net_mobilewebprint::mwp_app_hf_callback_t::mwp_app_hf_callback_t(std::string name_, void * app_data_, hp_mwp_hf_callback_t callback_)
  : name(name_), app_data(app_data_), callback(callback_)
{
}

net_mobilewebprint::mwp_app_hf_callback_t::mwp_app_hf_callback_t()
  : app_data(NULL), callback(NULL)
{
}

net_mobilewebprint::sap_app_hf_callback_t::sap_app_hf_callback_t(std::string name_, void * app_data_, hp_sap_hf_callback_t callback_)
  : name(name_), app_data(app_data_), callback(callback_)
{
}

net_mobilewebprint::sap_app_hf_callback_t::sap_app_hf_callback_t()
  : app_data(NULL), callback(NULL)
{
}

void net_mobilewebprint::controller_base_t::log_api(char const * format, ...)
{
  if (!flag("log_api")) { return; }

  va_list argList;

  char buffer[2048];

  va_start(argList, format);
  vsprintf(buffer, format, argList);
  va_end(argList);

  log_d(buffer, (log_param_t)NULL);
}

net_mobilewebprint::controller_http_request_t::controller_http_request_t(uint32 txn_id_, std::string const & verb_, std::string const & url_, serialization_json_t const & json_)
  : txn_id(txn_id_),
    verb(verb_),
    url(url_),
    json_body(json_),
    content_type("application/json"),
    body("")
{
}

net_mobilewebprint::controller_http_request_t::controller_http_request_t(uint32 txn_id_, std::string const & verb_, std::string const & url_, strmap const & query, serialization_json_t const & json_)
  : txn_id(txn_id_),
    verb(verb_),
    url(url_),
    json_body(json_),
    content_type("application/json"),
    body("")
{
  string search;
  for (strmap::const_iterator it = query.begin(); it != query.end(); ++it) {
    if (search.length() > 0) {
      search += "&";
    }

    search += it->first + "=" + it->second;
  }

  if (search.length() > 0) {
    url = url + "?" + search;
  }
}

net_mobilewebprint::controller_http_request_t::controller_http_request_t(uint32 txn_id_, std::string const & verb_, std::string const & url_, string const & body_, string content_type_)
  : txn_id(txn_id_),
    verb(verb_),
    url(url_),
    //json_body(json_t()),
    content_type(content_type_),
    body(body_)
{
}

