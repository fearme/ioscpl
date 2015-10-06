
#include "mwp_mq.hpp"
#include "mwp_utils.hpp"
#include <ctime>

#ifdef WIN32
static int   wsa_startup_result = -1;
WSADATA      wsa_data;
static bool  wsa_startup = false;
int          start_wsa() {
  if (!wsa_startup) {
    wsa_startup_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
  }
  return wsa_startup_result;
}
#else
int start_wsa() { return 1; }
#endif

using std::make_pair;
using namespace net_mobilewebprint::mq_enum;
using net_mobilewebprint::num_allocations;
using net_mobilewebprint::num_buffer_allocations;

net_mobilewebprint::byte net_mobilewebprint::mq_t::string_type      = 1;
net_mobilewebprint::byte net_mobilewebprint::mq_t::string32_type    = 2;
net_mobilewebprint::byte net_mobilewebprint::mq_t::string32_ip_type = 3;
net_mobilewebprint::byte net_mobilewebprint::mq_t::string3232_type  = 4;

//static char const * __select__ = "mq_selected";

char const * net_mobilewebprint::mq_selected_msg = "mq_selected";
char const * net_mobilewebprint::mq_timeout_msg = "mq_timeout";

net_mobilewebprint::mq_t::mq_t()
  : lock(NULL), lock_high(NULL), timers_lock(NULL), current_loop_num(0), current_loop_start(0), current_loop_second(0), max_timer_id(0),
    stats_bucket_start_time(get_tick_count()), _report_stats_flag(false), show_queue_state(false), show_timing_stats(false)
{
  start_wsa();

  allocate_host_lock("mq_normal", &lock);
  allocate_host_lock("mq_high", &lock_high);
  allocate_host_lock("mq_timers", &timers_lock);

  // 1/2 second
  long_timeout.tv_sec  = 0;
  long_timeout.tv_usec = 500000;

  zero_timeout.tv_sec  = 0;
  zero_timeout.tv_usec = 0;

  // Setup the one and only message object used for notifying of setTimeout firing
  int      id      = 0;
  int      txn_id  = 0;
  string   txn_name;

  timeout_msg_.appendT(&string_type, mq_timeout_msg, mq_timeout_msg);
  parse_msg(timeout_msg_, timeout_msg, timeout_msg_view, id, txn_id, txn_name);

}

net_mobilewebprint::mq_t::~mq_t()
{
  free_host_lock("mq_normal", lock);          lock = NULL;
  free_host_lock("mq_high", lock_high);     lock_high = NULL;
  free_host_lock("mq_timers", timers_lock);   timers_lock = NULL;
}

bool net_mobilewebprint::mq_t::run()
{
  bool result = start_thread(_thread_start_fn, this);


  // The platform can either 1) provide a start_thread function, so mq can re-start on that new thread,
  // or 2) have invoked this run() function in a dedicated thread.  In the case of (2), the start_thread
  // function will return false
  if (result) {
    log_d("Host provided start_thread function, using a new thread");
    return result;
  }

  /* otherwise */
  log_d("Host did not provided start_thread function, staying in this thread");
  _thread_start_fn(this);
  return true;
}

bool net_mobilewebprint::mq_t::is_done()
{
  return false;
}

bool net_mobilewebprint::mq_t::on(mq_handler_t * phandler)
{
  return on("", phandler);
}

bool net_mobilewebprint::mq_t::on(string name_, mq_handler_t * phandler)
{
  string name(name_);
  if (name.length() == 0) {
    name = "__any__";
  }

  handlerses[name].push_back(phandler);
  return true;
}

bool net_mobilewebprint::mq_t::on_selected(mq_select_handler_t * handler, bool is_fd_set_style)
{
  if (is_fd_set_style) {
    select_handlers.push_front(handler);
  } else {
    select_handlers.push_back(handler);
  }

  return true;
}

net_mobilewebprint::mq_manual_timer_t::mq_manual_timer_t(uint32 time__, uint32 interval_, bool recurring_)
  : time(time__), time_(0), interval(interval_), recurring(recurring_)
{
}

bool net_mobilewebprint::mq_manual_timer_t::has_elapsed(uint32 current_time)
{
  time_ = time;
  if (recurring) { return _has_elapsed(time, interval, current_time); }

  /* otherwise */
  bool result = _has_elapsed(time, interval, current_time);

  if (result) {
    time = (uint32)(0x7fffffff);
  }

  return result;
}

void net_mobilewebprint::mq_manual_timer_t::revert()
{
  time = time_;
}

void net_mobilewebprint::mq_manual_timer_t::trigger()
{
  time = get_tick_count() - interval;
}

void net_mobilewebprint::mq_manual_timer_t::delay(uint32 addl_amount_to_delay)
{
  uint32 new_time = get_tick_count() + addl_amount_to_delay;
  uint32 new_interval = new_time - time;

  if (new_interval > interval) {
    interval = new_interval;
  }
}

int32 net_mobilewebprint::mq_manual_timer_t::time_remaining(uint32 now)
{
  if (now == 0) {
    now = get_tick_count();
  }

  return (time + interval) - now;
}

void net_mobilewebprint::mq_manual_timer_t::dump()
{
  log_d(1, "", "Manual timer: time: %d, interval: %d", time, interval);
}




net_mobilewebprint::mq_backoff_timer_t::mq_backoff_timer_t(uint32 time, uint32 interval, float factor_, uint32 max, bool recurring)
  : mq_manual_timer_t(time, interval, recurring), factor(factor_), max_interval(max)
{
}

bool net_mobilewebprint::mq_backoff_timer_t::has_elapsed(uint32 current_time)
{
  interval_ = interval;
  bool result = mq_manual_timer_t::has_elapsed(current_time);

  if (result) {
    if ((interval *= factor) > max_interval) {
      interval = max_interval;
    }
  }

  return result;
}

void net_mobilewebprint::mq_backoff_timer_t::revert()
{
  interval = interval_;
  mq_manual_timer_t::revert();
}




net_mobilewebprint::mq_rolling_backoff_timer_t::mq_rolling_backoff_timer_t(uint32 time, uint32 interval, float factor, uint32 reset_timeout, uint32 max, bool recurring)
  : mq_backoff_timer_t(time, interval, factor, max, recurring), orig_interval(interval), reset_timer(time, reset_timeout, false)
{
}

bool net_mobilewebprint::mq_rolling_backoff_timer_t::has_elapsed(uint32 current_time)
{
  bool result = mq_backoff_timer_t::has_elapsed(current_time);

  if (reset_timer.has_elapsed(current_time)) {
    interval = interval_ = orig_interval;
  }

  return result;
}




void net_mobilewebprint::mq_t::off(string name_, mq_handler_t const * handler)
{
  string name(name_);
  if (name.length() == 0) {
    name = "__any__";
  }

  map<string, deque<mq_handler_t *> >::iterator item;
  if ((item = handlerses.find(name)) != handlerses.end()) {
    deque<mq_handler_t *> & handlers_for_name = item->second;
    for (deque<mq_handler_t *>::iterator it = handlers_for_name.begin(); it != handlers_for_name.end(); ++it) {
      if (*it == handler) {
        handlers_for_name.erase(it);
      }
    }
  }
}

void net_mobilewebprint::mq_t::off(mq_handler_t const * handler)
{
  off("", handler);
}

