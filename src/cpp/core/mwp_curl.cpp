
#include "mwp_curl.hpp"
#include "mwp_controller.hpp"
#include "mwp_utils.hpp"
#include "mwp_mq.hpp"
#include "curl.h"

using namespace net_mobilewebprint::msg;
using namespace net_mobilewebprint;

/**
 *  curl_t ctor.
 */
net_mobilewebprint::curl_t::curl_t(controller_base_t & controller_, string server_name_, uint16 server_port_)
  : controller(controller_), mq(controller_.mq), server_name(server_name_), server_port(server_port_), mcurl(NULL)
{
  mq.on(this);
  mq.on_selected(this);

  curl_global_init(0L);
}

net_mobilewebprint::curl_t::~curl_t()
{
  curl_multi_cleanup(mcurl);
  curl_global_cleanup();
}

std::string net_mobilewebprint::curl_t::mod_name()
{
  return "curl_t";
}

net_mobilewebprint::curl_connection_t * net_mobilewebprint::curl_t::fetch_from_mwp_server(char const * verb, string const & path, uint32 connection_id)
{
  log_d(1, "curl_t", "curl_t::fetch %s %s/%s %d", verb, server_name.c_str(), path.c_str(), connection_id);

  if (mcurl == NULL) {
    mcurl = curl_multi_init();
  }

  return (connections[connection_id] = new curl_connection_t(controller, mcurl, this, verb, path, connection_id));
}

net_mobilewebprint::curl_connection_t * net_mobilewebprint::curl_t::post_mwp_server(serialization_json_t const & json, string const & path, uint32 connection_id)
{
  //log_d(1, "curl_t", "curl_t::POST-JSON %s %d", path.c_str(), connection_id);

  if (mcurl == NULL) {
    mcurl = curl_multi_init();
  }

  return (connections[connection_id] = new curl_connection_t(controller, mcurl, this, json, path, connection_id));
}

net_mobilewebprint::curl_connection_t * net_mobilewebprint::curl_t::post_mwp_server(string const & json_str, string const & path, uint32 connection_id)
{
  //log_d(1, "curl_t", "curl_t::POST-JSON %s %d", path.c_str(), connection_id);

  if (mcurl == NULL) {
    mcurl = curl_multi_init();
  }

  return (connections[connection_id] = new curl_connection_t(controller, mcurl, this, path, connection_id, json_str));
}

int net_mobilewebprint::curl_t::pre_select(mq_pre_select_data_t * data)
{
  if (connections.size() > 0 && mcurl != NULL) {
    data->status = curl_multi_fdset(mcurl, data->readable, data->writable, data->exceptional, &data->max_fd);
    if (data->max_fd != -1) {
      curl_multi_timeout(mcurl, &data->timeout);
    } else {
      data->timeout = 100;
    }

    return 1;
  }

  return 0;
}

net_mobilewebprint::e_handle_result net_mobilewebprint::curl_t::handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  e_handle_result result = unhandled;

  return result;
}

net_mobilewebprint::e_handle_result net_mobilewebprint::curl_t::_mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra)
{
  int num_running_handles = 0;
  if (connections.size() > 0 && mcurl != NULL) {
    curl_multi_perform(mcurl, &num_running_handles);

    // Should we remove the connection?
    if (num_running_handles < connections.size()) {
      CURLMsg * msg = NULL;
      int msgs_in_queue = 0;
      while ((msg = curl_multi_info_read(mcurl, &msgs_in_queue)) != NULL) {
        if(msg->msg == CURLMSG_DONE) {
          connections_t::iterator it = _find_by_curl_handle(msg->easy_handle);
          if (it != connections.end()) {
            curl_connection_t * connection = it->second;
            delete connection;
            connections.erase(it);
          }
        }
      }
    }
  }

  return handled;
}

net_mobilewebprint::curl_t::connections_t::iterator net_mobilewebprint::curl_t::_find_by_curl_handle(CURL* curl)
{
  for (connections_t::iterator it = connections.begin(); it != connections.end(); ++it) {
    curl_connection_t * connection = it->second;
    if (connection->curl == curl) {
      return it;
    }
  }

  return connections.end();
}

std::string net_mobilewebprint::curl_t::translate_path(string const & path)
{
  string full_url = "http://" + server_name + ":" + mwp_itoa(server_port) + path;

  if (_starts_with(path, "netapp::/")) {
    size_t end = server_name.find(".mobile");
    if (end != string::npos) {
      string netapp_server_name = server_name;
      netapp_server_name.replace(0, end, "netapp");
      string netapp_path = path;
      netapp_path.replace(0, ::strlen("netapp::"), "");
      full_url = "http://" + netapp_server_name + ":" + mwp_itoa(server_port) + netapp_path;
    }
  }

  return full_url;
}

