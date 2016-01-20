
#include "mwp_upstream.hpp"
#include "mwp_controller.hpp"

using namespace net_mobilewebprint;
using namespace net_mobilewebprint::mq_enum;

net_mobilewebprint::upstream_t::upstream_t(controller_base_t & controller_)
  : controller(controller_), mq(controller_.mq)
{
  mq.on(this);
  //mq.on_selected(this);
}

std::string net_mobilewebprint::upstream_t::mod_name()
{
  return "upstream_t";
}

std::string net_mobilewebprint::upstream_t::send(string const & mod_name, string const & endpoint, serialization_json_t & json)
{
  uint32 txn_id    = controller._unique();
  string txn_name  = mod_name + ":" + mwp_itoa(txn_id);

  send(endpoint, json, txn_name, "_upstream_response", stats_t());

  return txn_name;
}

#if 0
std::string net_mobilewebprint::upstream_t::send(string const & mod_name, string const & endpoint, string const & body, stats_t const & stats)
{
  uint32 txn_id    = controller._unique();
  string txn_name  = mod_name + ":" + mwp_itoa(txn_id);

  send(endpoint, body, txn_name, "_upstream_response", stats);

  return txn_name;
}
#endif

std::string net_mobilewebprint::upstream_t::send(string const & endpoint, serialization_json_t & json, string const & message_name, stats_t const & stats)
{
  uint32 txn_id    = controller._unique();
  string txn_name = message_name + "/" + mwp_itoa(txn_id);
  send(endpoint, json, txn_name, message_name, stats);
  return txn_name;
}

void net_mobilewebprint::upstream_t::send(string const & endpoint, serialization_json_t & json, string const & txn_name, string const & message_name, stats_t const & stats)
{
  uint32 txn_id = controller.curl_http_post(endpoint, json);

  responses[txn_id] = new upstream_response_t(txn_name, message_name, stats);
}

std::string net_mobilewebprint::upstream_t::send(string const & endpoint, string const & body, string content_type, string const & message_name, stats_t const & stats)
{
  uint32 txn_id    = controller._unique();
  string txn_name = message_name + "/" + mwp_itoa(txn_id);
  send(endpoint, body, content_type, txn_name, message_name, stats);
  return txn_name;
}

void net_mobilewebprint::upstream_t::send(string const & endpoint, string const & body, string content_type, string const & txn_name, string const & message_name, stats_t const & stats)
{
  uint32 txn_id = controller.curl_http_post(endpoint, body, content_type);

  responses[txn_id] = new upstream_response_t(txn_name, message_name, stats);
}

std::string net_mobilewebprint::upstream_t::send_local(string ip, int port, string path, string const & body, string content_type, string const & message_name, stats_t const & stats)
{
  uint32 txn_id    = controller._unique();
  string txn_name = message_name + "/" + mwp_itoa(txn_id);
  send_local(ip, port, path, body, content_type, txn_name, message_name, stats);
  return txn_name;
}

void net_mobilewebprint::upstream_t::send_local(string ip, int port, string path, string const & body, string content_type, string const & txn_name, string const & message_name, stats_t const & stats)
{
  uint32 txn_id = controller.curl_local_send(ip, port, path, body, content_type, "POST");

  responses[txn_id] = new upstream_response_t(txn_name, message_name, stats);
}

std::string net_mobilewebprint::upstream_t::get(string const & endpoint, string const & message_name, stats_t const & stats)
{
  uint32 txn_id    = controller._unique();
  string txn_name  = message_name + "/" + mwp_itoa(txn_id);
  controller.curl_http_get(endpoint, txn_id);

  responses[txn_id] = new upstream_response_t(txn_name, message_name, stats);

  return txn_name;
}

void net_mobilewebprint::upstream_t::get(string const & endpoint, uint32 txn_id, string const & message_name)
{
  string txn_name = message_name + "/" + mwp_itoa(txn_id);
  controller.curl_http_get(endpoint, txn_id);

  responses[txn_id] = new upstream_response_t(txn_name, message_name, stats_t());
}

