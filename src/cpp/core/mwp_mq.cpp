
#include "mwp_mq.hpp"
#include "mwp_host.hpp"

#include <string>

using net_mobilewebprint::buffer_t;
using net_mobilewebprint::mq_result;
using net_mobilewebprint::mq_t;
using net_mobilewebprint::uint32;
using net_mobilewebprint::host::get_tick_count;
using std::string;

static uint32 LOOP_START_BIT                     = 0x00000010;
static uint32 ZERO_TIMEOUT_SELECT_BIT            = 0x00000020;
static uint32 SELECT_DISPATCH_BIT                = 0x00000040;
static uint32 MESSAGE_PARSING_BIT                = 0x00000080;
static uint32 MESSAGE_DISPATCH_BIT               = 0x00000100;
static uint32 LONG_TIMEOUT_SELECT_BIT            = 0x00000200;

//---------------------------------------------------------------------------------------------------
//------------------------------------- mq_t --------------------------------------------------------
//---------------------------------------------------------------------------------------------------
net_mobilewebprint::mq_t::mq_t()
  : current_loop_num(0), current_loop_start(0), current_loop_second(0),
    messages_lock(NULL),
    max_fd(-1),
    loop_start_stats(NULL),
    zero_timeout_select_stats(NULL),
    select_dispatch_stats(NULL),
    message_parsing_stats(NULL),
    message_dispatch_stats(NULL),
    long_timeout_select_stats(NULL),
    current_loop_stats(NULL)
{
  loop_stats.push_back(loop_start_stats                    = new mq_loop_stats_t(*this, LOOP_START_BIT));
  loop_stats.push_back(zero_timeout_select_stats           = new mq_loop_stats_t(*this, ZERO_TIMEOUT_SELECT_BIT));
  loop_stats.push_back(select_dispatch_stats               = new mq_loop_stats_t(*this, SELECT_DISPATCH_BIT));
  loop_stats.push_back(message_parsing_stats               = new mq_loop_stats_t(*this, MESSAGE_PARSING_BIT));
  loop_stats.push_back(message_dispatch_stats              = new mq_loop_stats_t(*this, MESSAGE_DISPATCH_BIT));
  loop_stats.push_back(long_timeout_select_stats           = new mq_loop_stats_t(*this, LONG_TIMEOUT_SELECT_BIT));

  zero_timeout.tv_sec   = 0;
  zero_timeout.tv_usec  = 0;
  long_timeout.tv_sec   = 0;
  long_timeout.tv_usec  = 500000;   // .5 sec
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
  bool   socket_was_active  = false;

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
    socket_was_active     = false;

    loop_start_stats->start();
    _on_select_loop_start();

    zero_timeout_select_stats->start();
    int num_zero_timeout_selects = _on_zero_select();

    //printf("Number of selects(0): %d\n", num_zero_timeout_selects);

    if (num_zero_timeout_selects > 0) {
      //printf("Dispatching zero select\n");
      select_dispatch_stats->start();
      _on_select_dispatch();

      // Network events get priority, but we do not starve other processing
      socket_was_active = true;
      if ((current_loop_num & 0x01) == 0) {
        continue;
      }
    }

    // Now dispatch events from the mq
    buffer_t * msg = pull();
    if (msg != NULL) {
      message_extra_t message_extra(*this);

      message_parsing_stats->start();
      bool parsed = parse_message(msg, message_extra);
      //printf("Pulled message %s, parsed: %d\n", message_extra.name.c_str(), (int)parsed);

      if (parsed) {
        message_dispatch_stats->start();
        mq_result dispatch_result = _on_message_dispatch(msg, message_extra);

        if (dispatch_result != ok_and_took_memory) {
          delete msg;
        }
      }

      // We processed a message. Go back and see if there are more network events to work on.
      if ((current_loop_num & 0x03) == 0) {
        continue;
      }
    }

    // TODO: idle work dispatched here

    // ------------------------------------------------------------------------------------
    // Under no circumstances should we ever do a sleep if there is still work to do
    // ------------------------------------------------------------------------------------

    if (messages.size() > 0 || socket_was_active) {
      continue;
    }

    // Now do a select with a timeout.  This is how the loop 'sleeps'
    long_timeout_select_stats->start();
    int num_long_timeout_selects = _on_long_select(500);

    //printf("Number of selects(long): %d\n", num_long_timeout_selects);

    if (num_long_timeout_selects > 0) {
      //printf("Dispatching long select\n");
      select_dispatch_stats->start();
      _on_select_dispatch();
    }

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

int net_mobilewebprint::mq_t::_on_zero_select()
{
//  printf("MQ on_zero_select\n");
  max_fd    = -1;

  FD_ZERO(&readable);
  FD_ZERO(&writable);
  FD_ZERO(&exceptional);

  pre_select_extra_t pre_select_extra(*this);

  for (deque<mq_handler_t *>::iterator it = handlers.begin(); it != handlers.end(); ++it) {
    mq_handler_t *& handler = *it;
    if (handler != NULL) {
//      printf("MQ::handler on_zero_select\n");
      handler->on_pre_select(pre_select_extra);
    }
  }

  // Do the select statement
  int result = select(max_fd + 1, &readable, &writable, &exceptional, &zero_timeout);

  return result;
}

int net_mobilewebprint::mq_t::_on_long_select(uint32 msec_timeout)
{
  max_fd    = -1;

  FD_ZERO(&readable);
  FD_ZERO(&writable);
  FD_ZERO(&exceptional);

  pre_select_extra_t pre_select_extra(*this);

  for (deque<mq_handler_t *>::iterator it = handlers.begin(); it != handlers.end(); ++it) {
    mq_handler_t *& handler = *it;
    if (handler != NULL) {
      handler->on_pre_select(pre_select_extra);
    }
  }

  // Do the select statement
  int result = select(max_fd + 1, &readable, &writable, &exceptional, &long_timeout);

  return result;
}

void net_mobilewebprint::mq_t::_on_select_dispatch()
{
  select_extra_t extra(*this);

  for (deque<mq_handler_t *>::iterator it = handlers.begin(); it != handlers.end(); ++it) {
    mq_handler_t *& handler = *it;
    if (handler != NULL) {
      handler->on_select(extra);
    }
  }
}

mq_result net_mobilewebprint::mq_t::_on_message_dispatch(buffer_t * msg, message_extra_t & extra)
{
  mq_result result = ok;

  printf("on_message_dispatch %s, %d\n", extra.name.c_str(), (int)handlers.size());

  for (deque<mq_handler_t *>::iterator it = handlers.begin(); it != handlers.end(); ++it) {
    mq_handler_t *& handler = *it;
    if (handler != NULL) {
      mq_result handler_result = handler->on_message(extra.name, *extra.payload, extra);

      if (handler_result == ok_and_took_memory) {
        result = handler_result;
        break;
      }
    }
  }

  return result;
}

buffer_t * net_mobilewebprint::mq_t::message(char const * name, uint32 id, uint32 seq_num)
{
  uint32 offset = 0;
  return message(name, id, 0, seq_num, offset);
}

buffer_t * net_mobilewebprint::mq_t::message(char const * name, uint32 id, uint32 seq_num, size_t num_extra, uint32 & user_offset)
{
  size_t name_len = strlen(name);
  if (name_len > 30) {
    name_len = 30;
  }

  buffer_t * buffer = new buffer_t;
  buffer->zero_pad(1 + 31 + sizeof(id) + sizeof(seq_num) + num_extra);

  byte * p = buffer->bytes;

  *p++ = 1;

  memcpy(p, name, name_len);     p += 31;
  *(uint32*)p = htonl(id);       p += sizeof(id);
  *(uint32*)p = htonl(seq_num);  p += sizeof(seq_num);

  user_offset = p - buffer->bytes;
  return buffer;
}

bool net_mobilewebprint::mq_t::parse_message(buffer_t * msg, message_extra_t & extra)
{
  buffer_reader_t reader(*msg);

  /* byte message_type = */ reader.read_byte();

  extra.data        = msg;
  extra.name        = reader.read_string_nz(31);
  extra.id          = reader.read_uint32();
  extra.seq_num     = reader.read_uint32();
  extra.payload     = new buffer_range_t(reader.p, msg->end());

  return true;
}

void net_mobilewebprint::mq_t::send(buffer_t * msg)
{
  messages.push_back(msg);
}

void net_mobilewebprint::mq_t::send(char const * name)
{
  buffer_t * msg = NULL;

  if ((msg = message(name, 0, 0)) != NULL) {
    messages.push_back(msg);
  }
}

buffer_t * net_mobilewebprint::mq_t::pull()
{
  if (host::lock("mq_messages", messages_lock)) {
    buffer_t * result = NULL;

    if (!result) {
      if (messages.size() > 0) {
        result = *(messages.begin());
        messages.pop_front();
      }
    }

    host::unlock("mq_messages", messages_lock);
    return result;
  }

  /* otherwise */
  return NULL;
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
//------------------------------------- select_extra_base_t -----------------------------------------
//---------------------------------------------------------------------------------------------------
net_mobilewebprint::select_extra_base_t::select_extra_base_t(mq_t & mq_)
  : extra_t(mq_),
    readable(mq_.readable), writable(mq_.writable), exceptional(mq_.exceptional)
{
}

void net_mobilewebprint::select_extra_base_t::fd_set_readable(int fd)
{
  FD_SET(fd, &mq.readable);
  FD_SET(fd, &mq.exceptional);

  if (fd > mq.max_fd) {
    mq.max_fd = fd;
  }
}

void net_mobilewebprint::select_extra_base_t::fd_set_writable(int fd)
{
  FD_SET(fd, &mq.writable);
  FD_SET(fd, &mq.exceptional);

  if (fd > mq.max_fd) {
    mq.max_fd = fd;
  }
}

bool net_mobilewebprint::select_extra_base_t::is_readable(int fd)
{
  return FD_ISSET(fd, &readable);
}

bool net_mobilewebprint::select_extra_base_t::is_writable(int fd)
{
  return FD_ISSET(fd, &writable);
}

bool net_mobilewebprint::select_extra_base_t::is_exceptional(int fd)
{
  return FD_ISSET(fd, &exceptional);
}


//---------------------------------------------------------------------------------------------------
//------------------------------------- pre_select_extra_t ------------------------------------------
//---------------------------------------------------------------------------------------------------
net_mobilewebprint::pre_select_extra_t::pre_select_extra_t(mq_t & mq_)
  : select_extra_base_t(mq_)
{
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- select_extra_t ----------------------------------------------
//---------------------------------------------------------------------------------------------------
net_mobilewebprint::select_extra_t::select_extra_t(mq_t & mq_)
  : select_extra_base_t(mq_)
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
net_mobilewebprint::mq_handler_t::~mq_handler_t()
{
  // Delete any remaining request packets
  for (deque<buffer_t *>::const_iterator it = requests.begin(); it != requests.end(); ++it) {
    buffer_t * const & pbuf = *it;
    delete pbuf;
  }

  requests.empty();
}

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

bool net_mobilewebprint::mq_handler_t::should_send()
{
  return true;
}

void net_mobilewebprint::mq_handler_t::push_request(buffer_t * req)
{
  if (!should_send()) { return; }

  requests.push_back(req);
}

buffer_t * net_mobilewebprint::mq_handler_t::pull_next_request()
{
  return _pull(requests);
}

