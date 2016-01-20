

#include "hp_cpl.h"
#include "cpl_controlled_print_api.hpp"

static net_mobilewebprint::controlled_print_library_api_t *                   cpl_api_ = NULL;

using std::string;
using net_mobilewebprint::sap_api;

net_mobilewebprint::controlled_print_library_api_t * net_mobilewebprint::cpl_api()
{
  if (cpl_api_ != NULL) { return cpl_api_; }

  /* otherwise */
  return cpl_api_ = new net_mobilewebprint::controlled_print_library_api_t(sap_api());
}

using net_mobilewebprint::cpl_api;

static bool startup(net_mobilewebprint::controlled_print_library_api_t * api, uint32 flags)
{
  bool start_scanning = (flags & HP_CPL_START_SCANNING) != 0;
  bool block          = (flags & HP_CPL_BLOCK_START_FN) != 0;

  return api->start(start_scanning, block);
}

extern "C" bool hp_cpl_start()
{
  hp_sap_start();
  return startup(cpl_api(), HP_CPL_START_APP_DEFAULT_FLAGS);
}

extern "C" bool hp_cpl_start_ex(uint32 flags_)
{
  hp_sap_start();
  return startup(cpl_api(), flags_);
}

extern "C" bool hp_cpl_register_bootstrap(char const * name, void * app_data, hp_cpl_callback_t callback)
{
  return cpl_api()->register_bootstrap(name, app_data, callback);
}

extern "C" bool hp_cpl_register_handler(char const * name, void * app_data, hp_cpl_callback_t callback)
{
  return cpl_api()->register_handler(name, app_data, callback);
}

extern "C" bool hp_cpl_deregister_handler(char const * name)
{
  return cpl_api()->deregister_handler(name);
}

extern "C" bool hp_cpl_register_hf_handler(char const * name, void * app_data, hp_cpl_hf_callback_t callback)
{
  return cpl_api()->register_hf_handler(name, app_data, callback);
}

extern "C" bool hp_cpl_mq_is_done()
{
  return cpl_api()->mq_is_done();
}

extern "C" bool hp_cpl_re_scan()
{
  return cpl_api()->re_scan();
}

extern "C" bool hp_cpl_send_job(char const * url, char const * printer_ip)
{
  return cpl_api()->send_job(url, printer_ip);
}

extern "C" char const * hp_cpl_get_option(char const *name, char * buffer, uint32 buf_len)
{
  ::strncpy(buffer, cpl_api()->get_option(name).c_str(), buf_len);
  return buffer;
}

extern "C" char const * hp_cpl_get_option_def(char const *name, char const *def, char * buffer, uint32 buf_len)
{
  ::strncpy(buffer, cpl_api()->get_option(name, def).c_str(), buf_len);
  return buffer;
}

extern "C" int hp_cpl_get_int_option(char const *name)
{
  return cpl_api()->get_int_option(name);
}

extern "C" int hp_cpl_get_int_option_def(char const *name, int def)
{
  return cpl_api()->get_int_option(name, def);
}

extern "C" bool hp_cpl_get_flag(char const *name)
{
  return cpl_api()->get_bool_option(name);
}

extern "C" bool hp_cpl_get_flag_def(char const *name, bool def = false)
{
  return cpl_api()->get_bool_option(name, def);
}

extern "C" int hp_cpl_set_option(char const *name, char const *value)
{
  cpl_api()->set_option(name, value);
  return 1;
}

extern "C" int hp_cpl_set_int_option(char const *name, int value)
{
  cpl_api()->set_option(name, value);
  return 1;
}

extern "C" int hp_cpl_set_flag(char const *name, bool value)
{
  cpl_api()->set_option(name, value);
  return 1;
}

extern "C" int hp_cpl_clear_flag(char const *name)
{
  cpl_api()->set_option(name, false);
  return 1;
}

extern "C" int  hp_cpl_parse_cli(int argc, void const * argv[])
{
  cpl_api()->parse_cli(argc, argv);
  return 1;
}

extern "C" int  hp_cpl_send(char const *message, char const *payload)
{
  cpl_api()->app_send(message, payload);
  return 1;
}

extern "C" bool hp_cpl_send_full_printer_list()
{
  cpl_api()->send_full_printer_list();
  return true;
}

extern "C" int hp_cpl_set_timeout(char const * message_to_send, int msecs_to_wait)
{
  return cpl_api()->app_set_timeout(message_to_send, msecs_to_wait);
}

extern "C" bool hp_cpl_print(char const * url)
{
  cpl_api()->print(url);
}

extern "C" void hp_cpl_stop()
{
  delete cpl_api_;
  cpl_api_ = NULL;
}


