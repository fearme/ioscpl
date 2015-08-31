
#include "mwp_controller.hpp"
#include "mwp_printer.hpp"
#include "mwp_assert.hpp"

using namespace net_mobilewebprint;
using net_mobilewebprint::mq_result;

using std::string;

//---------------------------------------------------------------------------------------------------
//------------------------------------- printer_manager_t -------------------------------------------
//---------------------------------------------------------------------------------------------------

net_mobilewebprint::printer_manager_t::printer_manager_t(controller_t & controller_)
  : controller(controller_), mq(controller_.mq)
{
  name_ = "printer_list";

  mq.register_handler(*this);
}

printer_t * net_mobilewebprint::printer_manager_t::find_by_key(string const & key, string const & value)
{
  printer_t * result = NULL;

  for (int i = 0; i < printer_list.size(); ++i) {
    if (printer_list[i].matches(key, value)) {
      return &printer_list[i];
    }
  }

  return result;
}

printer_t * net_mobilewebprint::printer_manager_t::create_by_key(string const & key, string const & value)
{
  printer_t * result = NULL;
  size_t      index  = printer_list.size();

  printer_list.push_back(printer_t(*this));

  return result = &printer_list[index];
}

mq_result net_mobilewebprint::printer_manager_t::initialize()
{
  return not_impl;
}

mq_result net_mobilewebprint::printer_manager_t::on_select_loop_start(select_loop_start_extra_t const & extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::printer_manager_t::on_pre_select(pre_select_extra_t & extra)
{
  return ok;
}

mq_result net_mobilewebprint::printer_manager_t::on_select(select_extra_t & extra)
{
  return ok;
}

mq_result net_mobilewebprint::printer_manager_t::on_message(string const & name_, buffer_view_t const & payload, message_extra_t & extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::printer_manager_t::on_select_loop_end(select_loop_end_extra_t const &   extra)
{
  return not_impl;
}

mq_result net_mobilewebprint::printer_manager_t::on_select_loop_idle(select_loop_idle_extra_t const &  extra)
{
  return not_impl;
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- printer_t ---------------------------------------------------
//---------------------------------------------------------------------------------------------------

net_mobilewebprint::printer_t::printer_t(printer_manager_t & manager)
  : controller(manager.controller)
{
}

void net_mobilewebprint::printer_t::set(string const & key, string const & value)
{
  _add(properties, key, value);
}

void net_mobilewebprint::printer_t::set(string const & key, uint16 value)
{
  _add(properties, key, value);
}

bool net_mobilewebprint::printer_t::matches(string const & key, string const & value)
{
  if (!_has(properties, key)) {
    // No use looking any further
    return false;
  }

  /* otherwise */
  return _get(properties, key) == value;
}

void check_attr(printer_t const & printer, char const * key, strset & needed_keys)
{
  if (!_has(printer.properties, key)) {
    if (_has(printer.property_times, key) && _time_since(printer.property_times.find(key)->second) < 500) {
      // It has not been long enough since we tried to get this
      return;
    }

    needed_keys.insert(key);
  }
}

/**
 *  Introspects itself and determines which attributes it still needs to get.
 */
void net_mobilewebprint::printer_t::auto_update()
{
  strset needed_keys;

  check_attr(*this, "ip", needed_keys);

  if (needed_keys.size() > 0) {
    controller.get_attributes(*this, needed_keys);
  }
}

