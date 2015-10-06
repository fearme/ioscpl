
#ifndef __CPL_API_HPP__
#define __CPL_API_HPP__

#include "hp_cpl.h"
#include "mwp_secure_asset_printing_api.hpp"
#include <string>

namespace net_mobilewebprint {

  using std::string;

  struct controlled_print_library_api_t
  {
    secure_asset_printing_api_t * sap;

    controlled_print_library_api_t(secure_asset_printing_api_t * sap);
    virtual ~controlled_print_library_api_t();

    virtual bool start(bool start_scanning = true, bool block = true);
    virtual bool mq_is_done();

    virtual bool send_job(char const * url, const char * printer_ip);

    // ---------- Messaging with the app ----------
    virtual bool register_handler(char const * name, void * app_data, hp_cpl_callback_t callback);
    virtual bool deregister_handler(char const * name);
    virtual bool app_send(char const * name, char const * payload = NULL);
    virtual int  app_set_timeout(char const * message_to_send, int msecs_to_wait);

    virtual bool send_full_printer_list();
    virtual bool send_immediately(std::string const & msg_name, std::string const & payload);

    virtual string const &                    get_option          (char const *name, char const *def = "");
    virtual int                               get_int_option      (char const *name, int def = 0);
    virtual bool                              get_bool_option     (char const *name);

    virtual void                              set_option          (char const *name, char const *value);
    virtual void                              set_option          (char const *name, int value);
    virtual void                              set_option          (char const *name, bool value = true);
    virtual void                            clear_option          (char const *name);

    virtual void                               parse_cli          (int argc, void const * argv[]);
  };

  controlled_print_library_api_t * cpl_api();

};

#endif // __CPL_API_HPP__
