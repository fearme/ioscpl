
#ifndef __MWP_SECURE_ASSET_PRINTING_HPP__
#define __MWP_SECURE_ASSET_PRINTING_HPP__

#include "hp_sap.h"
#include "mwp_core_api.hpp"

namespace net_mobilewebprint {

  struct secure_asset_printing_api_t : public core_api_t
  {

    secure_asset_printing_api_t();
    virtual ~secure_asset_printing_api_t();

    virtual bool                   start(bool start_scanning = true, bool block = true);
    virtual bool                   mq_is_done();

    virtual bool                   send_job            (char const * url, const char * printer_ip);

    // ---------- Messaging with the app ----------
    virtual bool                   register_handler    (char const * name, void * app_data, hp_sap_callback_t callback);
    virtual bool                   deregister_handler  (char const * name);
    virtual bool                   app_send            (char const * name, char const * payload = NULL);
    virtual int                    app_set_timeout     (char const * message_to_send, int msecs_to_wait);

    // ---------- Options within the app ----------
    virtual std::string const &    get_option          (char const * name, char const *def = "");
    virtual int                    get_int_option      (char const * name, int def = 0);
    virtual bool                   get_bool_option     (char const * name);

    virtual void                   set_option          (char const * name, char const *value);
    virtual void                   set_option          (char const * name, int value);
    virtual void                   set_option          (char const * name, bool value = true);
    virtual void                   clear_option        (char const * name);

    virtual void                   parse_cli           (int argc, void const * argv[]);

    // ---------- Request that SAP send the printer list ----------
    virtual bool                 send_full_printer_list();
  };

  secure_asset_printing_api_t * sap_api();

};

#endif // __MWP_SECURE_ASSET_PRINTING_HPP__