int net_mobilewebprint::curl_t::verbose_adj(string const & url)
{
  if (_starts_with(url, "/poll"))      { return 3; }
  if (_starts_with(url, "/telemetry")) { return 2; }

  if (_starts_with(url, "netapp::/command"))   { return 2; }

  return 0;
}

//---------------------------------------------------------------------------------------------------------------------
static size_t _conn_write_data(void * buffer, size_t size, size_t nmemb, void *userp)
{
  return ((net_mobilewebprint::curl_connection_t*)userp)->conn_write_data(buffer, size, nmemb);
}

static size_t _conn_header_data(void * buffer, size_t size, size_t nmemb, void *userp)
{
  return ((net_mobilewebprint::curl_connection_t*)userp)->conn_header_data(buffer, size, nmemb);
}

static size_t _conn_read_data(void * buffer, size_t size, size_t nmemb, void *userp)
{
  return ((net_mobilewebprint::curl_connection_t*)userp)->conn_read_data(buffer, size, nmemb);
}

/**
 *  curl_connector_t ctor.
 *
 *  VERB path
 *
 */
net_mobilewebprint::curl_connection_t::curl_connection_t(controller_base_t & controller_, CURLM * mcurl_, curl_t * parent_, char const * verb_, string const & path_, uint32 connection_id_)
  : controller(controller_), mq(controller_.mq), parent(parent_), connection_id(connection_id_), verb(verb_), path(path_), full_url(path_), mcurl(mcurl_), curl(NULL), packet_num(-1), num_recieved(0),
    request_payload(NULL), req_headers(NULL)
{
  _init();

  full_url = parent->translate_path(path);
  _set_url(full_url);
  _go();
}

/**
 *  curl_connector_t ctor.
 *
 *  For JSON POST.
 */
net_mobilewebprint::curl_connection_t::curl_connection_t(controller_base_t & controller_, CURLM * mcurl_, curl_t * parent_, serialization_json_t const & json, string const & path_, uint32 connection_id_)
  : controller(controller_), mq(controller_.mq), parent(parent_), connection_id(connection_id_), verb("POST"), path(path_), full_url(path_), mcurl(mcurl_), curl(NULL), packet_num(-1), num_recieved(0),
    request_payload(NULL), req_headers(NULL)
{
  _init();

  full_url = parent->translate_path(path);
  _set_url(full_url);

  _set_body(json);

  _go();
}

/**
 *  curl_connector_t ctor.
 *
 *  For JSON_str POST.
 */
net_mobilewebprint::curl_connection_t::curl_connection_t(controller_base_t & controller_, CURLM * mcurl_, curl_t * parent_, string const & path_, uint32 connection_id_, string const & body_str)
  : controller(controller_), mq(controller_.mq), parent(parent_), connection_id(connection_id_), verb("POST"), path(path_), full_url(path_), mcurl(mcurl_), curl(NULL), packet_num(-1), num_recieved(0),
    request_payload(NULL), req_headers(NULL)
{
  _init();

  full_url = parent->translate_path(path);
  _set_url(full_url);

  _set_body(body_str);

  _go();
}

net_mobilewebprint::curl_connection_t::~curl_connection_t()
{
  mq.send(mq.message("_on_txn_close", 0, connection_id));
  if (req_headers != NULL) {
    curl_slist_free_all(req_headers);
  }

  curl_multi_remove_handle(mcurl, curl);
  curl_easy_cleanup(curl);
}

net_mobilewebprint::curl_connection_t & net_mobilewebprint::curl_connection_t::_init()
{
  int result = 0;

  curl = curl_easy_init();
  if (get_flag("verbose") && get_option("v_log_level", 0) >= 4) {
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
  }
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1);

  result = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _conn_write_data);
  result = curl_easy_setopt(curl, CURLOPT_WRITEDATA,     this);

  result = curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, _conn_header_data);
  result = curl_easy_setopt(curl, CURLOPT_HEADERDATA,     this);

  // Are we using a proxy server?
  if (controller.arg("http_proxy_name").length() != 0) {
    string proxy_server = controller.arg("http_proxy_name") + ":" + mwp_itoa(controller.arg("http_proxy_port", parent->server_port));
    result = curl_easy_setopt(curl, CURLOPT_PROXY, proxy_server.c_str());
  }

  return *this;
}

