
#include "cpl_controlled_print_api.hpp"
#include "mwp_secure_asset_printing_api.hpp"

net_mobilewebprint::controlled_print_library_api_t::controlled_print_library_api_t(secure_asset_printing_api_t * sap_)
  : sap(sap_)
{
}

net_mobilewebprint::controlled_print_library_api_t::~controlled_print_library_api_t()
{
}

// TODO: Intercept the call to start()
bool net_mobilewebprint::controlled_print_library_api_t::start(bool start_scanning, bool block)
{
  return sap->start(start_scanning, block);
}

// TODO: Intercept the call to send_job(), when the app is printing a URL
bool net_mobilewebprint::controlled_print_library_api_t::send_job(char const * url, const char * printer_ip)
{
  return sap->send_job(url, printer_ip);
}

// TODO: Intercept the call to app_send(), when the app is sending a message into mq
bool net_mobilewebprint::controlled_print_library_api_t::app_send(char const * name, char const * payload)
{
  return sap->app_send(name, payload) != 0;
}

// TODO: Intercept the call to send_full_printer_list(), when the app is requesting the full printer list
bool net_mobilewebprint::controlled_print_library_api_t::send_full_printer_list()
{
  return sap->send_full_printer_list();
}

bool net_mobilewebprint::controlled_print_library_api_t::send_immediately(string const & msg_name, string const & payload)
{
  return sap->send_immediately(msg_name, payload);
}

int net_mobilewebprint::controlled_print_library_api_t::app_set_timeout(char const * message_to_send, int msecs_to_wait)
{
  return sap->app_set_timeout(message_to_send, msecs_to_wait);
}

bool net_mobilewebprint::controlled_print_library_api_t::register_handler(char const * name, void * app_data, hp_cpl_callback_t callback)
{
  // TODO: Convert callback to sap-style callback and call it
  //return sap->register_handler(name, app_data, callback);
  return false;
}

bool net_mobilewebprint::controlled_print_library_api_t::deregister_handler(char const * name)
{
  return sap->deregister_handler(name);
}

bool net_mobilewebprint::controlled_print_library_api_t::mq_is_done()
{
  return sap->mq_is_done();
}

std::string const & net_mobilewebprint::controlled_print_library_api_t::get_option(char const *name, char const *def)
{
  return sap->get_option(name, def);
}

int net_mobilewebprint::controlled_print_library_api_t::get_int_option(char const *name, int def)
{
  return sap->get_int_option(name, def);
}

bool net_mobilewebprint::controlled_print_library_api_t::get_bool_option(char const *name)
{
  return sap->get_bool_option(name);
}

void net_mobilewebprint::controlled_print_library_api_t::set_option(char const *name, char const *value)
{
  sap->set_option(name, value);
}

void net_mobilewebprint::controlled_print_library_api_t::set_option(char const *name, int value)
{
  sap->set_option(name, value);
}

void net_mobilewebprint::controlled_print_library_api_t::set_option(char const *name, bool value)
{
  sap->set_option(name, value);
}

void net_mobilewebprint::controlled_print_library_api_t::clear_option(char const *name)
{
  sap->clear_option(name);
}

void net_mobilewebprint::controlled_print_library_api_t::parse_cli(int argc, void const * argv[])
{
  sap->parse_cli(argc, argv);
}

