
#ifndef __MWP_MQ_HPP__
#define __MWP_MQ_HPP__

#include <vector>
#include <set>
#include <deque>
#include <map>
#include <string>
#include "mwp_types.hpp"

namespace net_mobilewebprint {

  using std::vector;
  using std::set;
  using std::deque;
  using std::map;
  using std::pair;

  extern char const * mq_selected_msg;
  extern char const * mq_timeout_msg;

  namespace mq_enum {
    enum e_handle_result
    {
      enot_impl = 0,
      not_impl,
      handled,
      unhandled,
      handled_and_took_message
    };
  };

  // ------------------------------------------------------------------------------------------------
  // Managing generic handlers
  // ------------------------------------------------------------------------------------------------

  struct mq_handler_extra_t
  {
    int       id;
    int       txn_id;
    string    txn_name;

    int       select_loop_num;
    uint32    time;

    mq_handler_extra_t(int id, int txn_id, string const & txn_name, int select_loop_num, uint32 time);
    mq_handler_extra_t(mq_handler_extra_t const & that);
  };

  struct mq_select_loop_start_data_t
  {
    int    current_loop_num;
    uint32 current_loop_start;
  };

  struct mq_select_loop_idle_data_t
  {
    int    current_loop_num;
    uint32 current_loop_start;
  };

  struct mq_handler_t
  {
    virtual string mod_name() = 0;

    // The full signature that actually gets called
    virtual mq_enum::e_handle_result handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra) = 0;