bool net_mobilewebprint::mq_t::send(string const & name)
{
  if (host_lock("mq_normal", lock)) {
    buffer_t * buffer = new buffer_t; /**/ num_buffer_allocations += 1;
    buffer->appendT(&string_type, name);

    mq_normal.push_back(buffer);

    host_unlock("mq_normal", lock);
  }

  return true;
}

bool net_mobilewebprint::mq_t::send(string const & name, char const * payload)
{
  if (host_lock("mq_normal", lock)) {
    buffer_t * buffer = new buffer_t; /**/ num_buffer_allocations += 1;
    buffer->appendT(&string_type, name, payload);

    mq_normal.push_back(buffer);

    host_unlock("mq_normal", lock);
  }

  return true;
}

bool net_mobilewebprint::mq_t::send_immediately(string const & name, string const & payload)
{
  if (host_lock("mq_normal", lock)) {
    buffer_t * buffer = new buffer_t; /**/ num_buffer_allocations += 1;
    buffer->appendT(&string_type, name, payload);

    mq_normal.push_front(buffer);

    host_unlock("mq_normal", lock);
  }

  return true;
}

bool net_mobilewebprint::mq_t::send(buffer_t * buffer)
{
  if (host_lock("mq_normal", lock)) {
    mq_normal.push_back(buffer);
    host_unlock("mq_normal", lock);
  }

  return true;
}

net_mobilewebprint::buffer_t * net_mobilewebprint::mq_t::message(char const * name)
{
  buffer_t * buffer = new buffer_t; /**/ num_buffer_allocations += 1;
  buffer->appendT(&string_type, name);
  return buffer;
}

net_mobilewebprint::buffer_t * net_mobilewebprint::mq_t::message(char const * name_, int id, int txn_id)
{
  char name[MWP_STRLEN_BUFFER_STRING];
  ::memset(name, 0, sizeof(name));
  ::memcpy(name, name_, min(sizeof(name), strlen(name_)));

  buffer_t * buffer = new buffer_t; /**/ num_buffer_allocations += 1;
  buffer->appendT(&string32_type, name);
  buffer->append((uint32)id);
  buffer->append((uint32)txn_id);
  return buffer;
}

net_mobilewebprint::buffer_t * net_mobilewebprint::mq_t::message_with_ip(char const * name_, int id, int txn_id, uint32 ip, uint16 port)
{
  char name[MWP_STRLEN_BUFFER_STRING];
  ::memset(name, 0, sizeof(name));
  ::memcpy(name, name_, min(sizeof(name), strlen(name_)));

  buffer_t * buffer = new buffer_t; /**/ num_buffer_allocations += 1;
  buffer->appendT(&string32_ip_type, name);
  buffer->append((uint32)id);
  buffer->append((uint32)txn_id);

  buffer->append((uint32)0);
  buffer->append((uint16)0);

  return buffer;
}

net_mobilewebprint::buffer_t * net_mobilewebprint::mq_t::message(char const * name_, int id, string const & txn_name)
{
  char name[MWP_STRLEN_BUFFER_STRING];

  ::memset(name, 0, sizeof(name));
  ::memcpy(name, name_, min(sizeof(name), strlen(name_)));

  buffer_t * buffer = new buffer_t; /**/ num_buffer_allocations += 1;
  buffer->appendT(&string3232_type, name);

  buffer->append((uint32)id);

  ::memset(name, 0, sizeof(name));
  ::memcpy(name, txn_name.c_str(), min(sizeof(name), txn_name.length()));
  buffer->appendT(name);

  return buffer;
}

// Later, you can stuff the IP, port into the message
void net_mobilewebprint::mq_t::add_ip_to_message(buffer_t * message, string ip_, uint16 port)
{
  uint32 ip = 0;
  inet_pton(AF_INET, ip_.c_str(), &ip);
  ip = ntohl(ip);                           // Must convert back to host byte-order

  add_ip_to_message(message, ip, port);
}

void net_mobilewebprint::mq_t::add_ip_to_message(buffer_t * message, uint32 ip, uint16 port)
{
  static int ip_offset = MWP_STRLEN_BUFFER_STRING + 1 + 4 + 4;
  message->set(ip_offset, ip);
  message->set(ip_offset + sizeof(ip), port);
}

/**
 *  The default pre_select handler does nothing.
 */
//int net_mobilewebprint::mq_handler_t::pre_select(mq_pre_select_data_t * pre_select_data)
//{
//  return 0;
//}

net_mobilewebprint::mq_handler_extra_t::mq_handler_extra_t(int id_, int txn_id_, string const & txn_name_, int select_loop_num_, uint32 time_)
  : id(id_), txn_id(txn_id_), txn_name(txn_name_), select_loop_num(select_loop_num_), time(time_)
{
}

net_mobilewebprint::mq_handler_extra_t::mq_handler_extra_t(mq_handler_extra_t const & that)
  : id(that.id), txn_id(that.txn_id), txn_name(that.txn_name), select_loop_num(that.select_loop_num), time(that.time)
{
}

net_mobilewebprint::mq_select_handler_extra_t::mq_select_handler_extra_t(mq_t & mq_, mq_handler_extra_t const & that)
  : mq_handler_extra_t(that), mq(mq_), num_selected(0)
{
}

int net_mobilewebprint::mq_select_handler_extra_t::_check(network_node_t & node, int result, char const *which_one)
{
  if (result) {
    num_selected += 1;
  }

  if (!mq.is_fd_part_of_select(node)) {
    net_mobilewebprint::log_w(":MQ:", "Fd %d is not part of %s select, but is being asked for res %d port: %d", node.the_fd(), which_one, result, (int)node.port);
//    mwp_assert(false);
  }

  return 1;
}

int net_mobilewebprint::mq_select_handler_extra_t::is_udp_readable(network_node_t & node)
{
  int result = mq.is_udp_readable(node);
  _check(node, result, "udp-read");
  return result;
}

int net_mobilewebprint::mq_select_handler_extra_t::is_udp_writable(network_node_t & node)
{
  int result = mq.is_udp_writable(node);
  _check(node, result, "udp-write");
  return result;
}

int net_mobilewebprint::mq_select_handler_extra_t::is_udp_exception(network_node_t & node)
{
  int result = mq.is_udp_exception(node);
  _check(node, result, "udp-except");
  return result;
}

int net_mobilewebprint::mq_select_handler_extra_t::is_tcp_readable(network_node_t & node)
{
  int result = mq.is_tcp_readable(node);
  _check(node, result, "tcp-read");
  return result;
}

int net_mobilewebprint::mq_select_handler_extra_t::is_tcp_writable(network_node_t & node)
{
  int result = mq.is_tcp_writable(node);
  _check(node, result, "tcp-write");
  return result;
}

int net_mobilewebprint::mq_select_handler_extra_t::is_tcp_exception(network_node_t & node)
{
  int result = mq.is_tcp_exception(node);
  _check(node, result, "tcp-except");
  return result;
}