net_mobilewebprint::curl_connection_t & net_mobilewebprint::curl_connection_t::_init_read_fn()
{
  int result = 0;

  result = curl_easy_setopt(curl, CURLOPT_READFUNCTION, _conn_read_data);
  result = curl_easy_setopt(curl, CURLOPT_READDATA,     this);

  return *this;
}

net_mobilewebprint::curl_connection_t & net_mobilewebprint::curl_connection_t::_set_url(string const & url, char const * verb)
{
  int result = 0;

  char const * fmt = "cUrl POSTing: %s";
  if (strcmp(verb, "GET") == 0) {
    fmt = "cUrl GETting: %s";
  }

  log_v(1 + verbose_adj(), "", fmt, url.c_str());
  result = curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

  return *this;
}

net_mobilewebprint::curl_connection_t & net_mobilewebprint::curl_connection_t::_set_body(string const & body, bool do_logging)
{
  _init_read_fn();

  if (do_logging) {
    log_v(2 + verbose_adj(), "", "        body: %s", body.c_str());
  }

  buffer_t * payload = new buffer_t(body.c_str()); /**/ num_buffer_allocations += 1;
  request_payload = new chunk_t(payload, *payload);

  return *this;
}

net_mobilewebprint::curl_connection_t & net_mobilewebprint::curl_connection_t::_set_body(serialization_json_t const & body, char const * verb)
{
  int result = 0;

  body.sjson_log_v(2 + verbose_adj(), "");

  string json_str = body.stringify();
  _set_body(json_str, false);

  result = curl_easy_setopt(curl, CURLOPT_POST, 1);

  req_headers = curl_slist_append(req_headers, "Transfer-Encoding: chunked");
  req_headers = curl_slist_append(req_headers, "Content-Type: application/json");

  return *this;
}

net_mobilewebprint::curl_connection_t & net_mobilewebprint::curl_connection_t::_go()
{
  int result = 0;

  if (req_headers != NULL) {
    result  = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, req_headers);
  }

  result = curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mario-" BUILD_USER " 1.1." BUILD_NUMBER " libcurl");

  result = curl_multi_add_handle(mcurl, curl);
  return *this;
}

std::string net_mobilewebprint::curl_connection_t::mod_name()
{
  return "curl_connection_t";
}

int net_mobilewebprint::curl_connection_t::pre_select(mq_pre_select_data_t * pre_select_data)
{
  return -1;  // not implemented
}

net_mobilewebprint::e_handle_result net_mobilewebprint::curl_connection_t::_mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra)
{
  return not_impl;
}

int net_mobilewebprint::curl_connection_t::verbose_adj()
{
  return parent->verbose_adj(path);
}


//---------------------------------------------------------------------------------------------------------------------------
//
// Helper functions
//
//---------------------------------------------------------------------------------------------------------------------------
size_t net_mobilewebprint::curl_connection_t::conn_write_data(void * buffer_, size_t size, size_t nmemb)
{
  byte * buffer = (byte*)buffer_;

  buffer_t * packet = mq.message("_on_http_payload", 0, connection_id);

  packet->append(buffer, size * nmemb);

  log_v(4, "curl_t", "Sending HTTP: %d (%d * %d)", (int)(size * nmemb), (int)size, (int)nmemb);
  mq.send(packet);

  return size * nmemb;
}

size_t net_mobilewebprint::curl_connection_t::conn_header_data(void * buffer_, size_t size, size_t nmemb)
{
  byte * buffer = (byte*)buffer_;

  buffer_t * packet = mq.message("_on_http_headers", 0, connection_id);

  packet->append(buffer, size * nmemb);

  log_v(4, "curl_t", "Sending HTTP headers: %d (%d * %d)", (int)(size * nmemb), (int)size, (int)nmemb);
  mq.send(packet);

  return size * nmemb;
}

size_t net_mobilewebprint::curl_connection_t::conn_read_data(void * buffer, size_t size, size_t nmemb)
{
  if (request_payload == NULL) { return 0; }

  size_t num_to_send = min(size*nmemb, request_payload->view.dsize() - 1);    // Dont include terminating NULL
  if (num_to_send == 0) { return 0; }

  log_vs(4, "curl_t", "Sending %d\n%s", (int)num_to_send, string((char*)request_payload->view.begin(), request_payload->view.dsize()));
  ::memcpy(buffer, request_payload->view.begin(), num_to_send);
  request_payload->view._begin += num_to_send;
  return num_to_send;
}