    // Pre process at the top of the select loop
    virtual mq_enum::e_handle_result on_select_loop_start(mq_select_loop_start_data_t const & data);
    virtual mq_enum::e_handle_result  on_select_loop_idle(mq_select_loop_idle_data_t  const & data);
  };

  // ------------------------------------------------------------------------------------------------
  // Managing network modules that care about select()
  // ------------------------------------------------------------------------------------------------

  struct mq_pre_select_data_t
  {
    int      status;
    fd_set * readable;
    fd_set * writable;
    fd_set * exceptional;
    int      max_fd;
    long     timeout;
  };

  struct mq_select_handler_extra_t : public mq_handler_extra_t
  {
    mq_t &    mq;

    set<int>  fds;
    int       num_selected;

    mq_select_handler_extra_t(mq_t & mq, mq_handler_extra_t const & that);

    int       is_udp_readable(network_node_t & node);
    int       is_udp_writable(network_node_t & node);
    int       is_udp_exception(network_node_t & node);
    int       is_tcp_readable(network_node_t & node);
    int       is_tcp_writable(network_node_t & node);
    int       is_tcp_exception(network_node_t & node);

    int       _check(network_node_t & node, int result, char const * which_one);
  };

  struct mq_select_handler_t
  {
    // The full signature that actually gets called
    virtual mq_enum::e_handle_result _mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra) = 0;
    virtual string mod_name() = 0;

    // A select handler must also handle this, it will be notified before the select() call.
    virtual int pre_select(mq_pre_select_data_t * pre_select_data) = 0;
  };

  typedef mq_enum::e_handle_result(*mq_handler_fn_t)(void * pv, std::string const & name, int id, int txn_id, buffer_view_i const & payload, buffer_t * data);
  typedef mq_enum::e_handle_result(*mq_base_handler_fn_t)(void * pv, std::string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);

  struct handler_holder_by_id_t : public mq_handler_t
  {
    int                   id;
    std::string           mod_name_;
    mq_base_handler_fn_t  fn;
    void *                that;

    handler_holder_by_id_t(char const * mod_name, int id, mq_base_handler_fn_t fn, void * that);

    virtual mq_enum::e_handle_result             handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);

    virtual string mod_name();
  };

  // ------------------------------------------------------------------------------------------------
  // Helper for manual timer
  // ------------------------------------------------------------------------------------------------

  struct mq_manual_timer_t
  {
    uint32  time;
    uint32  time_;
    uint32  interval;
    bool    recurring;

    mq_manual_timer_t(uint32 time, uint32 interval, bool recurring = true);
    virtual ~mq_manual_timer_t();

    virtual bool has_elapsed(uint32 current_time);
    virtual void revert();
    virtual void trigger();
    virtual void delay(uint32 addl_amount_to_delay);

    virtual int32 time_remaining(uint32 now = 0);

    virtual void dump();
  };

  struct mq_backoff_timer_t : public mq_manual_timer_t
  {
    uint32 interval_;
    float  factor;
    uint32 max_interval;

    mq_backoff_timer_t(uint32 time, uint32 interval, float factor, uint32 max = 20000, bool recurring = true);

    virtual bool has_elapsed(uint32 current_time);
    virtual void revert();
  };

  struct mq_rolling_backoff_timer_t : public mq_backoff_timer_t
  {
    uint32              orig_interval;
    mq_manual_timer_t   reset_timer;

    mq_rolling_backoff_timer_t(uint32 time, uint32 interval, float factor, uint32 max, uint32 reset_timeout, bool recurring = true);

    virtual bool has_elapsed(uint32 current_time);
  };

  // ------------------------------------------------------------------------------------------------
  // MQ
  // ------------------------------------------------------------------------------------------------

  struct mq_t
  {
    deque<buffer_t const *>   mq_normal;
    deque<buffer_t const *>   mq_high;

    map<string, deque<mq_handler_t *> >  handlerses;
    deque<mq_select_handler_t *>              select_handlers;

    void *                    lock;
    void *                    lock_high;
    void *                    timers_lock;

    int                       current_loop_num;
    uint32                    current_loop_start;
    uint32                    current_loop_second;

    static byte               string_type;        // == 1
    static byte               string32_type;      // == 2
    static byte               string32_ip_type;   // == 3
    static byte               string3232_type;    // == 4

    mq_t();
    ~mq_t();

    // ---------- queueing ----------

    // Event handling
    bool                        on(string name, mq_handler_t *);
    bool               on_selected(mq_select_handler_t *, bool is_fd_set_style = false);    // Is this a handler that wants the fd_set, like CURL?
    bool                        on(mq_handler_t *);

//    // Wrong!  use on_selected() instead
//    private:
//    bool                        on(string name, mq_select_handler_t *);
//    bool                        on(mq_select_handler_t *);

    public:
    void                       off(string name, mq_handler_t const *);
    void                       off(mq_handler_t const *);

    // Sending a message
    bool       send(string const & name);
    bool       send(string const & name, char const * payload);
    bool       send_immediately(string const & name, string const & payload);

    buffer_t * message(char const * name);
    buffer_t * message(char const * name, int id, int txn_id);
    buffer_t * message(char const * name, int id, string const & txn_name);

    buffer_t * message_with_ip(char const * name, int id, int txn_id, uint32 ip = 0, uint16 port = 0);
    void       add_ip_to_message(buffer_t * message, uint32 ip, uint16 port);
    void       add_ip_to_message(buffer_t * message, string ip, uint16 port);

    bool       send(buffer_t * msg);

    // The run loop
    bool run();
    bool is_done();
    void dispatch_loop();

    mq_enum::e_handle_result on_loop_start();
    mq_enum::e_handle_result on_loop_idle();

    mq_enum::e_handle_result dispatch_to_handlers(string queue_name, bool also_dispatch_any, string const & name, int id, int txn_id, string const & txn_name, buffer_range_view_t const & buffer_view, buffer_t * msg, bool dispatch_timers = true);
    mq_enum::e_handle_result dispatch_to_handlers(deque<mq_handler_t *> const & list, string const & name, mq_handler_extra_t & extra, buffer_range_view_t const & buffer_view, buffer_t * msg, bool dispatch_timers = true);
    mq_enum::e_handle_result dispatch_to_select_handlers(int id, int txn_id, buffer_range_view_t const & buffer_view, buffer_t * msg);

    void _show_queue_state(int select_result, char const * msg);

    // ---------- queueing--end ----------

    // ---------- select loop ----------

    // The other half - manage a select loop
    fd_set              readable, writable, exceptional;

    struct timeval      long_timeout;
    struct timeval      zero_timeout;

    mq_t & register_udp_for_select(network_node_t & node);
    mq_t & check_udp_read(network_node_t & node);
    mq_t & check_udp_write(network_node_t & node);

    mq_t & register_tcp_for_select(network_node_t & node, bool persistent_write = false);
    mq_t & check_tcp_read(network_node_t & node);
    mq_t & check_tcp_write(network_node_t & node);

    mq_t & deregister_for_select(network_node_t & node);

    bool is_fd_part_of_select(network_node_t & node);

    int  is_udp_readable(network_node_t & node);
    int  is_udp_writable(network_node_t & node);
    int  is_udp_exception(network_node_t & node);

    int  is_tcp_readable(network_node_t & node);
    int  is_tcp_writable(network_node_t & node);
    int  is_tcp_exception(network_node_t & node);

    mq_t & add_read(int fd);
    mq_t & add_write(int fd, bool persistent = false);
    mq_t & remove_read(int fd);
    mq_t & _remove_write(int fd);

    int  is_fd_readable(int fd);
    int  is_fd_writable(int fd);
    int  is_fd_exception(int fd);

    bool _do_select(int msec_timeout, int & select_result);

    set<int>            read_fds;
    set<int>            write_fds;
    set<int>            persistent_write_fds;

    set<int>            read_fds_for_current_select;
    set<int>            write_fds_for_current_select;

    void _clear_fd_state();

    int _num_curr_reads();
    int _num_curr_writes();
    int _num_curr_exceptions();

    // ---------- select loop--end ----------

    // ---------- timer ----------
    map<int, pair<uint32, mq_handler_t *> >   timers;
    deque<int>                                     timers_order;
    int                                            max_timer_id;

    int  setTimeout(mq_handler_t *, int msec, bool need_lock = false);
    int  setTimeout(handler_holder_by_id_t *& th, string const & mod_name, mq_base_handler_fn_t fn, void * that, int msec, bool need_lock = false);

    buffer_t timeout_msg_;
    string timeout_msg;
    buffer_range_view_t timeout_msg_view;

    int  _dispatch_timers();
    int  _msec_until_next_timer();

    // ---------- timer--end ----------

    /* private-ish */
    buffer_t const * pull();
    static void* _thread_start_fn(void *pvthat);

    // ---------- stats ----------
    uint32    stats_bucket_start_time;
    string    stats_bucket_name;
    bool      _report_stats_flag;
    void      report_and_restart_stats(char const * name);
    void      _report_and_restart_stats();

    map<string, int> event_timings;
    map<string, int> event_counts;
    void      _add_event(string const & name, int time);

    bool      show_queue_state;
    bool      show_timing_stats;
    // ---------- stats-end ----------
  };

  extern bool         parse_msg(net_mobilewebprint::buffer_view_i const & buffer, std::string & name_out, net_mobilewebprint::buffer_range_view_t & buffer_view_out, int & id_out, int & txn_id_out, string & txn_name_out);
  extern byte *       msg_payload_start(net_mobilewebprint::buffer_view_i & buffer);
};

#define MWP_MQ_BASE_HANDLER_HELPER(cls_name, fname) \
static net_mobilewebprint::e_handle_result fname(void * self, std::string const & name, net_mobilewebprint::buffer_view_i const & payload, net_mobilewebprint::buffer_t * data, net_mobilewebprint::mq_handler_extra_t & extra)  \
{ \
  return ((net_mobilewebprint::cls_name *)self)->_##fname(name, payload, data, extra); \
}

#define MWP_MQ_HANDLER_HELPER(cls_name, fname) \
static net_mobilewebprint::e_handle_result fname(void * self, std::string const & name, net_mobilewebprint::buffer_view_i const & payload, net_mobilewebprint::buffer_t * data, net_mobilewebprint::mq_handler_extra_t & extra)  \
{ \
  return ((net_mobilewebprint::cls_name *)self)->_##fname(name, extra.id, extra.txn_id, payload, data); \
}


#endif    // __MWP_MQ_HPP__
