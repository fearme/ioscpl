
#include "mwp_types.hpp"
#include "mwp_send_tcp_job.hpp"
#include "mwp_controller.hpp"

#include <cstdlib>

using namespace net_mobilewebprint::msg;
using namespace net_mobilewebprint::mq_enum;
using namespace net_mobilewebprint::tags;

using net_mobilewebprint::network_node_t;
using net_mobilewebprint::buffer_view_i;
using net_mobilewebprint::buffer_t;

/**
 *  send_tcp_job_t ctor.
 */
net_mobilewebprint::send_tcp_job_t::send_tcp_job_t(controller_base_t & controller_)
  : controller(controller_), mq(controller_.mq)
{
  mq.on(this);
  mq.on_selected(this);
}

std::string net_mobilewebprint::send_tcp_job_t::mod_name()
{
  return "send_tcp_job_t";
}

net_mobilewebprint::tcp_job_connection_t * net_mobilewebprint::send_tcp_job_t::send_to_printer(uint32 connection_id, string const & ip, uint16 port)
{
  network_node_t * printer = new network_node_t(ip.c_str(), port);

  if (mwp_assert(printer)) {
    if (printer->connect() == 0) {
      log_v(2, "", "!!!!!!!!!!!!!!!!!! Opening socket to printer %s:%d failed -- errno: %d", ip.c_str(), (int)port, (int)printer->last_error);
      return NULL;
    }

    /* otherwise */
    tcp_job_connection_t * connection = new tcp_job_connection_t(controller, printer, connection_id);
    connections[connection_id] = connection;
    return connection;
  }

  /* otherwise */
  return NULL;
}

int net_mobilewebprint::send_tcp_job_t::pre_select(mq_pre_select_data_t * pre_select_data)
{
  tcp_job_connection_t * connection = NULL;
  connections_t::iterator it;
  for (it = connections.begin(); it != connections.end(); ++it) {
    if ((connection = it->second) != NULL) {
      connection->pre_select(pre_select_data);
    }
  }

  return 0;
}

e_handle_result net_mobilewebprint::send_tcp_job_t::_mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra)
{
  e_handle_result result = unhandled;
  connections_t::iterator it;
  tcp_job_connection_t * connection = NULL;

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

e_handle_result net_mobilewebprint::send_tcp_job_t::handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  e_handle_result result = unhandled;
  connections_t::iterator it;
  tcp_job_connection_t * connection = NULL;

  if (name == "_on_http_payload" || name == "_on_txn_close") {

    it = connections.find(extra.txn_id);
    if (it != connections.end()) {
      connection = it->second;

      if (connection != NULL) {
        result = connection->handle(name, payload, data, extra);

        if (connection_is_closed(it, extra.txn_id)) {
          return result;
        }
      }
    }
  }

  return result;
}

bool net_mobilewebprint::send_tcp_job_t::connection_is_closed(connections_t::iterator & it, int txn_id)
{
  tcp_job_connection_t const * connection = it->second;

  if (connection != NULL && connection->closed) {
    connections.erase(it);
    controller.job_stat(connection->connection_id, "byte_stream_done", true);
    delete connection;
    return true;
  }

  return false;
}


//----------------------------------------------------------------------------
//
//  The connection
//
//----------------------------------------------------------------------------

/**
 *  tcp_job_connection_t ctor.
 */
net_mobilewebprint::tcp_job_connection_t::tcp_job_connection_t(controller_base_t & controller_, network_node_t * connection_, uint32 id_)
  : controller(controller_), mq(controller_.mq), printer(connection_), connection_id(id_), txn_closed(false), closed(false), packet_num(-1)
{
  //  mq.register_tcp_for_select(*printer);       // TODO: We need to register, or connect, but do not want write events until we have data to send
}

net_mobilewebprint::tcp_job_connection_t::~tcp_job_connection_t()
{
  delete printer;
  printer = NULL;
}

