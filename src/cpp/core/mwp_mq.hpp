
#ifndef __MWP_MQ_HPP__
#define __MWP_MQ_HPP__

#include "mwp_buffer.hpp"
#include <string>
#include <deque>

namespace net_mobilewebprint {

  using std::string;
  using std::deque;

  enum mq_result
  {
    not_impl            = 0,
    ok                  = 1,
    ok_and_took_memory
  };

  struct mq_t;

  struct extra_t
  {
    mq_t & mq;

    int     current_loop_num;
    uint32  current_loop_start;
    uint32  current_loop_second;

    extra_t(mq_t &);
  };

  struct select_loop_start_extra_t : public extra_t
  {
    select_loop_start_extra_t(mq_t &);
  };

  struct select_extra_base_t : public extra_t
  {
    fd_set & readable;
    fd_set & writable;
    fd_set & exceptional;

    select_extra_base_t(mq_t &);

    void fd_set_readable(int fd);
    void fd_set_writable(int fd);

    bool is_readable(int fd);
    bool is_writable(int fd);
    bool is_exceptional(int fd);
  };

  struct pre_select_extra_t : public select_extra_base_t
  {
    pre_select_extra_t(mq_t &);
  };

  struct select_extra_t : public select_extra_base_t
  {
    select_extra_t(mq_t &);
  };

  struct message_extra_t : public extra_t
  {
    buffer_t *        data;
    string            name;
    uint32            id;
    buffer_range_t *  payload;

    message_extra_t(mq_t &);
  };

  struct select_loop_end_extra_t : public extra_t
  {
    select_loop_end_extra_t(mq_t &);
  };

  struct select_loop_idle_extra_t : public extra_t
  {
    select_loop_idle_extra_t(mq_t &);
  };

  struct mq_handler_t
  {
    string                  name_;
    deque<buffer_t *>       requests;

    virtual ~mq_handler_t();

    virtual string const &                  name();

    virtual mq_result                 initialize();

    // To support the select loop
    virtual mq_result       on_select_loop_start(select_loop_start_extra_t const & extra);

    virtual mq_result              on_pre_select(pre_select_extra_t & extra);
    virtual mq_result                  on_select(select_extra_t & extra);
    virtual mq_result                 on_message(string const & name, buffer_view_t const & payload, message_extra_t & extra);

    virtual mq_result         on_select_loop_end(select_loop_end_extra_t const &   extra);
    virtual mq_result        on_select_loop_idle(select_loop_idle_extra_t const &  extra);

    // Helper functions
    virtual bool                     should_send();
    virtual void                    push_request(buffer_t * req);
    virtual buffer_t *         pull_next_request();
  };

  struct mq_loop_stats_t
  {
    mq_t &  mq;

    uint32  bit;
    int     count;
    uint32  start_time;
    uint32  running_time;
    uint32  total_running_time;

    mq_loop_stats_t(mq_t &, uint32 bit_);

    void start();
    void stop(uint32 now);
    void reset();
  };

  struct mq_t
  {
    //-----------------------------------------
    int       current_loop_num;
    uint32    current_loop_start;
    uint32    current_loop_second;

    //-----------------------------------------
    struct timeval zero_timeout;
    struct timeval long_timeout;

    //-----------------------------------------
    deque<mq_handler_t *>     handlers;
    deque<buffer_t *>         messages;

    void *                    messages_lock;

    //-----------------------------------------
    mq_t();

    bool run();
    bool is_done();
    void dispatch_loop();

    void register_handler(mq_handler_t &);

    buffer_t * message(char const * name, uint32 id);
    buffer_t * message(char const * name, uint32 id, size_t num_extra, uint32 & user_offset);
    bool       parse_message(buffer_t * msg, message_extra_t &);

    void       send(buffer_t * msg);
    void       send(char const * name);
    buffer_t * pull();

    // -------------- select --------------
    fd_set readable;
    fd_set writable;
    fd_set exceptional;

    int    max_fd;

    // -------------- stats --------------
    mq_loop_stats_t *          loop_start_stats;
    mq_loop_stats_t *          zero_timeout_select_stats;
    mq_loop_stats_t *          select_dispatch_stats;
    mq_loop_stats_t *          message_parsing_stats;
    mq_loop_stats_t *          message_dispatch_stats;
    mq_loop_stats_t *          long_timeout_select_stats;

    mq_loop_stats_t *          current_loop_stats;
    deque<mq_loop_stats_t *>   loop_stats;

    // -------------- Individual functions for select-loop functionality --------------
    void                 _initialize();
    void       _on_select_loop_start();
    int              _on_zero_select();
    void         _on_select_dispatch();
    mq_result   _on_message_dispatch(buffer_t * msg, message_extra_t & extra);
    int              _on_long_select(uint32 msec_timeout);
  };

};

#endif // __MWP_MQ_HPP__

