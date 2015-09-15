
#include "mwp_curl.hpp"
#include "mwp_controller.hpp"
#include "mwp_utils.hpp"

using namespace net_mobilewebprint::msg;

/**
 *  curl_t ctor.
 */
net_mobilewebprint::curl_t::curl_t(controller_base_t & controller_, string server_name_, uint16 server_port_)
  : controller(controller_), mq(controller_.mq), server_name(server_name_), server_port(server_port_)
{
  mq.on_selected(this);
}

net_mobilewebprint::curl_t::~curl_t()
{
}

std::string net_mobilewebprint::curl_t::mod_name()
{
  return "curl_null_t";
}

net_mobilewebprint::curl_connection_t * net_mobilewebprint::curl_t::fetch_from_mwp_server(char const * verb, string const & path_, uint32 connection_id)
{
  return NULL;
}

net_mobilewebprint::curl_connection_t * net_mobilewebprint::curl_t::post_mwp_server(serialization_json_t const & json, string const & path, uint32 connection_id)
{
  return NULL;
}

net_mobilewebprint::curl_connection_t * net_mobilewebprint::curl_t::post_mwp_server(string const & json, string const & path, uint32 connection_id)
{
  return NULL;
}

int net_mobilewebprint::curl_t::pre_select(mq_pre_select_data_t * data)
{
  return 0;
}

net_mobilewebprint::e_handle_result net_mobilewebprint::curl_t::_mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra)
{
  e_handle_result result = unhandled;

  return result;
}

net_mobilewebprint::e_handle_result net_mobilewebprint::curl_t::handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  e_handle_result result = unhandled;

  return result;
}