void net_mobilewebprint::upstream_t::get(string const & endpoint, string const & txn_name, string const & message_name)
{
  uint32 txn_id = controller.curl_http_get(endpoint);

  responses[txn_id] = new upstream_response_t(txn_name, message_name, stats_t());
}

net_mobilewebprint::e_handle_result net_mobilewebprint::upstream_t::handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  upstream_response_t * response = NULL;

  if (name == "_on_http_headers") {
//    return handled;
    if ((response = _lookup(responses, (uint32)extra.txn_id)) != NULL) {
      push(response->headers_chunks, data, payload);
      return handled_and_took_message;    // We now have ownership of the data memory
    }
  }

  if (name == "_on_http_payload") {
    if ((response = _lookup(responses, (uint32)extra.txn_id)) != NULL) {
      push(response->body_chunks, data, payload);
      return handled_and_took_message;    // We now have ownership of the data memory
    }
  }

  if (name == "_on_txn_close") {
    // We have the response, send the result payload
    if ((response = _lookup(responses, (uint32)extra.txn_id)) != NULL) {
      buffer_t * packet = mq.message(response->message_name.c_str(), 0, response->txn_name);

      string headers = join(response->headers_chunks, "");
      string body    = join(response->body_chunks, "");

//      log_vs(2, "upstream_t", "payload-headers back from upstream:%s", headers);
//      log_vs(2, "upstream_t", "payload-body back from upstream:%s", body);

      packet->append((uint32)extra.txn_id);
      packet->append((uint32)200);
      packet->append((uint32)headers.length() + 1);
      packet->append((uint32)body.length() + 1);

      //packet->dump(NULL);

      packet->appendT(headers);
      //packet->dump(NULL);
      packet->appendT(body);
      //packet->dump(NULL);
      mq.send(packet);

      //delete response;
      //responses.erase((uint32)extra.txn_id);
    }
  }

  return not_impl;
}

bool net_mobilewebprint::upstream_t::parse_response(buffer_view_i const & payload, int & code, string & http_version, strmap & headers, string & body_str, stats_t & stats_out)
{
  buffer_view_i::const_iterator   p         = payload.first();
  upstream_response_t *           response  = NULL;

  uint32 txn_id      = payload.read_uint32(p);
  uint32 http_code   = payload.read_uint32(p);
  uint32 headers_len = payload.read_uint32(p);
  uint32 body_len    = payload.read_uint32(p);

  string headers_str = payload.read_string_nz(p, headers_len);
            body_str = payload.read_string_nz(p, body_len);

  if ((response = _lookup(responses, (uint32)txn_id)) != NULL) {
    stats_out = response->stats;
    delete response;
    responses.erase((uint32)txn_id);
  }

  strlist headers_list = split((char const *)headers_str.c_str(), "\r\n");

  strlist::const_iterator it = headers_list.begin();
  if (it != headers_list.end()) {
    string key, value, res_line, fragment;

    int code_ = 0;
    string http_version_;
    for (; it != headers_list.end(); ++it) {
      string line = *it;

      // First, determine if it is a result line, like "HTTP/1.1 100 Continue"
      if (line.length() > 0) {
        http_version_ = parse_on(line, " ");

        if (line.length() > 0) {
          fragment = parse_on(line, " ");
          if (is_num(fragment)) {
            code_ = mwp_atoi(fragment);
            if (_starts_with(http_version_, "HTTP")) {
              code = code_;
              http_version = http_version_;
              headers = strmap();   // This resets the header list
              continue;
            }
          }
        }
      }

      // Not a result line, see if it is a key-value header
      strlist kv = split(it->c_str(), ":", 1);
      strlist::const_iterator it_kv = kv.begin();
      if (it_kv != kv.end()) {

        // Get the key
        key = _lower(*it_kv++);
        if (it_kv != kv.end()) {

          // Get the value
          value = ltrim(*it_kv);

          headers[key] = value;
        }
      }
    }
  }

  return true;
}