void net_mobilewebprint::mq_t::_show_queue_state(int select_result, char const * msg)
{
//  show_queue_state = true;
  // Set a breakpoint and change "show_queue_state" to true to see the state of the queue
  if (show_queue_state) {
    if (host_lock("mq_normal", lock)) {
      // Log the state of the queue
      log_d(1, "",
      //printf(
             "MQ_select_loop(%s-%03d): %3d network events(%2d/%d, %2d/%d, %2d/%d); %3d timers pending; items on the queue %3d;",
             msg,
             current_loop_num,
             select_result,
             _num_curr_reads(), (int)read_fds_for_current_select.size(),
             _num_curr_writes(), (int)write_fds_for_current_select.size(),
             _num_curr_exceptions(), (int)_union(read_fds_for_current_select, write_fds_for_current_select).size(),
             (int)timers.size(),
             (int)mq_normal.size());

#if 0
      // Add the fds
      set<int> all_fds = _union(read_fds, write_fds);
      for (set<int>::const_iterator it_fd = all_fds.begin(); it_fd != all_fds.end(); ++it_fd) {
        int fd = *it_fd;
        log_d(1, "",
        //printf(
               "0x%04x(%d,%d,%d) ", fd, !!FD_ISSET(fd, &readable), !!FD_ISSET(fd, &writable), !!FD_ISSET(fd, &exceptional));
      }
#endif

#if 1
      // Add the first few items in the queue
      string msg;
      std::map<string, int> hist;
      int msg_count = 0;
      for (deque<buffer_t const *>::const_iterator it_msg = mq_normal.begin(); it_msg != mq_normal.end() && msg_count < 600; ++it_msg, ++msg_count) {
        buffer_t const * pb = *it_msg;
        string item((char const *)(pb->const_begin() + 1));
        msg += item;

        if (hist.find(item) == hist.end()) {
          hist.insert(make_pair(item, 1));
        } else {
          hist[item] += 1;
        }
      }

      log_d(1, "", "%30s: %5d", "count", msg_count);
      for (std::map<string, int>::const_iterator it = hist.begin(); it != hist.end(); it++) {
        log_d(1, "", "%30s: %5d", (*it).first.c_str(), it->second);
      }
#endif

      host_unlock("mq_normal", lock);
    }
  }
}

// TODO: make these members
int    num_loop_starts                          = 0;
uint32 loop_start_total_running_time            = 0;

int    num_zero_timeout_selects                 = 0;
uint32 zero_timeout_select_total_running_time   = 0;
int    num_zero_timeout_dispatches              = 0;
uint32 zero_timeout_dispatch_total_running_time = 0;

int    num_message_parsings                     = 0;
uint32 message_parsings_total_running_time      = 0;
int    num_message_dispatches                   = 0;
uint32 message_dispatches_total_running_time    = 0;

int    num_timers_1                             = 0;
uint32 timer_1_total_running_time               = 0;

int    num_long_timeout_selects                 = 0;
uint32 long_timeout_select_total_running_time   = 0;
int    num_long_timeout_dispatches              = 0;
uint32 long_timeout_dispatch_total_running_time = 0;

int    num_timers_2                             = 0;
uint32 timer_2_total_running_time               = 0;

int    num_loop_idles                           = 0;
uint32 loop_idle_total_running_time             = 0;

uint32 do_select_0_a_running_time               = 0;
uint32 do_select_0_b_running_time               = 0;
uint32 do_select_0_total_running_time           = 0;
uint32 do_select_a_running_time                 = 0;
uint32 do_select_b_running_time                 = 0;
uint32 do_select_total_running_time             = 0;

static uint32 LOOP_START            = 0x00000100;
static uint32 ZERO_TIMEOUT_SELECT   = 0x00000001;
static uint32 LONG_TIMEOUT_SELECT   = 0x00000002;
static uint32 ZERO_TIMEOUT_DISPATCH = 0x00000004;
static uint32 LONG_TIMEOUT_DISPATCH = 0x00000008;
static uint32 MESSAGE_PARSING       = 0x00000010;
static uint32 MESSAGE_DISPATCH      = 0x00000020;
static uint32 TIMER_1               = 0x00000040;
static uint32 TIMER_2               = 0x00000080;
static uint32 LOOP_IDLE             = 0x00000100;

void net_mobilewebprint::mq_t::report_and_restart_stats(char const * name)
{
  stats_bucket_name   = name;
  _report_stats_flag  = true;
}

#define SHOW log_d
//#define SHOW printf

void net_mobilewebprint::mq_t::_report_and_restart_stats()
{
  uint32    now                      = get_tick_count();
  uint32    stats_bucket_total_time_ = now - stats_bucket_start_time;

  // Set a breakpoint and change "show_timing_stats" to true to see the state of the queue
  //show_timing_stats = true;
  if (show_timing_stats) {
    float     stats_bucket_total_time  = (float)stats_bucket_total_time_;

    SHOW("Stats report: %d msecs\n", stats_bucket_total_time_);
    SHOW("    %40s: %6.4f%% %5d, count: %5d\n", "loop-start",                   no_div_zero(loop_start_total_running_time, stats_bucket_total_time), loop_start_total_running_time, num_loop_starts);
    SHOW("    %40s: %6.4f%% %5d, count: %5d\n", "zero-timeout-select",          no_div_zero(zero_timeout_select_total_running_time, stats_bucket_total_time), zero_timeout_select_total_running_time, num_zero_timeout_selects);
    SHOW("    %40s: %6.4f%% %5d, count: %5d\n", "zero-timeout-select dispatch", no_div_zero(zero_timeout_dispatch_total_running_time, stats_bucket_total_time), zero_timeout_dispatch_total_running_time, num_zero_timeout_dispatches);
    SHOW("    %40s: %6.4f%% %5d, count: %5d\n", "message parsing",              no_div_zero(message_parsings_total_running_time, stats_bucket_total_time), message_parsings_total_running_time, num_message_parsings);
    SHOW("    %40s: %6.4f%% %5d, count: %5d\n", "message dispatches",           no_div_zero(message_dispatches_total_running_time, stats_bucket_total_time), message_dispatches_total_running_time, num_message_dispatches);

    SHOW("    %40s: %6.4f%% %5d, count: %5d\n", "timer 1",                      no_div_zero(timer_1_total_running_time, stats_bucket_total_time), timer_1_total_running_time, num_timers_1);

    SHOW("    %40s: %6.4f%% %5d, count: %5d\n", "long-timeout-select",          no_div_zero(long_timeout_select_total_running_time, stats_bucket_total_time), long_timeout_select_total_running_time, num_long_timeout_selects);
    SHOW("    %40s: %6.4f%% %5d, count: %5d\n", "long-timeout-select dispatch", no_div_zero(long_timeout_dispatch_total_running_time, stats_bucket_total_time), long_timeout_dispatch_total_running_time, num_long_timeout_dispatches);

    SHOW("    %40s: %6.4f%% %5d, count: %5d\n", "timer 2",                      no_div_zero(timer_2_total_running_time, stats_bucket_total_time), timer_2_total_running_time, num_timers_2);
    SHOW("    %40s: %6.4f%% %5d, count: %5d\n", "loop-idle",                    no_div_zero(loop_idle_total_running_time, stats_bucket_total_time), loop_idle_total_running_time, num_loop_idles);

    SHOW("\n");
    for (map<string, int>::const_iterator it = event_counts.begin(); it != event_counts.end(); ++it) {
      string const & name = it->first;
      int            count = it->second;
      int            time = event_timings[name];

      SHOW("    %40s: %6.4f%% %5d, count: %5d\n", name.c_str(), no_div_zero(time, stats_bucket_total_time), time, count);
    }

    SHOW("\n");
    SHOW("    %40s: %6.4f%% %5d\n", "do_select 0 A", no_div_zero(do_select_0_a_running_time, do_select_0_total_running_time), do_select_0_a_running_time);
    SHOW("    %40s: %6.4f%% %5d\n", "do_select 0 B", no_div_zero(do_select_0_b_running_time, do_select_0_total_running_time), do_select_0_b_running_time);
    SHOW("    %40s: %6.4f%% %5d\n", "do_select A", no_div_zero(do_select_a_running_time, do_select_total_running_time), do_select_a_running_time);
    SHOW("    %40s: %6.4f%% %5d\n", "do_select B", no_div_zero(do_select_b_running_time, do_select_total_running_time), do_select_b_running_time);
  }


  stats_bucket_start_time  = now;

  num_loop_starts                          = 0;
  loop_start_total_running_time            = 0;
  num_zero_timeout_selects                 = 0;
  zero_timeout_select_total_running_time   = 0;
  num_long_timeout_selects                 = 0;
  long_timeout_select_total_running_time   = 0;
  num_zero_timeout_dispatches              = 0;
  zero_timeout_dispatch_total_running_time = 0;
  num_long_timeout_dispatches              = 0;
  long_timeout_dispatch_total_running_time = 0;
  num_message_parsings                     = 0;
  message_parsings_total_running_time      = 0;
  num_message_dispatches                   = 0;
  message_dispatches_total_running_time    = 0;
  num_timers_1                             = 0;
  timer_1_total_running_time               = 0;
  num_timers_2                             = 0;
  timer_2_total_running_time               = 0;
  num_loop_idles                           = 0;
  loop_idle_total_running_time             = 0;

  do_select_0_a_running_time               = 0;
  do_select_0_b_running_time               = 0;
  do_select_0_total_running_time           = 0;
  do_select_a_running_time                 = 0;
  do_select_b_running_time                 = 0;
  do_select_total_running_time             = 0;

  event_timings = map<string, int>();
  event_counts = map<string, int>();

}

