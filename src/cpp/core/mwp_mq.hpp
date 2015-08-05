
#ifndef __MWP_MQ_HPP__
#define __MWP_MQ_HPP__

#include <mwp_buffer.hpp>
#include <string>

namespace net_mobilewebprint {

  using std::string;

  enum mq_result
  {
    not_impl            = 0,
    ok                  = 1,
    ok_and_took_memory
  };

  struct select_loop_start_data_t
  {
  };

  struct select_extra_t
  {
  };

  struct message_extra_t
  {
  };

  struct select_loop_end_data_t
  {
  };

  struct select_loop_idle_data_t
  {
  };

  struct mq_handler_t
  {
    virtual string const &                  name() = 0;

    virtual mq_result       on_select_loop_start(select_loop_start_data_t const & data);

    virtual mq_result                  on_select(string const & name, buffer_view_t const & payload, select_extra_t & extra);
    virtual mq_result                 on_message(string const & name, buffer_view_t const & payload, message_extra_t & extra);

    virtual mq_result         on_select_loop_end(select_loop_end_data_t const &   data);
    virtual mq_result        on_select_loop_idle(select_loop_idle_data_t const &  data);
  };

  struct mq
  {
  };

};

#endif // __MWP_MQ_HPP__

