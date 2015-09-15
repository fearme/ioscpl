
#include "mwp_types.hpp"
#include "mwp_mini_curl.hpp"
#include "mwp_controller.hpp"
#include "mwp_utils.hpp"

#include <cstdio>
#include <cstdlib>

using namespace net_mobilewebprint::msg;
using namespace net_mobilewebprint::mq_enum;

using net_mobilewebprint::network_node_t;
using net_mobilewebprint::buffer_view_i;
using net_mobilewebprint::buffer_t;

net_mobilewebprint::network_node_t * _connect_through(std::string const & server_name, uint16 port, int timeout_ms);

/**
 *  mini_curl_t ctor.
 */
net_mobilewebprint::mini_curl_t::mini_curl_t(controller_base_t & controller_, string server_name_, uint16 server_port_)
  : controller(controller_), mq(controller_.mq), server_name(server_name_), server_port(server_port_)
{
  mq.on_selected(this);
}

std::string net_mobilewebprint::mini_curl_t::mod_name()
{
  return "mini_curl_t";
}

net_mobilewebprint::mini_curl_connection_t * net_mobilewebprint::mini_curl_t::post_mwp_server(serialization_json_t const & json, string const & path, uint32 connection_id)
{
  network_node_t * server = _connect_to(path);

  char const verb[] = "POST";

  if (server != NULL) {
    // Success.  Create the connection object

    return (connections[connection_id] = new mini_curl_connection_t(controller, server, json, path, connection_id));
  }

  /* otherwise */
  return NULL;
}

net_mobilewebprint::mini_curl_connection_t * net_mobilewebprint::mini_curl_t::post_mwp_server(string const & body, string const & path, uint32 connection_id)
{
  network_node_t * server = _connect_to(path);

  char const verb[] = "POST";

  if (server != NULL) {
    // Success.  Create the connection object

    return (connections[connection_id] = new mini_curl_connection_t(controller, server, path, connection_id, body));
  }

  /* otherwise */
  return NULL;
}

net_mobilewebprint::mini_curl_connection_t * net_mobilewebprint::mini_curl_t::fetch_from_mwp_server(char const * verb, string const & path_, uint32 connection_id)
{
  log_v(4, "", "fetching from mwp_server  %d %s %s", connection_id, verb, path_.c_str());
  string path(path_);
  int timeout_ms = -1;    // No timeout
  network_node_t * server = NULL;

  // What server should we connect to?
  string connection_server = server_name;
  uint16 connection_port = server_port;

  // Are we using a proxy server?
  string proxy_server = controller.arg("http_proxy_name");
  uint16 proxy_port   = controller.arg("http_proxy_port", connection_port);

  if (proxy_server.length() == 0) {
    server = _connect_through(connection_server, connection_port, timeout_ms = -1);
  } else {

    // We are using a proxy server... connect to it...
    if ((server = _connect_through(connection_server = proxy_server, connection_port = proxy_port, timeout_ms = 3000)) == NULL) {
      if ((server = _connect_through(connection_server = server_name, connection_port = server_port, timeout_ms = 3000)) == NULL) {
        if ((server = _connect_through(connection_server = proxy_server, connection_port = proxy_port, timeout_ms = 10000)) == NULL) {
          server = _connect_through(connection_server = server_name, connection_port = server_port, timeout_ms = 10000);
        }
      }
    }

    // ... and put the full URL into the request
    if (server != NULL && connection_server == proxy_server) {
      if (!_starts_with(path, "http")) {
        path = string("http://") + server_name + path;
      }
    }
  }

  if (server != NULL) {
    // Success.  Create the connection object
    log_v(4, "", "Connected to: %s:%d (through: %s:%d)", server->ip.c_str(), server->port, connection_server.c_str(), connection_port);

    return (connections[connection_id] = new mini_curl_connection_t(controller, server, verb, path, connection_id));
  }

  /* otherwise */
  return NULL;
}

net_mobilewebprint::network_node_t * net_mobilewebprint::mini_curl_t::_connect_to(std::string path)
{
  network_node_t *  server      = NULL;
  int               timeout_ms  = -1;    // No timeout

  // What server should we connect to?
  string connection_server = server_name;
  uint16 connection_port = server_port;

  // Are we using a proxy server?
  string proxy_server = controller.arg("http_proxy_name");
  uint16 proxy_port   = controller.arg("http_proxy_port", connection_port);

  if (proxy_server.length() == 0) {
    server = _connect_through(connection_server, connection_port, timeout_ms = -1);
  } else {

    // We are using a proxy server... connect to it...
    if ((server = _connect_through(connection_server = proxy_server, connection_port = proxy_port, timeout_ms = 3000)) == NULL) {
      if ((server = _connect_through(connection_server = server_name, connection_port = server_port, timeout_ms = 3000)) == NULL) {
        if ((server = _connect_through(connection_server = proxy_server, connection_port = proxy_port, timeout_ms = 10000)) == NULL) {
          server = _connect_through(connection_server = server_name, connection_port = server_port, timeout_ms = 10000);
        }
      }
    }

    // ... and put the full URL into the request
    if (server != NULL && connection_server == proxy_server) {
      if (!_starts_with(path, "http")) {
        path = string("http://") + server_name + path;
      }
    }
  }

  if (server != NULL) {
    log_v(4, "", "Connected to: %s:%d (through: %s:%d)", server->ip.c_str(), server->port, connection_server.c_str(), connection_port);
  }

  return server;
}