std::string net_mobilewebprint::tcp_job_connection_t::mod_name()
{
  return "tcp_job_connection_t";
}

int net_mobilewebprint::tcp_job_connection_t::pre_select(mq_pre_select_data_t * pre_select_data)
{
  if (chunks.size() > 0 && !closed) {
    mq.check_tcp_write(*printer);
  }

  if (!closed) {
    mq.check_tcp_read(*printer);
  }

  return 0;
}

e_handle_result net_mobilewebprint::tcp_job_connection_t::handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  if (name == "_on_http_payload")       { return _on_http_payload(name, payload, data, extra); }
  if (name == "_on_txn_close")          { return _on_txn_close(name, payload, data, extra); }

  return not_impl;
}

e_handle_result net_mobilewebprint::tcp_job_connection_t::_mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra)
{
  if (!mwp_assert(printer)) { return unhandled; }

  if (!mq.is_fd_part_of_select(*printer)) {
    return unhandled;
  }

  e_handle_result result = unhandled;

  if (chunks.size() > 0) {
    if (extra.is_tcp_writable(*printer)) {
      packet_num += 1;

      chunk_t * chunk = chunks.front();
      chunks.pop_front();

      chunk->data->dump("send_tcp_job", "sending to printer", 1024);

      int num_sent = 0, num_to_send = (int)chunk->view.dsize();
      num_sent = printer->send_tcp(chunk->view, 's');

      if (num_sent < 0) {
        controller.printers.network_error(printer->ip, printer->last_error);
      } else {
        controller.job_stat_incr(connection_id, "totalSent", num_sent);

        log_v(4, TAG_DATA_FLOW, "send_job(%d)::write: %d / %d(%d), %d chunks remaining", printer->tcp_fd, num_sent, (int)chunk->view.dsize(), chunk->data->mem_length, (int)chunks.size());

        if (num_sent < num_to_send) {
          chunk->view._begin += num_sent;
          chunks.push_front(chunk);
        } else {
          delete chunk; /**/ num_chunk_allocations -= 1;
        }
      }

      result = handled;
    }
  }

  int recv_result = 0;
  if (!closed) {
    if (extra.is_tcp_readable(*printer)) {
      buffer_t packet;
      result = handled;

      if ((recv_result = printer->recv_tcp(packet, 2 * 1024)) > 0) {
        // TODO: The printer usually does not talk back, just log this.

        //log_d("received from printer: %s:%d", sender.ip.c_str(), sender.port);
      }
    }
  }

  if (txn_closed && chunks.size() == 0) {
    log_v("Closing printer node %d\n", connection_id);
    mq.deregister_for_select(*printer);
    printer->close();
    closed = true;
  }

  return result;
}

e_handle_result net_mobilewebprint::tcp_job_connection_t::_on_http_payload(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  log_v("http payload(%d): %d bytes\n", extra.txn_id, (int)payload.dsize());
  chunks.push_back(new chunk_t(data, payload)); /**/ num_chunk_allocations += 1;

  controller.job_stat_incr(connection_id, "numDownloaded", (int)payload.dsize());

  return handled_and_took_message;    // We now have ownership of the data memory
}

e_handle_result net_mobilewebprint::tcp_job_connection_t::_on_txn_close(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  txn_closed = true;

  log_v("############################# At close time, we had %d chunks remaining\n", (int)chunks.size());

  if (chunks.size() == 0) {
    buffer_view_i::const_iterator p = payload.first();
    uint32 curl_status = payload.read_uint32(p);
    long http_code = payload.read_long(p);
    if(curl_status != CURL_NO_ERROR){
      controller.printers.network_error(printer->ip, curl_status);
    } else if(http_code == HTTP_CODE_498) {
      controller.printers.upstream_error(printer->ip, http_code);
    }
    mq.deregister_for_select(*printer);
    printer->close();
    closed = true;
  }

  return handled;
}