void net_mobilewebprint::mq_t::_add_event(string const & name, int time)
{
  if (event_timings.find(name) == event_timings.end()) {
    event_timings[name] = 0;
    event_counts[name] = 0;
  }

  event_timings[name] += time;
  event_counts[name] += 1;
}


std::string selected_msg;
void net_mobilewebprint::mq_t::dispatch_loop()
{
  log_d("Starting dispatch loop");

  bool      dispatch_select = false;
  int       id = 0, txn_id = 0, zero_timeout_select_result = 0, long_timeout_select_result = 0;
  string    txn_name;

  uint32    loop_events                    = 0;
  uint32    event_start                    = 0;
  uint32    event_end                      = 0;
  uint32    current_loop_end               = 0;
  uint32    loop_items_run_time            = 0;
  uint32    remainder_run_time             = 0;

  uint32    loop_start_run_time            = 0;
  uint32    zero_timeout_select_run_time   = 0;
  uint32    long_timeout_select_run_time   = 0;
  uint32    zero_timeout_dispatch_run_time = 0;
  uint32    long_timeout_dispatch_run_time = 0;
  uint32    message_parsing_run_time       = 0;
  uint32    message_dispatch_run_time      = 0;
  uint32    timer_1_run_time               = 0;
  uint32    timer_2_run_time               = 0;
  uint32    loop_idle_run_time             = 0;

  // Setup the one and only message object used for notifying of select results
  buffer_t selected_msg_;
  selected_msg_.appendT(&string_type, mq_selected_msg, mq_selected_msg);

  buffer_range_view_t selected_msg_view;
  parse_msg(selected_msg_, selected_msg, selected_msg_view, id, txn_id, txn_name);

  string              name;
  buffer_range_view_t buffer_view;
  e_handle_result     handled_result     = (e_handle_result)0;
  bool                parsed             = false;
  int                 num_timers_expired = 0;
  int                 msec               = 0;
  bool                socket_was_active  = false;


  // The select loop!
  for (current_loop_num = 0;; ++current_loop_num) {
    current_loop_end = get_tick_count();

    // Collect stats
    //   Since this loop uses 'continue' statements, you must put your end-of-loop
    //   stats gathering here, and don't do it for the first iteration
    if (current_loop_num != 0) {
      remainder_run_time = loop_items_run_time = current_loop_end - current_loop_start;

      if (loop_events & LOOP_START) {
        num_loop_starts += 1;
        loop_start_total_running_time += loop_start_run_time;
        remainder_run_time -= loop_start_run_time;
      }

      if (loop_events & ZERO_TIMEOUT_SELECT) {
        num_zero_timeout_selects += 1;
        zero_timeout_select_total_running_time += zero_timeout_select_run_time;
        remainder_run_time -= zero_timeout_select_run_time;
      }

      if (loop_events & LONG_TIMEOUT_SELECT) {
        num_long_timeout_selects += 1;
        long_timeout_select_total_running_time += long_timeout_select_run_time;
        remainder_run_time -= long_timeout_select_run_time;
      }

      if (loop_events & ZERO_TIMEOUT_DISPATCH) {
        num_zero_timeout_dispatches += 1;
        zero_timeout_dispatch_total_running_time += zero_timeout_dispatch_run_time;
        remainder_run_time -= zero_timeout_dispatch_run_time;
      }

      if (loop_events & LONG_TIMEOUT_DISPATCH) {
        num_long_timeout_dispatches += 1;
        long_timeout_dispatch_total_running_time += long_timeout_dispatch_run_time;
        remainder_run_time -= long_timeout_dispatch_run_time;
      }

      if (loop_events & MESSAGE_PARSING) {
        num_message_parsings += 1;
        message_parsings_total_running_time += message_parsing_run_time;
        remainder_run_time -= message_parsing_run_time;
      }

      if (loop_events & MESSAGE_DISPATCH) {
        num_message_dispatches += 1;
        message_dispatches_total_running_time += message_dispatch_run_time;
        remainder_run_time -= message_dispatch_run_time;
      }

      if (loop_events & TIMER_1) {
        num_timers_1 += 1;
        timer_1_total_running_time += timer_1_run_time;
        remainder_run_time -= timer_1_run_time;
      }

      if (loop_events & TIMER_2) {
        num_timers_2 += 1;
        timer_2_total_running_time += timer_2_run_time;
        remainder_run_time -= timer_2_run_time;
      }

      if (loop_events & LOOP_IDLE) {
        num_loop_idles += 1;
        loop_idle_total_running_time += loop_idle_run_time;
        remainder_run_time -= loop_idle_run_time;
      }

      if (_report_stats_flag) {
        _report_and_restart_stats();
        _report_stats_flag = false;
      }
    }

    current_loop_start  = get_tick_count();
    event_start         = current_loop_start;
    current_loop_second = current_loop_start / 1000;
    loop_events         = 0;
    socket_was_active   = false;

    // ----- Let all handlers know we are starting at the top of the select loop
    loop_events                   |= LOOP_START;
    handled_result                 = on_loop_start();
    loop_start_run_time            = (event_end = get_tick_count()) - event_start;
    event_start                    = event_end;

    // ----- Do the select() that has zero timeout; remember if we should dispatch it
    loop_events                   |= ZERO_TIMEOUT_SELECT;
    dispatch_select                = _do_select(-1, zero_timeout_select_result) && zero_timeout_select_result > 0;
    zero_timeout_select_run_time   = (event_end = get_tick_count()) - event_start;
    event_start                    = event_end;

    // Show the queue
    _show_queue_state(zero_timeout_select_result, "zero-timeout");

    // ----- Dispatch the select, if any FDs indicated so
    if (dispatch_select) {

      // We waited zero time, and there were events.  Dispatch them, then immediately do
      // this select again (continue the loop).
      loop_events                     |= ZERO_TIMEOUT_DISPATCH;
      handled_result                   = dispatch_to_select_handlers(0, 0, selected_msg_view, NULL);
      zero_timeout_dispatch_run_time   = (event_end = get_tick_count()) - event_start;
      event_start                      = event_end;

      // Once dispatched, must not allow reading at any 'ol time
      _clear_fd_state();

      // Network events get priority.  Go around again, but don't starve the other messages
      socket_was_active = true;
      if ((current_loop_num & 0x1) == 0) {
        continue;
      }
    }

    buffer_t const * msg = pull();
    if (msg) {

      // Dispatch to the right handler(s)
      loop_events                     |= MESSAGE_PARSING;
      parsed                           = parse_msg(*msg, name, buffer_view, id, txn_id, txn_name);
      message_parsing_run_time         = (event_end = get_tick_count()) - event_start;
      event_start                      = event_end;

      if (parsed) {

        loop_events                     |= MESSAGE_DISPATCH;
        handled_result                   = dispatch_to_handlers(name, true, name, id, txn_id, txn_name, buffer_view, (buffer_t *)msg);
        message_dispatch_run_time        = (event_end = get_tick_count()) - event_start;
        event_start                      = event_end;

        if (handled_result != handled_and_took_message) {
          delete msg; /**/ num_buffer_allocations -= 1;
        }
      }

      // We processed a message.  Go back to the top of the loop and see if any network events are pending
      if ((current_loop_num & 0x3) == 0) {
        continue;
      }
    }

    loop_events                   |= TIMER_1;
    num_timers_expired             = _dispatch_timers();
    timer_1_run_time               = (event_end = get_tick_count()) - event_start;
    event_start                    = event_end;

    // If there is still work, go back and do it, but don't starve idle processing
    if (mq_normal.size() > 0 && current_loop_num & 0x7) {
      continue;
    }

    // Do any 'idle' processing
    loop_events                   |= LOOP_IDLE;
    handled_result                 = on_loop_idle();
    loop_idle_run_time             = (event_end = get_tick_count()) - event_start;
    event_start                    = event_end;

    // ------------------------------------------------------------------------
    // Under no circumstances should we do a sleep if there is still work to do
    // ------------------------------------------------------------------------

    if (mq_normal.size() > 0 || socket_was_active) {
      continue;
    }

    // No messages in the queue, no network events.  Use this select as the mechanism whereby this loop 'sleeps'

    msec = _msec_until_next_timer();
    //log_d(1, "", "time until next timer: %d\n", msec);
    if (msec < 0 || msec > 500) {
      msec = 250;
    }

    loop_events                   |= LONG_TIMEOUT_SELECT;
    dispatch_select                = _do_select(msec, long_timeout_select_result) && long_timeout_select_result != 0;
    long_timeout_select_run_time   = (event_end = get_tick_count()) - event_start;
    event_start                    = event_end;

    _show_queue_state(long_timeout_select_result, "long timeout");

    if (dispatch_select) {

      loop_events                     |= LONG_TIMEOUT_DISPATCH;
      handled_result                   = dispatch_to_select_handlers(0, 0, selected_msg_view, NULL);
      long_timeout_dispatch_run_time   = (event_end = get_tick_count()) - event_start;
      event_start                      = event_end;

      // Once dispatched, must not allow reading at any 'ol time
      _clear_fd_state();
    }

    log_v("------------------------------ long select %d %d --------------", msec, long_timeout_select_result);

    loop_events                   |= TIMER_2;
    num_timers_expired            += _dispatch_timers();
    timer_2_run_time               = (event_end = get_tick_count()) - event_start;
    event_start                    = event_end;
  }
}

