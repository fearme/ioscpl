
#include "mwp_controller.hpp"
#include "mwp_curl.hpp"
#include "mwp_utils.hpp"
#include "mwp_assert.hpp"

using namespace net_mobilewebprint;
using net_mobilewebprint::mq_result;

using std::string;

//---------------------------------------------------------------------------------------------------
//------------------------------------- curl_base_t ------------------------------------------------------
//---------------------------------------------------------------------------------------------------

net_mobilewebprint::curl_base_t::curl_base_t(controller_t & controller_)
  : controller(controller_), mq(controller_.mq)
{
  mq.register_handler(*this);
}

mq_result net_mobilewebprint::curl_base_t::initialize()
{
  return not_impl;
}

mq_result net_mobilewebprint::curl_base_t::on_select_loop_start(select_loop_start_extra_t const & extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::curl_base_t::on_pre_select(pre_select_extra_t & extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::curl_base_t::on_select(select_extra_t & extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::curl_base_t::on_message(string const & name_, buffer_view_t const & payload, message_extra_t & extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::curl_base_t::on_select_loop_end(select_loop_end_extra_t const &   extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::curl_base_t::on_select_loop_idle(select_loop_idle_extra_t const &  extra)
{
  return not_impl;
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- curl_mini_t ------------------------------------------------------
//---------------------------------------------------------------------------------------------------

net_mobilewebprint::curl_mini_t::curl_mini_t(controller_t & controller)
  : curl_base_t(controller)
{
  name_ = "curl_mini";
}

mq_result net_mobilewebprint::curl_mini_t::initialize()
{
  return not_impl;
}

mq_result net_mobilewebprint::curl_mini_t::on_select_loop_start(select_loop_start_extra_t const & extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::curl_mini_t::on_pre_select(pre_select_extra_t & extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::curl_mini_t::on_select(select_extra_t & extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::curl_mini_t::on_message(string const & name_, buffer_view_t const & payload, message_extra_t & extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::curl_mini_t::on_select_loop_end(select_loop_end_extra_t const &   extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::curl_mini_t::on_select_loop_idle(select_loop_idle_extra_t const &  extra)
{
  return not_impl;
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- curl_t ------------------------------------------------------
//---------------------------------------------------------------------------------------------------

#if HAVE_CURL
net_mobilewebprint::curl_t::curl_t(controller_t & controller)
  : curl_base_t(controller)
{
  name_ = "curl";
}

mq_result net_mobilewebprint::curl_t::initialize()
{
  return not_impl;
}

mq_result net_mobilewebprint::curl_t::on_select_loop_start(select_loop_start_extra_t const & extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::curl_t::on_pre_select(pre_select_extra_t & extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::curl_t::on_select(select_extra_t & extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::curl_t::on_message(string const & name_, buffer_view_t const & payload, message_extra_t & extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::curl_t::on_select_loop_end(select_loop_end_extra_t const &   extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::curl_t::on_select_loop_idle(select_loop_idle_extra_t const &  extra)
{
  return not_impl;
}
#endif  // HAVE_CURL


#if 0
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
#endif


