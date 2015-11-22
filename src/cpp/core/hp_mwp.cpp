

#include "hp_mwp.h"
#include "hp_sap.h"
#include "mwp_core_api.hpp"
#include "mwp_secure_asset_printing_api.hpp"
#include "mwp_controller.hpp"

static net_mobilewebprint::core_api_t *                   mwp_api_ = NULL;
static net_mobilewebprint::secure_asset_printing_api_t  * sap_api_ = NULL;

// -------------------------------------------------------------------
// Common
net_mobilewebprint::core_api_t * net_mobilewebprint::core_api()
{
  net_mobilewebprint::mwp_assert(sap_api_ == NULL, "mwp::core_api sap_api does not exist");
  if (mwp_api_ != NULL) { return mwp_api_; }

  /* otherwise */
  return (mwp_api_ = new net_mobilewebprint::core_api_t());
}

net_mobilewebprint::secure_asset_printing_api_t * net_mobilewebprint::sap_api()
{
  net_mobilewebprint::mwp_assert(mwp_api_ == NULL, "mwp::core_api mwp_api does not exist");
  if (sap_api_ != NULL) { return sap_api_; }

  /* otherwise */
  return sap_api_ = new net_mobilewebprint::secure_asset_printing_api_t();
}

/**
 *  The api object that has been started.
 */
net_mobilewebprint::core_api_t * started_api()
{
  if (sap_api_ != NULL) { return sap_api_; }
  if (mwp_api_ != NULL) { return mwp_api_; }

  /* otherwise */
  net_mobilewebprint::mwp_assert(false, "started_api neither api started");
  return NULL;
}



// -------------------------------------------------------------------
// Mobile Web Print

static bool startup(net_mobilewebprint::core_api_t * api, uint32 flags)
{
  bool start_scanning = (flags & HP_MWP_START_SCANNING) != 0;
  bool block          = (flags & HP_MWP_BLOCK_START_FN) != 0;

  return api->start(start_scanning, block);
}

using net_mobilewebprint::core_api;
using net_mobilewebprint::sap_api;

extern "C" bool hp_mwp_start()
{
  return startup(core_api(), HP_MWP_START_APP_DEFAULT_FLAGS);
}

extern "C" bool hp_mwp_start_ex(uint32 flags)
{
  return startup(core_api(), flags);
}

extern "C" bool hp_mwp_register_bootstrap(char const * name, void * app_data, hp_mwp_callback_t callback)
{
  //printf("-------------------------------------------- here i am receiving a mwp bootstrap fn\n");
  return started_api()->register_bootstrap(name, app_data, callback);
}

extern "C" bool hp_mwp_register_handler(char const * name, void * app_data, hp_mwp_callback_t callback)
{
  return core_api()->register_handler(name, app_data, callback);
}

extern "C" bool hp_mwp_deregister_handler(char const * name)
{
  return core_api()->deregister_handler(name);
}

extern "C" bool hp_mwp_mq_is_done()
{
  return core_api()->mq_is_done();
}

extern "C" bool hp_mwp_send_job(char const * url, char const * printer_ip)
{
  return core_api()->send_job(url, printer_ip);
}

//---------------------------------------------------------------------------------------------
// setting options
//---------------------------------------------------------------------------------------------

extern "C" char const * hp_mwp_get_option(char const *name, char * buffer, uint32 buf_len)
{
  ::strncpy(buffer, started_api()->get_option(name).c_str(), buf_len);
  return buffer;
}

extern "C" char const * hp_mwp_get_option_def(char const *name, char const *def, char * buffer, uint32 buf_len)
{
  ::strncpy(buffer, started_api()->get_option(name, def).c_str(), buf_len);
  return buffer;
}

extern "C" int hp_mwp_get_int_option(char const *name)
{
  return started_api()->get_int_option(name);
}

extern "C" int hp_mwp_get_int_option_def(char const *name, int def)
{
  return started_api()->get_int_option(name, def);
}

extern "C" bool hp_mwp_get_flag(char const *name)
{
  return started_api()->get_bool_option(name);
}

extern "C" int hp_mwp_set_option(char const *name, char const *value)
{
  started_api()->set_option(name, value);
  return 1;
}

extern "C" int hp_mwp_set_int_option(char const *name, int value)
{
  started_api()->set_option(name, value);
  return 1;
}

extern "C" int hp_mwp_set_flag(char const *name, bool value)
{
  started_api()->set_option(name, value);
  return 1;
}

extern "C" int hp_mwp_clear_flag(char const *name)
{
  started_api()->set_option(name, false);
  return 1;
}

extern "C" int  hp_mwp_parse_cli(int argc, void const * argv[])
{
  started_api()->parse_cli(argc, argv);
  return 1;
}

//---------------------------------------------------------------------------------------------