e_handle_result net_mobilewebprint::mq_t::dispatch_to_handlers(
  string queue_name, bool also_dispatch_any,
  string const & name, int id, int txn_id, string const & txn_name,
  buffer_range_view_t const & buffer_view, buffer_t * msg, bool dispatch_timers)
{
  e_handle_result      result = enot_impl;
  mq_handler_extra_t   extra(id, txn_id, txn_name, current_loop_num, current_loop_start);

  map<string, deque<mq_handler_t *> >::const_iterator pphandler;

  // Send to handlers that specifically asked for this message first
  if ((pphandler = handlerses.find(queue_name)) != handlerses.end()) {
    result = dispatch_to_handlers(pphandler->second, name, extra, buffer_view, msg, dispatch_timers);
  }

  if (result == handled_and_took_message) { return result; }

  /* otherwise */
  if (also_dispatch_any && (pphandler = handlerses.find("__any__")) != handlerses.end()) {
    result = dispatch_to_handlers(pphandler->second, name, extra, buffer_view, msg, dispatch_timers);
  }

  return result;
}

e_handle_result net_mobilewebprint::mq_t::dispatch_to_handlers(deque<mq_handler_t *> const & handler_queue, string const & name, mq_handler_extra_t & extra, buffer_range_view_t const & buffer_view, buffer_t * msg, bool dispatch_timers)
{
  e_handle_result result = enot_impl, handled_result = enot_impl;

  for (deque<mq_handler_t *>::const_iterator phandler = handler_queue.begin(); phandler != handler_queue.end(); ++phandler) {
    mq_handler_t * handler = *phandler;

    uint32 start = get_tick_count();
    if ((handled_result = handler->handle(name, buffer_view, msg, extra)) != enot_impl && handled_result != not_impl) {
      result = handled_result;
    }

    uint32 now = get_tick_count();
    _add_event(name, now - start);

    if (now - start > 750) {
      log_w("mq_dispatch", "Handler %s for %s took too long: %f seconds.", handler->mod_name().c_str(), name.c_str(), ((float)now - start) / 1000.0);
    }

    if (handled_result == handled_and_took_message) {
      break;
    }
  }

  if (dispatch_timers) {
    _dispatch_timers();
  }

  return handled_result;
}