std::string net_mobilewebprint::upstream_t::parse_response(buffer_view_i const & payload, int & code, string & http_version, strmap & headers, string & body_str, json_t & json, json_array_t & json_array, stats_t & stats_out)
{
  if (!parse_response(payload, code, http_version, headers, body_str, stats_out)) { return ""; }

  /* otherwise */
  string content_type("");
  if (_has(headers, "content-type")) {
    content_type = headers["content-type"];
  }

  if (content_type == "application/json") {
    JSON_parse(json, body_str);
    JSON_parse_array(json_array, body_str);
  }

  return content_type;
}

bool net_mobilewebprint::upstream_t::parse_response(buffer_view_i const & payload, int & code, string & http_version, strmap & headers, json_t & json, stats_t & stats_out)
{
  string body_str;

  if (!parse_response(payload, code, http_version, headers, body_str, stats_out)) { return false; }

  /* otherwise */
  string content_type("");
  if (_has(headers, "content-type")) {
    content_type = headers["content-type"];
  }

  if (content_type != "application/json" && content_type != "") { return false; }

  /* otherwise */
  return JSON_parse(json, body_str);
}

bool net_mobilewebprint::upstream_t::parse_response(buffer_view_i const & payload, int & code, json_t & json, stats_t & stats_out)
{
  string body_str, http_version;
  strmap headers;

  if (!parse_response(payload, code, http_version, headers, body_str, stats_out)) { return false; }

  /* otherwise */
  string content_type("");
  if (_has(headers, "content-type")) {
    content_type = headers["content-type"];
  }

  if (content_type != "application/json" && content_type != "") { return false; }

  /* otherwise */
  return JSON_parse(json, body_str);
}

bool net_mobilewebprint::upstream_t::parse_response(buffer_view_i const & payload, int & code, string & http_version, strmap & headers, json_array_t & json, stats_t & stats_out)
{
  string body_str;

  if (!parse_response(payload, code, http_version, headers, body_str, stats_out)) { return false; }

  /* otherwise */
  string content_type("");
  if (_has(headers, "content-type")) {
    content_type = headers["content-type"];
  }

  if (content_type != "application/json" && content_type != "") { return false; }

  /* otherwise */
  bool result = JSON_parse_array(json, body_str);
  return result;
}

bool net_mobilewebprint::upstream_t::parse_response(buffer_view_i const & payload, int & code, json_array_t & json, stats_t & stats_out)
{
  string body_str, http_version;
  strmap headers;

  if (!parse_response(payload, code, http_version, headers, body_str, stats_out)) { return false; }

  /* otherwise */
  string content_type("");
  if (_has(headers, "content-type")) {
    content_type = headers["content-type"];
  }

  if (content_type != "application/json" && content_type != "") { return false; }

  /* otherwise */
  return JSON_parse_array(json, body_str);
}

net_mobilewebprint::upstream_handler_t::~upstream_handler_t()
{
}


//---------------------------------------------------------------------------------------------------------------------
// upstream_response_t
//---------------------------------------------------------------------------------------------------------------------
net_mobilewebprint::upstream_response_t::upstream_response_t(string const & txn_name_, string const & message_name_, stats_t const & stats_)
  : txn_name(txn_name_), message_name(message_name_), stats(stats_)
{
}

net_mobilewebprint::upstream_response_t::~upstream_response_t()
{
  chunks_t::iterator it = headers_chunks.begin();
  for (; it != headers_chunks.end(); ++it) {
    delete (*it); /**/ num_chunk_allocations -= 1;
  }

  for (it = body_chunks.begin(); it != body_chunks.end(); ++it) {
    delete (*it); /**/ num_chunk_allocations -= 1;
  }
}