extern "C" int  hp_mwp_send(char const *message, char const *payload)
{
  core_api()->app_send(message, payload);
  return 1;
}

extern "C" bool hp_mwp_send_full_printer_list()
{
  core_api()->send_full_printer_list();
  return true;
}

extern "C" bool hp_mwp_send_immediately(char const *message, char const *payload)
{
  return started_api()->send_immediately(message, payload);
}

extern "C" int hp_mwp_set_timeout(char const * message_to_send, int msecs_to_wait)
{
  return core_api()->app_set_timeout(message_to_send, msecs_to_wait);
}

extern "C" bool hp_mwp_print(char const * url)
{
  return false;
}

extern "C" void hp_mwp_stop()
{
  delete mwp_api_;
  mwp_api_ = NULL;
}


// -------------------------------------------------------------------
// Secure Asset Printing specific items

static bool startup(net_mobilewebprint::secure_asset_printing_api_t * api, uint32 flags)
{
  bool start_scanning = (flags & HP_SAP_START_SCANNING) != 0;
  bool block          = (flags & HP_SAP_BLOCK_START_FN) != 0;

  return api->start(start_scanning, block);
}

extern "C" bool hp_sap_start()
{
  return startup(sap_api(), HP_MWP_START_APP_DEFAULT_FLAGS);
}

extern "C" bool hp_sap_start_ex(uint32 flags_)
{
  // Force-clear any flags that are not part of Secure Asset Printing
  uint32 flags = flags_ & ~HP_MWP_NON_SAP;

  return startup(sap_api(), flags);
}

extern "C" bool hp_sap_register_bootstrap(char const * name, void * app_data, hp_sap_callback_t callback)
{
  //printf("-------------------------------------------- here i am receiving a sap bootstrap fn\n");
  return sap_api()->register_bootstrap(name, app_data, callback);
}

extern "C" bool hp_sap_register_handler(char const * name, void * app_data, hp_sap_callback_t callback)
{
  return sap_api()->register_handler(name, app_data, callback);
}

extern "C" bool hp_sap_deregister_handler(char const * name)
{
  return sap_api()->deregister_handler(name);
}

extern "C" bool hp_sap_mq_is_done()
{
  return sap_api()->mq_is_done();
}

extern "C" bool hp_sap_send_job(char const * url, char const * printer_ip)
{
  return sap_api()->send_job(url, printer_ip);
}

extern "C" char const * hp_sap_get_option(char const *name, char * buffer, uint32 buf_len)
{
  ::strncpy(buffer, sap_api()->get_option(name).c_str(), buf_len);
  return buffer;
}

extern "C" char const * hp_sap_get_option_def(char const *name, char const *def, char * buffer, uint32 buf_len)
{
  ::strncpy(buffer, sap_api()->get_option(name, def).c_str(), buf_len);
  return buffer;
}

//---------------------------------------------------------------------------------------------
// setting options
//---------------------------------------------------------------------------------------------

extern "C" int hp_sap_get_int_option(char const *name)
{
  return sap_api()->get_int_option(name);
}

extern "C" int hp_sap_get_int_option_def(char const *name, int def)
{
  return sap_api()->get_int_option(name, def);
}

extern "C" bool hp_sap_get_flag(char const *name)
{
  return sap_api()->get_bool_option(name);
}

extern "C" int hp_sap_set_option(char const *name, char const *value)
{
  sap_api()->set_option(name, value);
  return 1;
}

extern "C" int hp_sap_set_int_option(char const *name, int value)
{
  sap_api()->set_option(name, value);
  return 1;
}

extern "C" int hp_sap_set_flag(char const *name, bool value)
{
  sap_api()->set_option(name, value);
  return 1;
}

extern "C" int hp_sap_clear_flag(char const *name)
{
  sap_api()->set_option(name, false);
  return 1;
}

extern "C" int  hp_sap_parse_cli(int argc, void const * argv[])
{
  sap_api()->parse_cli(argc, argv);
  return 1;
}
//---------------------------------------------------------------------------------------------

extern "C" int  hp_sap_send(char const *message, char const *payload)
{
  sap_api()->app_send(message, payload);
  return 1;
}

extern "C" bool hp_sap_send_full_printer_list()
{
  sap_api()->send_full_printer_list();
  return true;
}

extern "C" bool hp_sap_send_immediately(char const *message, char const *payload)
{
  return sap_api()->send_immediately(message, payload);
}

extern "C" int hp_sap_set_timeout(char const * message_to_send, int msecs_to_wait)
{
  return sap_api()->app_set_timeout(message_to_send, msecs_to_wait);
}

extern "C" bool hp_sap_print(char const * url)
{
  return hp_mwp_print(url);
}

extern "C" void hp_sap_stop()
{
  delete sap_api_;
  sap_api_ = NULL;
}