bool net_mobilewebprint::parse_msg(net_mobilewebprint::buffer_view_i const & buffer, std::string & name_out, net_mobilewebprint::buffer_range_view_t & buffer_view_out, int & id_out, int & txn_id_out, string & txn_name_out)
{
  net_mobilewebprint::byte *    p = buffer.const_begin();
  net_mobilewebprint::byte   type = *p++;

  id_out        = -1;
  txn_id_out    = 0;
  txn_name_out  = "";

  if (type == net_mobilewebprint::mq_t::string_type) {

    name_out = (char*)p;
    p += ::strlen((char*)p) + 1;

    buffer_view_out = net_mobilewebprint::buffer_range_view_t(p, buffer.const_end());

    return true;
  }

  /* otherwise */
  if (type == net_mobilewebprint::mq_t::string32_type) {

    char name[MWP_STRLEN_BUFFER_STRING + 1];
    ::memset(name, 0, sizeof(name));
    ::memcpy(name, p, MWP_STRLEN_BUFFER_STRING);

    name_out = name;
    p += MWP_STRLEN_BUFFER_STRING;

    id_out = (int)(ntohl(*(uint32*)p));
    p += 4;

    txn_id_out = (int)(ntohl(*(uint32*)p));
    p += 4;

    buffer_view_out = net_mobilewebprint::buffer_range_view_t(p, buffer.const_end());

    return true;
  }

  /* otherwise */
  if (type == net_mobilewebprint::mq_t::string32_ip_type) {

    char name[MWP_STRLEN_BUFFER_STRING + 1];
    ::memset(name, 0, sizeof(name));
    ::memcpy(name, p, MWP_STRLEN_BUFFER_STRING);

    name_out = name;
    p += MWP_STRLEN_BUFFER_STRING;

    id_out = (int)(ntohl(*(uint32*)p));
    p += 4;

    txn_id_out = (int)(ntohl(*(uint32*)p));
    p += 4;

    p += 4;       // IP address isn't sent out, but the view should not contain it
    p += 2;       // Port isn't sent out, but the view should not contain it

    buffer_view_out = net_mobilewebprint::buffer_range_view_t(p, buffer.const_end());

    return true;
  }

  /* otherwise */
  if (type == net_mobilewebprint::mq_t::string3232_type) {

    char name[MWP_STRLEN_BUFFER_STRING + 1];
    char name2[MWP_STRLEN_BUFFER_STRING + 1];
    ::memset(name, 0, sizeof(name));
    ::memset(name2, 0, sizeof(name2));

    ::memcpy(name, p, MWP_STRLEN_BUFFER_STRING);

    name_out = name;
    p += MWP_STRLEN_BUFFER_STRING;

    id_out = (int)(ntohl(*(uint32*)p));
    p += 4;

    ::memcpy(name2, p, MWP_STRLEN_BUFFER_STRING);
    txn_name_out = name2;
    p += MWP_STRLEN_BUFFER_STRING;

    buffer_view_out = net_mobilewebprint::buffer_range_view_t(p, buffer.const_end());

    return true;
  }

  buffer_view_out = net_mobilewebprint::buffer_range_view_t();
  return false;
}

net_mobilewebprint::byte * net_mobilewebprint::msg_payload_start(net_mobilewebprint::buffer_view_i & buffer)
{
  net_mobilewebprint::byte *    p = buffer.begin();
  net_mobilewebprint::byte   type = *p++;

  if (type == net_mobilewebprint::mq_t::string_type) {
    return p + ::strlen((char*)p) + 1;
  }

  /* otherwise */
  if (type == net_mobilewebprint::mq_t::string32_type) {
    return p + MWP_STRLEN_BUFFER_STRING + 4 + 4;
  }

  /* otherwise */
  if (type == net_mobilewebprint::mq_t::string32_ip_type) {
    return p + MWP_STRLEN_BUFFER_STRING + 4 + 4 + 4 + 2;
  }

  return p;
}

int net_mobilewebprint::mq_t::setTimeout(handler_holder_by_id_t *& pth, string const & mod_name, mq_base_handler_fn_t fn, void * that, int msec, bool need_lock)
{
  if (pth == NULL) {
    pth = new handler_holder_by_id_t(mod_name.c_str(), 0, fn, that); /**/ num_allocations += 1;
  }
  pth->id = setTimeout(pth, msec, need_lock);
  return pth->id;
}

int net_mobilewebprint::mq_t::setTimeout(mq_handler_t * phandler, int msec, bool need_lock)
{
  int id = -1;
  bool got_lock = false;

  if (need_lock) {
    got_lock = host_lock("mq_timers", timers_lock);
  }

  if (!need_lock || got_lock) {
    id = ++max_timer_id;
    //float clock_expire = ((float)clock()) * msec_per_clock + (float)msec;
    uint32 clock_expire = get_tick_count() + msec;

    log_v("Setting timeout %d, msec: %d", id, msec);

    timers.insert(make_pair(id, make_pair(clock_expire, phandler)));

    // Put this item in the proper place in the ordered list
    map<int, pair<uint32, mq_handler_t *> >::const_iterator it_handler;
    bool inserted = false;
    for (deque<int>::iterator it = timers_order.begin(); it != timers_order.end(); ++it) {
      if ((it_handler = timers.find(*it)) != timers.end()) {
        if ((*it_handler).second.first >= clock_expire) {
          timers_order.insert(it, id);
          inserted = true;
          break;
        }
      }
    }

    if (!inserted) {
      timers_order.push_back(id);
    }

    log_v("setTimeout %d -- %d -- %d", msec, timers.size(), clock_expire);

    if (got_lock) {
      host_unlock("mq_timers", timers_lock);
    }
  }

  return id;
}

int net_mobilewebprint::mq_t::_dispatch_timers()
{
  map<int, pair<uint32, mq_handler_t *> >::iterator it;

  int id                        = 0;
  int count                     = 0;

  if (host_lock("mq_timers", timers_lock)) {
    mq_handler_extra_t extra(id, /*txn_id=*/0, /*txn_name=*/"", current_loop_num, current_loop_start);

    while (timers_order.size() > 0 && (id = *timers_order.begin()) && (it = timers.find(id)) != timers.end()) {

      int                 timer_id  = it->first;
      uint32              expiry    = it->second.first;
      mq_handler_t * handler   = it->second.second;

      log_v("dispatch_timer(%d)? %d -- %d -- %d > %d", timer_id, timers.size(), timers_order.size(), extra.time, expiry);
      if (expiry > extra.time) { break; }

      timers_order.pop_front();
      timers.erase(it);

      if (handler != NULL) {
        extra.id = id;
        handler->handle(timeout_msg, timeout_msg_view, NULL, extra);
        count += 1;
      }
    }

    host_unlock("mq_timers", timers_lock);
  }

  return count;
}

int net_mobilewebprint::mq_t::_msec_until_next_timer()
{
  int result = -1;

  map<int, pair<uint32, mq_handler_t *> >::iterator it;
  int id                        = 0;

  uint32 expiry = 0;
  if (host_lock("mq_timers", timers_lock)) {
    if (timers_order.size() > 0 && (id = *timers_order.begin()) && (it = timers.find(id)) != timers.end()) {
      expiry = it->second.first;
    }
    host_unlock("mq_timers", timers_lock);
  } else {
    return 2500;
  }

  if (expiry == 0) {
    return 2500;
  }

  result = expiry - get_tick_count();
  return result;
}

net_mobilewebprint::mq_enum::e_handle_result net_mobilewebprint::mq_t::on_loop_start()
{
  mq_select_loop_start_data_t loop_start_data = {0};

  loop_start_data.current_loop_num    = current_loop_num;
  loop_start_data.current_loop_start  = current_loop_start;

  for (map<string, deque<mq_handler_t *> >::const_iterator handler_it = handlerses.begin(); handler_it != handlerses.end(); ++handler_it) {
    deque<mq_handler_t *> const & handlers_for_name = handler_it->second;
    for (deque<mq_handler_t *>::const_iterator it = handlers_for_name.begin(); it != handlers_for_name.end(); ++it) {
      mq_handler_t * const & handler = *it;
      handler->on_select_loop_start(loop_start_data);
    }
  }

  return handled;
}

