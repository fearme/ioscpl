
#include "mwp_mq.hpp"
#include "mwp_host.hpp"

#include <string>

using net_mobilewebprint::mq_result;
using net_mobilewebprint::mq_t;
using net_mobilewebprint::uint32;
using net_mobilewebprint::host::get_tick_count;
using std::string;

static uint32 LOOP_START_BIT            = 0x00000010;
static uint32 ZERO_TIMEOUT_SELECT_BIT   = 0x00000020;

//---------------------------------------------------------------------------------------------------
//------------------------------------- mq_t --------------------------------------------------------
//---------------------------------------------------------------------------------------------------
net_mobilewebprint::mq_t::mq_t()
  : current_loop_num(0), current_loop_start(0), current_loop_second(0),
    loop_start_stats(NULL),
    zero_timeout_select_stats(NULL),
    current_loop_stats(NULL)
{
  loop_stats.push_back(loop_start_stats           = new mq_loop_stats_t(*this, LOOP_START_BIT));
  loop_stats.push_back(zero_timeout_select_stats  = new mq_loop_stats_t(*this, ZERO_TIMEOUT_SELECT_BIT));

  zero_timeout.tv_sec   = 0;
  zero_timeout.tv_usec  = 0;
  long_timeout.tv_sec   = 0;
  long_timeout.tv_usec  = 0;
}

static void * _thread_start_fn(void * pvthat)
{
  mq_t* that = (mq_t*)pvthat;
  that->dispatch_loop();
  return NULL;
}

bool net_mobilewebprint::mq_t::run()
{
  if (host::start_thread(_thread_start_fn, this)) {
    return true;
  }

  /* otherwise */
  _thread_start_fn(this);
  return true;
}

bool net_mobilewebprint::mq_t::is_done()
{
  return false;
}

void net_mobilewebprint::mq_t::dispatch_loop()
{
  uint32 current_loop_end   = 0;
  uint32 event_start        = 0;

  _initialize();

  for (current_loop_num = 0;; ++current_loop_num) {

    // Note: This loop has a lot of "continue" statements, so the very end of this for-loop
    //       runs much less than every time through the loop.  So, the end-of-loop processing
    //       has to be here, at the top.
    current_loop_end = get_tick_count();

    // Collect stats
    if (current_loop_num != 0) {
    }

    // ----- This is the real top-of-loop processing
    current_loop_start    = get_tick_count();
    current_loop_second   = current_loop_start * 0.001;
    event_start           = current_loop_start;

    loop_start_stats->start();
    _on_select_loop_start();

    zero_timeout_select_stats->start();
  }
}

void net_mobilewebprint::mq_t::register_handler(mq_handler_t & handler)
{
  handlers.push_back(&handler);
}

void net_mobilewebprint::mq_t::_initialize()
{
  for (deque<mq_handler_t *>::iterator it = handlers.begin(); it != handlers.end(); ++it) {
    mq_handler_t *& handler = *it;
    if (handler != NULL) {
      handler->initialize();
    }
  }
}

void net_mobilewebprint::mq_t::_on_select_loop_start()
{
  select_loop_start_extra_t extra(*this);

  for (deque<mq_handler_t *>::iterator it = handlers.begin(); it != handlers.end(); ++it) {
    mq_handler_t *& handler = *it;
    if (handler != NULL) {
      handler->on_select_loop_start(extra);
    }
  }
}

void net_mobilewebprint::mq_t::_on_zero_select()
{
  pre_select_extra_t pre_select_extra(*this);

  for (deque<mq_handler_t *>::iterator it = handlers.begin(); it != handlers.end(); ++it) {
    mq_handler_t *& handler = *it;
    if (handler != NULL) {
      handler->on_pre_select(pre_select_extra);
    }
  }

  select_extra_t extra(*this);

  for (deque<mq_handler_t *>::iterator it = handlers.begin(); it != handlers.end(); ++it) {
    mq_handler_t *& handler = *it;
    if (handler != NULL) {
      handler->on_select(extra);
    }
  }
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- mq_loop_stats_t ---------------------------------------------
//---------------------------------------------------------------------------------------------------
net_mobilewebprint::mq_loop_stats_t::mq_loop_stats_t(mq_t & mq_, uint32 bit_)
  : mq(mq_), bit(bit_), count(0), start_time(0), running_time(0), total_running_time(0)
{
}

void net_mobilewebprint::mq_loop_stats_t::start()
{
  uint32 now = start_time = get_tick_count();

  if (mq.current_loop_stats != NULL) {
    mq.current_loop_stats->stop(now);
  }
  mq.current_loop_stats = this;
}

void net_mobilewebprint::mq_loop_stats_t::stop(uint32 now)
{
  running_time += now - start_time;
}

void net_mobilewebprint::mq_loop_stats_t::reset()
{
  running_time = 0;
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- extra_t -----------------------------------------------------
//---------------------------------------------------------------------------------------------------
net_mobilewebprint::extra_t::extra_t(mq_t & mq_)
  : mq(mq_),
    current_loop_num(mq_.current_loop_num), current_loop_start(mq_.current_loop_start), current_loop_second(mq_.current_loop_second)
{
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- select_loop_start_extra_t -----------------------------------
//---------------------------------------------------------------------------------------------------
net_mobilewebprint::select_loop_start_extra_t::select_loop_start_extra_t(mq_t & mq)
  : extra_t(mq)
{
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- pre_select_extra_t ------------------------------------------
//---------------------------------------------------------------------------------------------------
net_mobilewebprint::pre_select_extra_t::pre_select_extra_t(mq_t & mq_)
  : extra_t(mq_),
    readable(mq_.readable), writable(mq_.writable), exceptional(mq_.exceptional)
{
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- select_extra_t ----------------------------------------------
//---------------------------------------------------------------------------------------------------
net_mobilewebprint::select_extra_t::select_extra_t(mq_t & mq_)
  : extra_t(mq_),
    readable(mq_.readable), writable(mq_.writable), exceptional(mq_.exceptional)
{
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- message_extra_t ---------------------------------------------
//---------------------------------------------------------------------------------------------------
net_mobilewebprint::message_extra_t::message_extra_t(mq_t & mq)
  : extra_t(mq)
{
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- select_loop_end_extra_t -------------------------------------
//---------------------------------------------------------------------------------------------------
net_mobilewebprint::select_loop_end_extra_t::select_loop_end_extra_t(mq_t & mq)
  : extra_t(mq)
{
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- select_loop_idle_extra_t ------------------------------------
//---------------------------------------------------------------------------------------------------
net_mobilewebprint::select_loop_idle_extra_t::select_loop_idle_extra_t(mq_t & mq)
  : extra_t(mq)
{
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- mq_hander_t -------------------------------------------------
//---------------------------------------------------------------------------------------------------
string const & net_mobilewebprint::mq_handler_t::name()
{
  return name_;
}

mq_result net_mobilewebprint::mq_handler_t::initialize()
{
  return not_impl;
}

mq_result net_mobilewebprint::mq_handler_t::on_select_loop_start(select_loop_start_extra_t const & extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::mq_handler_t::on_pre_select(pre_select_extra_t & extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::mq_handler_t::on_select(select_extra_t & extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::mq_handler_t::on_message(string const & name, buffer_view_t const & payload, message_extra_t & extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::mq_handler_t::on_select_loop_end(select_loop_end_extra_t const &   extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::mq_handler_t::on_select_loop_idle(select_loop_idle_extra_t const &  extra)
{
  return not_impl;
}

