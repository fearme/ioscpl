
#ifndef __MWP_CORE_API_HPP__
#define __MWP_CORE_API_HPP__

#include "hp_mwp.h"
#include <string>

namespace net_mobilewebprint {

  struct controller_base_t;

  struct core_api_t
  {
    controller_base_t *    controller;

    core_api_t();
    core_api_t(bool secure);
    virtual ~core_api_t();

    // ---------- The easiest ----------
    virtual bool                   start               (bool start_scanning = true, bool block = true);
    virtual bool                   mq_is_done          ();
    virtual bool                   re_scan             ();

    virtual bool                   send_job            (char const * url, const char * printer_ip);
    virtual bool                   print               (char const * url);

//unimplemented TODO: async, so allocate_job must return txn_id
//    // ---------- More control ----------
//    virtual std::string            allocate_job        ();
//    virtual void                   set_job_param       (std::string const & job_id, char const * name, char const *value);
//    virtual void                   set_job_param       (std::string const & job_id, char const * name, int);
//    virtual void                   set_job_param       (std::string const & job_id, char const * name, bool);
//    virtual void                   set_job_param       (std::string const & job_id, char const * name, double const &);

    // ---------- Messaging with the app ----------
            bool                   register_bootstrap  (char const * name, void * app_data, hp_mwp_callback_t callback);

            bool                   register_handler    (char const * name, void * app_data, hp_mwp_callback_t callback);
    virtual bool                   deregister_handler  (char const * name);
    virtual bool                   app_send            (char const * name, char const * payload = NULL);
    virtual int                    app_set_timeout     (char const * message_to_send, int msecs_to_wait);

    // ---------- Options within the app ----------
    virtual std::string const &    get_option          (char const * name, char const *def = "");
    virtual int                    get_int_option      (char const * name, int def = 0);
    virtual bool                   get_bool_option     (char const * name, bool def = false);

    virtual void                   set_option          (char const * name, char const *value);
    virtual void                   set_option          (char const * name, int value);
    virtual void                   set_option          (char const * name, bool value = true);
    virtual void                   clear_option        (char const * name);

    virtual void                   parse_cli           (int argc, void const * argv[]);

    // ---------- Request that MWP send the printer list ----------
    virtual bool                 send_full_printer_list();
    virtual bool                 send_immediately      (std::string const & msg_name, std::string const & payload);
  };

  core_api_t * core_api();

};

#endif  // __MWP_CORE_API_HPP__