net_mobilewebprint::mq_enum::e_handle_result net_mobilewebprint::mq_t::on_loop_idle()
{
  mq_select_loop_idle_data_t loop_idle_data = {0};

  loop_idle_data.current_loop_num    = current_loop_num;
  loop_idle_data.current_loop_start  = current_loop_start;

  for (map<string, deque<mq_handler_t *> >::const_iterator handler_it = handlerses.begin(); handler_it != handlerses.end(); ++handler_it) {
    deque<mq_handler_t *> const & handlers_for_name = handler_it->second;
    for (deque<mq_handler_t *>::const_iterator it = handlers_for_name.begin(); it != handlers_for_name.end(); ++it) {
      mq_handler_t * const & handler = *it;
      handler->on_select_loop_idle(loop_idle_data);
    }
  }

  return handled;
}

static net_mobilewebprint::mq_pre_select_data_t & _setup_pre_select(net_mobilewebprint::mq_t & mq, net_mobilewebprint::mq_pre_select_data_t & pre_select_data)
{
  pre_select_data.status      = 0;
  pre_select_data.readable    = &mq.readable;
  pre_select_data.writable    = &mq.writable;
  pre_select_data.exceptional = &mq.exceptional;
  pre_select_data.max_fd      = -1;
  pre_select_data.timeout     = 0;

  return pre_select_data;
}

bool net_mobilewebprint::mq_t::_do_select(int msec_timeout, int & select_result)
{
  uint32 ds_start = get_tick_count();

  int orig_msec_timeout = msec_timeout;
  if (msec_timeout == -1) {
    msec_timeout = 0;
  }

  mq_pre_select_data_t pre_select_data = {0};

  int fd = 0;
  int max_fd = -1;

  FD_ZERO(&readable);
  FD_ZERO(&writable);
  FD_ZERO(&exceptional);

  _setup_pre_select(*this, pre_select_data);

  bool     have_curl_response = false;

  // Let the handlers know we are about to do the select()
  for (deque<mq_select_handler_t *>::const_iterator it = select_handlers.begin(); it != select_handlers.end(); ++it) {
    mq_select_handler_t * const & handler = *it;
    if (handler->pre_select(&pre_select_data) == 1) {
      // This is the response from CURL
      have_curl_response = true;
      max_fd = max(max_fd, pre_select_data.max_fd);
    }
  }

  int num_read_fds = 0;
  for (set<int>::iterator pfd = read_fds.begin(); pfd != read_fds.end(); ++pfd) {
    num_read_fds += 1;
    max_fd = max(max_fd, (fd = *pfd));
    FD_SET(fd, &readable);
    FD_SET(fd, &exceptional);
  }

  int num_write_fds = 0;
  for (set<int>::iterator pfd = write_fds.begin(); pfd != write_fds.end(); ++pfd) {
    num_write_fds += 1;
    max_fd = max(max_fd, (fd = *pfd));
    FD_SET(fd, &writable);
    FD_SET(fd, &exceptional);
  }

  fd_set *preadable = NULL, *pwritable = NULL, *pexceptional = NULL;
  struct timeval timeout;
  if (have_curl_response) {

    // This is what we got from CURL
    preadable       = pre_select_data.readable;
    pwritable       = pre_select_data.writable;
    pexceptional    = pre_select_data.exceptional;

    if (orig_msec_timeout != -1 && pre_select_data.timeout != -1) {
      if ((int)pre_select_data.timeout < msec_timeout) {
        //log_d(1, "", "Using smaller CURL timeout: %d < %d\n", (int)pre_select_data.timeout, msec_timeout);
        msec_timeout = (int)pre_select_data.timeout;
      }
    }

  } else {

#if 1
    preadable = pre_select_data.readable;
    pwritable = pre_select_data.writable;
    pexceptional = pre_select_data.exceptional;
#else
    // Make sure there's something to do
    if (num_read_fds == 0 && num_write_fds == 0) {
      select_result = 0;
      return false;
    }

    // This is how we have always done it
    if (num_read_fds > 0) {
      preadable = &readable;
    }

    if (num_write_fds > 0) {
      pwritable = &writable;
    }
#endif
  }

  if (msec_timeout < 0)   { msec_timeout = 0; }
  if (msec_timeout >1000) { msec_timeout = 1000; }

  //if (orig_msec_timeout == -1 && msec_timeout != 0) {
  //  log_d(1, "", "orig %d, using %d, curl: %d\n", orig_msec_timeout, msec_timeout, (int)pre_select_data.timeout);
  //}
  timeout.tv_sec  = msec_timeout / 1000;
  timeout.tv_usec = (msec_timeout % 1000) * 1000;

  read_fds_for_current_select = read_fds;
  write_fds_for_current_select = write_fds;
  write_fds = persistent_write_fds;

  //if (orig_msec_timeout != -1) {
  //  log_d(1, "", "Long timeout: %d: %d -- %d\n", msec_timeout, (int)timeout.tv_sec, (int)timeout.tv_usec);
  //}

  uint32 ds_A = get_tick_count();
  select_result = select(max_fd + 1, preadable, pwritable, pexceptional, &timeout);
  uint32 ds_B = get_tick_count();


  if (show_timing_stats) {
    if (orig_msec_timeout == -1) {
      //printf("zero-select (%d) running time: %d\n", select_result, ds_B - ds_A);
      do_select_0_a_running_time               += ds_A - ds_start;
      do_select_0_b_running_time               += ds_B - ds_A;
      do_select_0_total_running_time           += ds_B - ds_start;
    } else {
      //printf("select (%d) running time: %d\n", select_result, ds_B - ds_A);
      do_select_a_running_time                 += ds_A - ds_start;
      do_select_b_running_time                 += ds_B - ds_A;
      do_select_total_running_time             += ds_B - ds_start;
    }
  }

  return true;
}

e_handle_result net_mobilewebprint::mq_t::dispatch_to_select_handlers(int id, int txn_id, buffer_range_view_t const & buffer_view, buffer_t * msg)
{
  e_handle_result result = enot_impl, handled_result = enot_impl;

  mq_handler_extra_t h_extra(id, txn_id, /*txn_name=*/"", current_loop_num, current_loop_start);

  for (deque<mq_select_handler_t *>::const_iterator it_handler = select_handlers.begin(); it_handler != select_handlers.end(); ++it_handler) {
    mq_select_handler_t * handler = *it_handler;
    mq_select_handler_extra_t extra(*this, h_extra);

    uint32 start = get_tick_count();
    if ((handled_result = handler->_mq_selected(selected_msg, buffer_view, msg, extra)) != enot_impl && handled_result != not_impl) {
      result = handled_result;
    }

    uint32 now = get_tick_count();
    if (now - start > 750) {
      log_w("mq_dispatch", "Handler %s for %s took too long: %f seconds.", handler->mod_name().c_str(), "mq_selected", ((float)now - start) / 1000.0);
    }

    if (handled_result == handled_and_took_message) {
      break;
    }
  }

  return handled_result;
}

void net_mobilewebprint::mq_t::_clear_fd_state()
{
  read_fds_for_current_select.empty();

  write_fds = _union(write_fds, write_fds_for_current_select);
  write_fds_for_current_select.empty();

  FD_ZERO(&readable);
  FD_ZERO(&writable);
  FD_ZERO(&exceptional);
}

net_mobilewebprint::mq_t & net_mobilewebprint::mq_t::register_udp_for_select(network_node_t & node)
{
  // Put the fd into the read list, and the write-once list
  int fd = node.udp();

  if (fd) {
    add_read(fd);
    add_write(fd);
  }

  return *this;
}

