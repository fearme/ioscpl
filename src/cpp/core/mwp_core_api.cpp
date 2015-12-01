
#include "mwp_core_api.hpp"
#include "mwp_controller.hpp"

//struct net_mobilewebprint::hp_sap_callback_t;

net_mobilewebprint::core_api_t::core_api_t()
  : controller(new controller_base_t((mwp_app_callback_t*)NULL))
{
}

net_mobilewebprint::core_api_t::core_api_t(bool secure)
  : controller(new controller_base_t((sap_app_callback_t*)NULL))
{
}

net_mobilewebprint::core_api_t::~core_api_t()
{
}

bool net_mobilewebprint::core_api_t::start(bool start_scanning, bool block)
{
  return controller->start(start_scanning, block);
}

bool net_mobilewebprint::core_api_t::mq_is_done()
{
  return controller->mq_is_done();
}

bool net_mobilewebprint::core_api_t::re_scan()
{
  controller->printers.re_scan();
  return true;
}

bool net_mobilewebprint::core_api_t::send_job(char const * url, const char * printer_ip)
{
  if (printer_ip == NULL) {
    return controller->send_job(url, "");
  }

  /* otherwise */
  return controller->send_job(url, printer_ip);
}

bool net_mobilewebprint::core_api_t::print(char const * url)
{
  return controller->print(url);
}

bool net_mobilewebprint::core_api_t::register_bootstrap(char const * name, void * app_data, hp_mwp_callback_t callback)
{
  return controller->register_bootstrap(name, app_data, callback);
}

bool net_mobilewebprint::core_api_t::register_handler(char const * name, void * app_data, hp_mwp_callback_t callback)
{
  return controller->register_handler(name, app_data, callback);
}

bool net_mobilewebprint::core_api_t::deregister_handler(char const * name)
{
  return controller->deregister_handler(name);
}

bool net_mobilewebprint::core_api_t::register_hf_handler(char const * name, void * app_data, hp_mwp_hf_callback_t callback)
{
  return controller->register_hf_handler(name, app_data, callback);
}

bool net_mobilewebprint::core_api_t::app_send(char const * name, char const * payload)
{
  return controller->app_send(name, payload) != 0;
}

bool net_mobilewebprint::core_api_t::send_full_printer_list()
{
  return controller->send_full_printer_list();
}

bool net_mobilewebprint::core_api_t::send_immediately(string const & msg_name, string const & payload)
{
  return controller->send_immediately(msg_name, payload);
}

int net_mobilewebprint::core_api_t::app_set_timeout(char const * message_to_send, int msecs_to_wait)
{
  return controller->app_set_timeout(message_to_send, msecs_to_wait);
}

std::string const & net_mobilewebprint::core_api_t::get_option(char const *name, char const *def)
{
  if (def == NULL) { return controller->arg(name); }

  /* otherwise */
  return controller->arg(name, def);
}

int net_mobilewebprint::core_api_t::get_int_option(char const *name, int def)
{
  return controller->arg(name, def);
}

bool net_mobilewebprint::core_api_t::get_bool_option(char const *name, bool def)
{
  return controller->flag(name, def);
}

void net_mobilewebprint::core_api_t::set_option(char const *name, char const *value)
{
  controller->set_arg(name, value);
}

void net_mobilewebprint::core_api_t::set_option(char const *name, int value)
{
  controller->set_arg(name, value);
}

void net_mobilewebprint::core_api_t::set_option(char const *name, bool value)
{
  controller->set_flag(name, value);
}

void net_mobilewebprint::core_api_t::clear_option(char const *name)
{
  controller->clear_flag(name);
}

void net_mobilewebprint::core_api_t::parse_cli(int argc, void const * argv[])
{
  controller->parse_cli(argc, argv);
}