net_mobilewebprint::network_node_t * _connect_through(std::string const & server_name, uint16 port, int timeout_ms)
{
  net_mobilewebprint::log_v(4, "", "Connecting through: %s", server_name.c_str());

  net_mobilewebprint::network_node_t * server = new net_mobilewebprint::network_node_t(server_name, port);
  if (server->connect(timeout_ms) != 0) {
    return server;
  }

  delete server;
  return NULL;
}

int net_mobilewebprint::mini_curl_t::pre_select(mq_pre_select_data_t * pre_select_data)
{
  mini_curl_connection_t * connection = NULL;
  connections_t::iterator it;
  for (it = connections.begin(); it != connections.end(); ++it) {
    if ((connection = it->second) != NULL) {
      connection->pre_select(pre_select_data);
    }
  }

  return 0;
}

e_handle_result net_mobilewebprint::mini_curl_t::_mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra)
{
  e_handle_result result = unhandled;
  connections_t::iterator it;
  mini_curl_connection_t * connection = NULL;

  /* otherwise */
  result = handled;
  for (it = connections.begin(); it != connections.end(); ++it) {
    if ((connection = it->second) != NULL) {
      connection->_mq_selected(name, payload, data, extra);
      if (connection_is_closed(it, extra.txn_id)) {
        return result;
      }
    }
  }

  return result;
}

bool net_mobilewebprint::mini_curl_t::connection_is_closed(connections_t::iterator & it, int txn_id)
{
  mini_curl_connection_t const * connection = it->second;

  if (connection != NULL && connection->closed) {
    connections.erase(it);
    delete connection;
    return true;
  }

  return false;
}




//----------------------------------------------------------------------------
//
//  Free functions to allow mini_curl_connection_t to have multiple handlers
//
//----------------------------------------------------------------------------
//MWP_MQ_HANDLER_HELPER(mini_curl_connection_t, resend_attribute_request)

/**
 *  mini_curl_connector_t ctor.
 */
net_mobilewebprint::mini_curl_connection_t::mini_curl_connection_t(controller_base_t & controller_, network_node_t * connection_, char const * verb_, string const & path_, uint32 txn_id_)
  : controller(controller_), mq(controller_.mq), server(connection_), txn_id(txn_id_), verb(verb_), path(path_), closed(false), recv_packet(NULL), packet_num(-1), num_recieved(0), request_sent(false)
{
//  mq.on_selected(this);
  mq.register_tcp_for_select(*server);

  request_payload.append_strs_sans_null(verb_, " ", path.c_str(), " HTTP/1.0\r\n");
  request_payload.append_strs_sans_null("Host: demo.mobilewebprint.net\r\n");
  request_payload.append_strs_sans_null("User-Agent: Mario mini-curl 0.99\r\n");
  request_payload.append_strs_sans_null("\r\n");

  recv_packet = new buffer_t(64 * 1024); /**/ num_buffer_allocations += 1;
}

/**
 *  mini_curl_connector_t ctor.
 */
net_mobilewebprint::mini_curl_connection_t::mini_curl_connection_t(controller_base_t & controller_, network_node_t * connection_, serialization_json_t const & bodyJson, string const & path_, uint32 txn_id_)
  : controller(controller_), mq(controller_.mq), server(connection_), txn_id(txn_id_), verb("POST"), path(path_), closed(false), recv_packet(NULL), packet_num(-1), num_recieved(0), request_sent(false)
{
//  mq.on_selected(this);
  mq.register_tcp_for_select(*server);

  string body = bodyJson.stringify();

  request_payload.append_strs_sans_null("POST ", path.c_str(), " HTTP/1.0\r\n");
  request_payload.append_strs_sans_null("Host: demo.mobilewebprint.net\r\n");
  request_payload.append_strs_sans_null("User-Agent: Mario mini-curl 0.99\r\n");
  request_payload.append_strs_sans_null("Content-Type: application/json\r\n");
  request_payload.append_strs_sans_null("Content-Length: ", mwp_itoa(body.length()).c_str(), "\r\n");
  request_payload.append_strs_sans_null("\r\n");

  request_payload.append_strs_sans_null(body.c_str());

  recv_packet = new buffer_t(64 * 1024); /**/ num_buffer_allocations += 1;
}