net_mobilewebprint::mq_t & net_mobilewebprint::mq_t::check_udp_read(network_node_t & node)
{
  int fd = node.udp();

  if (fd) {
    add_read(fd);
  }

  return *this;
}

net_mobilewebprint::mq_t & net_mobilewebprint::mq_t::check_udp_write(network_node_t & node)
{
  int fd = node.udp();

  if (fd) {
    add_write(fd);
  }

  return *this;
}

net_mobilewebprint::mq_t & net_mobilewebprint::mq_t::register_tcp_for_select(network_node_t & node, bool persistent_write)
{
  // Put the fd into the read list, and the write-once list
  int fd = node.connect();

  if (fd) {
    add_read(fd);
    add_write(fd, persistent_write);
  }

  return *this;
}

net_mobilewebprint::mq_t & net_mobilewebprint::mq_t::check_tcp_read(network_node_t & node)
{
  int fd = node.connect();

  if (fd) {
    add_read(fd);
  }

  return *this;
}

net_mobilewebprint::mq_t & net_mobilewebprint::mq_t::check_tcp_write(network_node_t & node)
{
  int fd = node.connect();

  if (fd) {
    add_write(fd);
  }

  return *this;
}

net_mobilewebprint::mq_t & net_mobilewebprint::mq_t::deregister_for_select(network_node_t & node)
{
  int fd = node.tcp_fd != 0 ? node.tcp_fd : node.udp_fd;

  if (fd) {
    remove_read(fd);
    _remove_write(fd);
  }

  return *this;
}

int net_mobilewebprint::mq_t::is_udp_readable(network_node_t & node)
{
  int fd = node.udp();
  return fd && is_fd_readable(fd);
}

int net_mobilewebprint::mq_t::is_udp_writable(network_node_t & node)
{
  int fd = node.udp();
  return fd && is_fd_writable(fd);
}

int net_mobilewebprint::mq_t::is_udp_exception(network_node_t & node)
{
  int fd = node.udp();
  return fd && is_fd_exception(fd);
}

int net_mobilewebprint::mq_t::is_tcp_readable(network_node_t & node)
{
  int fd = node.connect();
  return fd && is_fd_readable(fd);
}

int net_mobilewebprint::mq_t::is_tcp_writable(network_node_t & node)
{
  int fd = node.connect();
  return fd && is_fd_writable(fd);
}

int net_mobilewebprint::mq_t::is_tcp_exception(network_node_t & node)
{
  int fd = node.connect();
  return fd && is_fd_exception(fd);
}

net_mobilewebprint::mq_t & net_mobilewebprint::mq_t::add_read(int fd)
{
  read_fds.insert(fd);
  return *this;
}

net_mobilewebprint::mq_t & net_mobilewebprint::mq_t::add_write(int fd, bool persistent)
{
  if (persistent) {
    persistent_write_fds.insert(fd);
  } else {
    write_fds.insert(fd);
  }
  return *this;
}

net_mobilewebprint::mq_t & net_mobilewebprint::mq_t::remove_read(int fd)
{
  read_fds.erase(fd);
  return *this;
}

net_mobilewebprint::mq_t & net_mobilewebprint::mq_t::_remove_write(int fd)
{
  write_fds.erase(fd);
  persistent_write_fds.erase(fd);
  return *this;
}

int _num_fd_set(std::set<int> const & fds, fd_set & set_of_fds)
{
  int result = 0;
  std::set<int>::const_iterator it;

  for (it = fds.begin(); it != fds.end(); ++it) {
    if (FD_ISSET(*it, &set_of_fds)) {
      result += 1;
    }
  }

  return result;
}

int net_mobilewebprint::mq_t::_num_curr_reads()
{
  return _num_fd_set(read_fds_for_current_select, readable);
}

int net_mobilewebprint::mq_t::_num_curr_writes()
{
  return _num_fd_set(write_fds_for_current_select, writable);
}

int net_mobilewebprint::mq_t::_num_curr_exceptions()
{
#if 1
  return 0;
#else
  return _num_fd_set(_union(read_fds_for_current_select, write_fds_for_current_select), exceptional);
#endif
}

bool net_mobilewebprint::mq_t::is_fd_part_of_select(network_node_t & node)
{
  int fd = node.tcp_fd != 0 ? node.tcp_fd : node.udp_fd;

  if (read_fds_for_current_select.find(fd) != read_fds_for_current_select.end()) { return true; }
  if (write_fds_for_current_select.find(fd) != write_fds_for_current_select.end()) { return true; }

  return false;
}

int net_mobilewebprint::mq_t::is_fd_readable(int fd)
{
  // The fd must have been in the list when we did the select()
  if (read_fds_for_current_select.find(fd) == read_fds_for_current_select.end()) { return 0; }

  return FD_ISSET(fd, &readable);
}

int net_mobilewebprint::mq_t::is_fd_writable(int fd)
{
  // The fd must have been in the list when we did the select()
  if (write_fds_for_current_select.find(fd) == write_fds_for_current_select.end()) { return 0; }

  int result = FD_ISSET(fd, &writable);

  // Auto-remove the fd if it is writable
  if (result) {
    write_fds_for_current_select.erase(fd);
  }

  return result;
}

int net_mobilewebprint::mq_t::is_fd_exception(int fd)
{
  // The fd must have been in the list when we did the select()
  if (read_fds_for_current_select.find(fd)  != read_fds_for_current_select.end() ||
      write_fds_for_current_select.find(fd) != write_fds_for_current_select.end())
  {
    return FD_ISSET(fd, &exceptional);
  }

  return 0;
}

net_mobilewebprint::buffer_t const * net_mobilewebprint::mq_t::pull()
{
  if (host_lock("mq_normal", lock)) {
    buffer_t const * result = NULL;

    if (!result) {
      if (mq_high.size() > 0) {
        result = *(mq_high.begin());
        mq_high.pop_front();
      }
    }

    if (!result) {
      if (mq_normal.size() > 0) {
        result = *(mq_normal.begin());
        mq_normal.pop_front();
      }
    }

    host_unlock("mq_normal", lock);
    return result;
  }

  /* otherwise */
  return NULL;
}

/* static */ void* net_mobilewebprint::mq_t::_thread_start_fn(void *pvthat)
{
  mq_t * that = (mq_t*)pvthat;
  that->dispatch_loop();
  return NULL;
}






// -------------------------------------------------------------------------------------------------
//
net_mobilewebprint::mq_enum::e_handle_result net_mobilewebprint::mq_handler_t::on_select_loop_start(mq_select_loop_start_data_t const & data)
{
  return not_impl;
}

net_mobilewebprint::mq_enum::e_handle_result net_mobilewebprint::mq_handler_t::on_select_loop_idle(mq_select_loop_idle_data_t  const & data)
{
  return not_impl;
}

// -------------------------------------------------------------------------------------------------
//
net_mobilewebprint::handler_holder_by_id_t::handler_holder_by_id_t(char const * mod_name, int id, mq_base_handler_fn_t fn, void * that)
  : id(id), mod_name_(mod_name), fn(fn), that(that)
{
}

std::string net_mobilewebprint::handler_holder_by_id_t::mod_name()
{
  return mod_name_;
}

e_handle_result net_mobilewebprint::handler_holder_by_id_t::handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra)
{
  if (extra.id == id) {
    return fn(that, name, payload, data, extra);
  }

  return not_impl;
}

