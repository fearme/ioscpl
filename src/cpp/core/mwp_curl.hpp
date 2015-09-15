
#ifndef __MWP_CURL_HPP__
#define __MWP_CURL_HPP__

#include "mwp_mq.hpp"
#include "mwp_socket.hpp"

#include <string>

namespace net_mobilewebprint {

  struct curl_base_t : public mq_handler_t
  {
    controller_t &          controller;
    mq_t         &          mq;

    curl_base_t(controller_t &);

    // ----- Hooking into the select loop -----
    virtual mq_result                 initialize();

    virtual mq_result       on_select_loop_start(select_loop_start_extra_t const & extra);

    virtual mq_result              on_pre_select(pre_select_extra_t & extra);
    virtual mq_result                  on_select(select_extra_t & extra);
    virtual mq_result                 on_message(string const & name, buffer_view_t const & payload, message_extra_t & extra);

    virtual mq_result         on_select_loop_end(select_loop_end_extra_t const &   extra);
    virtual mq_result        on_select_loop_idle(select_loop_idle_extra_t const &  extra);
  };

  struct curl_mini_t : public curl_base_t
  {
    curl_mini_t(controller_t &);

    // ----- Hooking into the select loop -----
    virtual mq_result                 initialize();

    virtual mq_result       on_select_loop_start(select_loop_start_extra_t const & extra);

    virtual mq_result              on_pre_select(pre_select_extra_t & extra);
    virtual mq_result                  on_select(select_extra_t & extra);
    virtual mq_result                 on_message(string const & name, buffer_view_t const & payload, message_extra_t & extra);

    virtual mq_result         on_select_loop_end(select_loop_end_extra_t const &   extra);
    virtual mq_result        on_select_loop_idle(select_loop_idle_extra_t const &  extra);
  };

  struct curl_mini_connection_t : public mq_handler_t
  {
    curl_mini_connection_t(curl_mini_t &);

    // ----- Hooking into the select loop -----
    virtual mq_result                 initialize();

    virtual mq_result       on_select_loop_start(select_loop_start_extra_t const & extra);

    virtual mq_result              on_pre_select(pre_select_extra_t & extra);
    virtual mq_result                  on_select(select_extra_t & extra);
    virtual mq_result                 on_message(string const & name, buffer_view_t const & payload, message_extra_t & extra);

    virtual mq_result         on_select_loop_end(select_loop_end_extra_t const &   extra);
    virtual mq_result        on_select_loop_idle(select_loop_idle_extra_t const &  extra);
  };

#if HAVE_CURL
  struct curl_t : public curl_base_t
  {
    curl_t(controller_t &);

    // ----- Hooking into the select loop -----
    virtual mq_result                 initialize();

    virtual mq_result       on_select_loop_start(select_loop_start_extra_t const & extra);

    virtual mq_result              on_pre_select(pre_select_extra_t & extra);
    virtual mq_result                  on_select(select_extra_t & extra);
    virtual mq_result                 on_message(string const & name, buffer_view_t const & payload, message_extra_t & extra);

    virtual mq_result         on_select_loop_end(select_loop_end_extra_t const &   extra);
    virtual mq_result        on_select_loop_idle(select_loop_idle_extra_t const &  extra);
  };
#endif  // HAVE_CURL

};

#endif  // __MWP_CURL_HPP__


