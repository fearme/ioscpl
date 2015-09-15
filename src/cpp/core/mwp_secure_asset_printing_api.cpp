
#include "mwp_secure_asset_printing_api.hpp"
#include "mwp_controller.hpp"

net_mobilewebprint::secure_asset_printing_api_t::secure_asset_printing_api_t()
  : core_api_t(true)
{
}

net_mobilewebprint::secure_asset_printing_api_t::~secure_asset_printing_api_t()
{
}

bool net_mobilewebprint::secure_asset_printing_api_t::start(bool start_scanning, bool block)
{
  return core_api_t::start(start_scanning, block);
}

bool net_mobilewebprint::secure_asset_printing_api_t::mq_is_done()
{
  return core_api_t::mq_is_done();
}

bool net_mobilewebprint::secure_asset_printing_api_t::send_job(char const * url, const char * printer_ip)
{
  return core_api_t::send_job(url, printer_ip);
}

bool net_mobilewebprint::secure_asset_printing_api_t::register_handler(char const * name, void * app_data, hp_sap_callback_t callback)
{
  return controller->register_handler(name, app_data, callback);
}

bool net_mobilewebprint::secure_asset_printing_api_t::deregister_handler(char const * name)
{
  return core_api_t::deregister_handler(name);
}

bool net_mobilewebprint::secure_asset_printing_api_t::app_send(char const * name, char const * payload)
{
  return core_api_t::app_send(name, payload) != 0;
}

bool net_mobilewebprint::secure_asset_printing_api_t::send_full_printer_list()
{
  return core_api_t::send_full_printer_list();
}

int net_mobilewebprint::secure_asset_printing_api_t::app_set_timeout(char const * message_to_send, int msecs_to_wait)
{
  return core_api_t::app_set_timeout(message_to_send, msecs_to_wait);
}

std::string const & net_mobilewebprint::secure_asset_printing_api_t::get_option(char const *name, char const *def)
{
  return core_api_t::get_option(name, def);
}

int net_mobilewebprint::secure_asset_printing_api_t::get_int_option(char const *name, int def)
{
  return core_api_t::get_int_option(name, def);
}

bool net_mobilewebprint::secure_asset_printing_api_t::get_bool_option(char const *name)
{
  return core_api_t::get_bool_option(name);
}

void net_mobilewebprint::secure_asset_printing_api_t::set_option(char const *name, char const *value)
{
  core_api_t::set_option(name, value);
}

void net_mobilewebprint::secure_asset_printing_api_t::set_option(char const *name, int value)
{
  core_api_t::set_option(name, value);
}

void net_mobilewebprint::secure_asset_printing_api_t::set_option(char const *name, bool value)
{
  core_api_t::set_option(name, value);
}

void net_mobilewebprint::secure_asset_printing_api_t::clear_option(char const *name)
{
  core_api_t::clear_option(name);
}

void net_mobilewebprint::secure_asset_printing_api_t::parse_cli(int argc, void const * argv[])
{
  core_api_t::parse_cli(argc, argv);
}