/**
 *  mini_curl_connector_t ctor.
 */
net_mobilewebprint::mini_curl_connection_t::mini_curl_connection_t(controller_base_t & controller_, network_node_t * connection_, string const & path_, uint32 txn_id_, string const & body)
  : controller(controller_), mq(controller_.mq), server(connection_), txn_id(txn_id_), verb("POST"), path(path_), closed(false), recv_packet(NULL), packet_num(-1), num_recieved(0), request_sent(false)
{
//  mq.on_selected(this);
  mq.register_tcp_for_select(*server);

  request_payload.append_strs_sans_null("POST ", path.c_str(), " HTTP/1.0\r\n");
  request_payload.append_strs_sans_null("Host: demo.mobilewebprint.net\r\n");
  request_payload.append_strs_sans_null("User-Agent: Mario mini-curl 0.99\r\n");
  request_payload.append_strs_sans_null("Content-Type: application/json\r\n");
  request_payload.append_strs_sans_null("Content-Length: ", mwp_itoa(body.length()).c_str(), "\r\n");
  request_payload.append_strs_sans_null("\r\n");

  request_payload.append_strs_sans_null(body.c_str());

  recv_packet = new buffer_t(64 * 1024); /**/ num_buffer_allocations += 1;
}

net_mobilewebprint::mini_curl_connection_t::~mini_curl_connection_t()
{
  delete server;
  server = NULL;
  delete recv_packet;
  recv_packet = NULL;
}

std::string net_mobilewebprint::mini_curl_connection_t::mod_name()
{
  return "mini_curl_connection_t";
}

int net_mobilewebprint::mini_curl_connection_t::pre_select(mq_pre_select_data_t * pre_select_data)
{
  if (!request_sent) {
    mq.check_tcp_write(*server);
  }

  if (!closed) {
    mq.check_tcp_read(*server);
  }

  return 0;
}

e_handle_result net_mobilewebprint::mini_curl_connection_t::_mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra)
{
  if (!mwp_assert(server)) { return unhandled; }

  e_handle_result result = unhandled;

  if (!request_sent && extra.is_tcp_writable(*server)) {
    server->send_tcp(request_payload, 'c');
    request_sent = true;
    result = handled;
  }

  int recv_result = 0;
  if (!closed && extra.is_tcp_readable(*server)) {

    recv_packet->clear();
    recv_result = server->recv_tcp(*recv_packet);
    log_v(5, "mini_curl_connection_t", "mc-conn::selected fd:%d read:%d", server->tcp_fd, recv_result);
    if (recv_result > 0) {
      packet_num   += 1;
      num_recieved += recv_result;

      byte * p_start = recv_packet->begin();

      // The first packet probably has leading zeros.  Strip them.
      if (packet_num == 0) {

        byte const * p = p_start;
        while (*p == 0 && p != recv_packet->const_end()) {
          p += 1;
        }

        // Should we shrink the payload?
        if (p == p_start) {
          // No leading zeros... do nothing
        } else if (p == recv_packet->const_end()) {
          // All leading zeros... disregard the packet
          packet_num -= 1;
          return handled;
        } else {
          // Shrink
          recv_packet->lshift(p_start, p);
          recv_packet->dump("mini_curl", "shifted", 1024);
        }
      }

      // The headers are not part of the HTTP payload -- read and remove them
      if (headers.size() == 0) {
        char const * psz = (char const *)p_start;
        if ((psz = skip_past_double_newline(psz, (char const *)recv_packet->const_end())) != NULL) {

          append_to(headers, compact(split((char const *)p_start, "\r\n", psz)));
          //log_d(1, "mc", "mini_curl::recv headers: |%s|", join(headers, "\n").c_str());

          // Put the headers into the mq
          buffer_t * packet = mq.message("_on_http_headers", 0, txn_id);
          packet->appendT(join(headers, "\r\n"));

          mq.send(packet);

          recv_packet->lshift(p_start, (byte const *)psz);
          recv_packet->dump("mini_curl", "shifted", 1024);
        }
      }

      // Create a new packet, so we dont send too many bytes into the queue
      buffer_t * packet = mq.message("_on_http_payload", 0, txn_id);
      size_t packet_header_length = packet->dsize();
      packet->append(*recv_packet);

      //printf("Sending HTTP: %d (%d recvd)\n", (int)packet->dsize(), (int)recv_packet->data_length); 

      if (packet->dsize() != packet_header_length) {
        mq.send(packet);
      }

      result = handled;

    } else if (recv_result == 0) {
      // Received zero bytes -- Closed
      mq.deregister_for_select(*server);
      server->close();
      closed = true;

      log_v(4, "", "mini-curl::close %d", (int)txn_id);
      mq.send(mq.message("_on_txn_close", 0, txn_id));

    } else {
      log_e(0, "socket", "Error: recv returned < 0: %d", recv_result);
    }
  }

  return result;
}


