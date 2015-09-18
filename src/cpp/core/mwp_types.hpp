/**
 *  mwp_types.hpp

    In the world of network programming, a few metaphors and idioms are pervasive.
    In this file we implement them.

    buffer
    ------

    buffer_t - a managed chunk of memory.

    network node
    ------------

    network_node_t - another node on the network (essentially an IP/port pair)
 */
#ifndef __MWP_TYPES_HPP__
#define __MWP_TYPES_HPP__

#include "mwp_core_config.h"

#include "hp_mwp.h"

#include <string>
#include <vector>
#include <set>
#include <map>
#include <deque>
#include <cctype>
#include <algorithm>

#ifdef WIN32
#include <Winsock2.h>
#include <Ws2tcpip.h>

#define mwp_sprintf sprintf_s
#define mwp_socket_close closesocket

#else
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>

#if HAVE_IF_ETHER_H
#include <netinet/if_ether.h>
#define HAVE_SIOCGARP 1
#endif

#if HAVE_IF_ARP_H
#include <net/if_arp.h>
#endif

#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define mwp_sprintf sprintf
#define mwp_socket_close ::close

#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define MWP_STRLEN_BUFFER_STRING 31

namespace net_mobilewebprint {

  using std::string;
  using std::vector;
  using std::set;

  struct controller_base_t;
  struct slp_t;
  struct mdns_t;
  struct mq_t;

  extern mq_t & get_mq(controller_base_t &);
  extern int    num_allocations;
  extern int    num_buffer_allocations;
  extern int    num_buf_bytes_allocations;
  extern int    num_buf_bytes;
  extern int    num_chunk_allocations;

  typedef std::map<string, string>    strmap;
  typedef std::pair<string, string>   strmap_entry;
  typedef std::map<string, int>       intmap;
  typedef std::pair<string, int>      intmap_entry;
  typedef std::map<string, bool>      boolmap;
  typedef std::pair<string, bool>     boolmap_entry;

  typedef std::deque<string>          strlist;
  typedef std::vector<string>         strvlist;
  typedef std::set<string>            strset;

  // -------------------------------------------------------------------------------------------
  // The hosting implementation must provide these -- see below for examples

  // ----- General -----
  char const * platform_name();

  // Logging -- the weird signature makes it unlikely that this will clash with the printf-style logging

  // The hosting application should send the msg to the debug logging mechanism; it should NOT append a newline.
  typedef std::map<bool, double>::const_iterator * log_param_t;
  void log_d(char const * msg, log_param_t /* never used */);
  void log_d(char const * msg, char const * tag, log_param_t /* never used */);
  void log_w(char const * msg, log_param_t /* never used */);
  void log_v(char const * msg, log_param_t /* never used */);
  void log_v(char const * msg, char const * tag, log_param_t /* never used */);
  void log_e(char const * msg, log_param_t /* never used */);

  // The hosting application should do the platform/dev-tool normal "assert" functionality, and return
  // the passed-in parameter as-is.
  //     NOTE: it should "assert" as the C/C++/POSIX community knows it.  (Said another way: under MSVC
  //     it should use "verify" as verify gets compiled into release builds, where assert does not.
  void * mwp_assert(void *);
  int    mwp_assert(int);
  bool   mwp_assert(bool);

  // The hosting applicaiton needs to translate the host-style string to a C standard ASCII string.
  //     NOTE: the networking functions expect ASCII strings, irrespective of the "normal" platform
  //     preference for other string formats like UNICODE.
  string platform_to_ascii_string(void const *str);

  // ----- Timing and async -----

  // The MWP library NEEDS to run it's dispatch loop in a "separate" (non-UI) thread.  The hosting
  // applicaiton has two choices to provide this thread under the OSs model: 1) Start a thread in
  // this start_thread() function, calling the function and passing the data; or 2) Call this library's
  // start function in dedicated, separate thread.  In the case of (2), return false from start_thread()
  // to indicate that the start function is already in a separate thread.
  bool start_thread(void*(*pfn)(void*data), void*data);

  // The hosting applicatoin should call the OS's sleep mechanism to swap-out this thread, but to do it
  // in a way that will wake the thread on network events.
  bool interruptable_sleep(int msec);

  // The hosting applicaiton should return the number of msec (1/1000th of a second) since a reliable
  // starting point.  (The difference between two such values should be an accurate count of msec's that
  // have passed in the duration.)  The most common thing that OSs provide are "msec since Jan 1, 1970"
  // or "msec since app start".  Either will work.
  uint32 get_tick_count();

  // The library needs to protect the event queue with a mutual-exclusion lock (a MUTEX).  The hosting
  // application needs to provide this capability.
  bool allocate_host_lock(char const * name, void ** result_lock_out);
  bool host_lock(char const * name, void * lock);
  bool host_unlock(char const * name, void * lock);
  void free_host_lock(char const * name, void * lock);

  // The hosting application should set the socket to blocking or non-blocking
  bool set_socket_non_blocking(int fd);
  bool set_socket_blocking(int fd);
  bool connect_in_progress();
  int  get_last_network_error();

  // END -- The hosting implementation must provide these
  // -------------------------------------------------------------------------------------------

  namespace dumb_and_ok {
    void *    mwp_assert(void *);
    int       mwp_assert(int);
    bool      mwp_assert(bool);

    string    platform_to_ascii_string(void const *str);      // Just returns str
  };

  // Mario has POSIX implementations for these.  If they work for your platform, just route
  // calls to these implementations
  namespace POSIX {
    bool                  start_thread(void*(*pfn)(void*data), void*data);

    bool            allocate_host_lock(char const * name, void ** result_lock_out);
    bool                     host_lock(char const * name, void * lock);
    bool                   host_unlock(char const * name, void * lock);
    void                free_host_lock(char const * name, void * lock);

    bool       set_socket_non_blocking(int fd);
    bool           set_socket_blocking(int fd);
    bool           connect_in_progress();
    int         get_last_network_error();

    bool           interruptable_sleep(int msec);
    uint32              get_tick_count();                     // Uses gettimeofday()
  };

  void log_d(string const & msg);
  void log_w(string const & msg);
  void log_v(string const & msg);
  void log_e(string const & msg);

//  void log_d(char const * tag, string const & msg);
  void log_w(char const * tag, string const & msg);
  void log_v(int level, char const * tag, string const & msg);
  void log_e(char const * tag, string const & msg);

  void log_d(char const * format, ...);
//  void log_w(char const * format, ...);
  void log_v(char const * format, ...);
//  void log_e(char const * format, ...);

  void log_d(int level, char const * tag, char const * format, ...);
  void log_d(char level, char const * tag, char const * format, ...);
  void log_w(char const * tag, char const * format, ...);
  void log_v(int level, char const * tag, char const * format, ...);
  void log_vs(int level, char const * tag, char const * format, string const & big_str);
  void log_vs(int level, char const * tags, char const * format, string const & s1, string const & s2);
  void log_vs(int level, char const * tags, char const * format, string const & s1, string const & s2, string const & s3);
  void log_e(char const * tag, char const * format, ...);

  typedef unsigned char byte;

  extern int bytes_sent;
  extern strmap mac_addresses;
  extern strmap new_mac_addresses;

#ifdef WIN32
#define inet_pton InetPtonA
  typedef int socklen_t;
#endif

  struct buffer_view_i
  {
    typedef byte *        iterator;
    typedef byte const *  const_iterator;

    virtual byte *   const_begin() const = 0;
    virtual byte *     const_end() const = 0;
    virtual size_t         dsize() const = 0;

    virtual byte *         begin() = 0;
    virtual byte *           end() = 0;

    byte at(int offset) const;
    uint16 uint16_at(int offset) const;
    uint32 uint32_at(int offset) const;

    virtual const_iterator  first() const;
    virtual bool            is_valid(const_iterator it) const;

    virtual byte            read_byte(const_iterator & it) const;
    virtual uint16          read_uint16(const_iterator & it) const;
    virtual uint32          read_uint32(const_iterator & it) const;
    virtual string          read_string(const_iterator & it) const;
    virtual string          read_string_nz(const_iterator & it, size_t count) const;
    virtual string          read_ip(const_iterator & it) const;

    virtual bool            read_string_nz(const_iterator & it, size_t count, char const *& begin, char const *& end) const;

    string                  str() const;
    string                  to_lower() const;

    virtual void            dump(char const * mod_name, char const * msg = NULL, int limit = 640) const;
  };

  extern bool   parse_slp(slp_t & slp, buffer_view_i const & payload, std::map<string, string> & attrs, std::map<string, string> & attrs_lc, std::deque<string> & other_entries);

  // A view into a buffer-ish object
  //
  //      !!!   You cannot add a new type if that type has byte-ordering issues   !!!
  //
  namespace bv_non_endian {

    byte const * begin(buffer_view_i const & buffer);
    byte const *   end(buffer_view_i const & buffer);
    size_t       dsize(buffer_view_i const & buffer);

    byte const * begin(string const & str);
    byte const *   end(string const & str);
    size_t       dsize(string const & str);

    byte const * begin(char const * str);
    byte const *   end(char const * str);
    size_t       dsize(char const * str);

    byte const * begin(char str[MWP_STRLEN_BUFFER_STRING]);
    byte const *   end(char str[MWP_STRLEN_BUFFER_STRING]);
    size_t       dsize(char str[MWP_STRLEN_BUFFER_STRING]);

    byte const * begin(std::pair<char const *, char const *> const &);
    byte const *   end(std::pair<char const *, char const *> const &);
    size_t       dsize(std::pair<char const *, char const *> const &);

    byte const * begin(std::pair<byte const *, byte const *> const &);
    byte const *   end(std::pair<byte const *, byte const *> const &);
    size_t       dsize(std::pair<byte const *, byte const *> const &);

    byte const * begin(byte const * pby);
    byte const *   end(byte const * pby);
    size_t       dsize(byte const * pby);

    byte const * begin(byte const & n);
    byte const *   end(byte const & n);
    size_t       dsize(byte const & n);

  };

  /**
   *  buffer_tt<byte>

      A big 'ol chunk of memory.  Probably destined to be the payload of a
      network data transfer.
   */
  template <typename Tch>
  struct buffer_tt : public net_mobilewebprint::buffer_view_i
  {
    byte*  bytes;
    size_t mem_length, data_length;
    size_t grow_size;

    /**
     *  Default ctor.
     */
    buffer_tt()
      : bytes(NULL), mem_length(0), data_length(0), grow_size(128)
    {
      _debug();
    }

    /**
     *  Copy ctor.
     */
    buffer_tt(buffer_tt const & that)
      : bytes(NULL), mem_length(0), data_length(0), grow_size(that.grow_size)
    {
      cpy(resize(that.mem_length), that.bytes, data_length = that.data_length);
      _debug();
    }

    /**
     *  Copy ctor2.
     */
    buffer_tt(buffer_tt const & that, bool)
      : bytes(NULL), mem_length(0), data_length(0), grow_size(that.grow_size)
    {
      cpy(resize(that.data_length), that.bytes, data_length = that.data_length);
      _debug();
    }

    /**
     *  Data ctor.
     *
     *  Construct from a C-string
     */
    buffer_tt(char const* str)
      : bytes(NULL), mem_length(0), data_length(0), grow_size(128)
    {
      cpy(resize((size_t)strlen(str) + 1), (byte const *)str, data_length = (size_t)strlen(str) + 1);
      _debug();
    }

    /**
     *  Data ctor.
     *
     *  Allocate 'mem_length' bytes.
     *
     *  This ctor is good to use if you know the total size of the buffer, but don't
     *  have a continuous chunk of memory to start with.  Using this ctor, buffer_t
     *  will only allocate memory once.
     */
    buffer_tt(size_t mem_length)
      : bytes(NULL), mem_length(mem_length), data_length(mem_length), grow_size(128)
    {
      bytes = new byte[mem_length]; /**/ num_buf_bytes_allocations += 1; num_buf_bytes += mem_length;
      memset(bytes, 0, mem_length);
      _debug();
    }

    /**
    *  Data ctor.
    *
    *  Copy 'data_length' bytes, starting at 'bytes', but allocate 'extra_mem_length' extra bytes.
    *
    *  This ctor is good to use if you have an initial chunk of memory, and have a good guess at
    *  how many more bytes you will need eventually.  Construct with this ctor, then use append().
    *  Using this ctor, buffer_t will only allocate memory once.
    */
    buffer_tt(byte const* bytes_, size_t data_length_, size_t extra_mem_length = 0)
      : bytes(NULL), mem_length(0), data_length(0), grow_size(128)
    {
      cpy(resize(data_length_ + extra_mem_length), bytes_, data_length = data_length_);
      _debug();
    }

    /**
    *  Data ctor.
    *
    *  Copy 'data_length' bytes, starting at 'bytes', but allocate 'extra_mem_length' extra bytes.
    *
    *  This ctor is good to use if you have an initial chunk of memory, and have a good guess at
    *  how many more bytes you will need eventually.  Construct with this ctor, then use append().
    *  Using this ctor, buffer_t will only allocate memory once.
    *
    *  Fill the memory with the value in fill_value.
    */
    buffer_tt(byte const* bytes_, size_t data_length_, size_t extra_mem_length, byte fill_value)
      : bytes(NULL), mem_length(0), data_length(0), grow_size(128)
    {
      cpy(resize(data_length_ + extra_mem_length), bytes_, data_length = data_length_);

      ::memset(bytes + data_length, fill_value, extra_mem_length);
      data_length += extra_mem_length;
      _debug();
    }

    /**
     *  Data ctor.
     *
     *  Copy a chunk of memory out of another buffer_t.
     */
    buffer_tt(buffer_tt const & that, int offset)
      : bytes(NULL), mem_length(0), data_length(0), grow_size(128)
    {
      if (offset < 0) {
        // Copy from that point onwards
        data_length = mem_length = that.data_length + offset;
        cpy(resize(mem_length), that.bytes - offset, data_length);
        return;
      }

      // Copy from the beginning of the buffer up to, but not including the offset
      bytes = new byte[mem_length = data_length = offset]; /**/ num_buf_bytes_allocations += 1; num_buf_bytes += mem_length;
      ::memcpy(bytes, that.bytes, offset);
    }

    virtual ~buffer_tt()
    {
      if (bytes != NULL) {
        delete[] bytes; /**/ num_buf_bytes_allocations -= 1; num_buf_bytes -= mem_length;
      }
    }

    buffer_tt & operator=(buffer_tt const & that)
    {
      if (this != &that) {
        cpy(resize(that.mem_length), that.bytes, data_length = that.data_length);
      }

      return *this;
    }

    void clear() {
      data_length = 0;
    }

    /**
     *  Fills bytes to help debug.
     */
    buffer_tt & _debug()
    {
      if (mem_length == data_length) { return *this; }    // No extra bytes to fill

      int    length = (int)(mem_length - data_length), i = 0;
      byte * first  = bytes + data_length;
      byte * last   = first + length;

      ::memset(first, 0, length);                         // Clear the empty memory

      if (length < 12) { return *this; }

      // Write "deadbeef" to the beginning and end
      last -= 8;
      *first++ = *last++ = 'd';
      *first++ = *last++ = 'e';
      *first++ = *last++ = 'a';
      *first++ = *last++ = 'd';
      *first++ = *last++ = 'b';
      *first++ = *last++ = 'e';
      *first++ = *last++ = 'e';
      *first++ = *last++ = 'f';
      last -= 8;

      i = 0;
      for (byte * p = first; p < last && i < 256; ++p, ++i) {
        *p = 'a';
      }

      i = 0;
      for (byte * p = last - 1; p >= first && i < 256; --p, ++i) {
        *p = 'z';
      }

      return *this;
    }

    byte * resize(size_t new_mem_length) {

      size_t orig_mem_length = mem_length;
      if (new_mem_length <= mem_length) {
        // We alerady have the right size
        return bytes;
      }

      /* otherwise */
      size_t num_to_increase_by = max(new_mem_length - mem_length, grow_size);
      //log_d("Increasing buffer size %d -> %d (%d)", (int)mem_length, (int)(mem_length + num_to_increase_by), (int)new_mem_length);
      byte * temp = new byte[mem_length = (mem_length + num_to_increase_by)]; /**/ num_buf_bytes_allocations += 1; num_buf_bytes += mem_length;
      memset(temp, 0, mem_length);

      if (bytes != NULL) {
        if (data_length > 0) {
          memcpy(temp, bytes, data_length);
        }
        delete[] bytes; /**/ num_buf_bytes_allocations -= 1; num_buf_bytes -= orig_mem_length;
      }

      bytes = temp;

      return bytes;
    }

    size_t cpy(byte * dest, byte const * src, size_t count) {
      size_t num_to_cpy = min(count, mem_length);
      if (dest == NULL || num_to_cpy == 0) { return 0; }

      /* otherwise */
      memcpy(bytes = dest, src, num_to_cpy);

      return num_to_cpy;
    }

    void fill_in(byte by)
    {
      resize(data_length + 1);

      *(bytes + data_length) = by;
      data_length += 1;

      _debug();
    }

    void fill_back(byte by)
    {
      resize(data_length + 1);

      byte * p = NULL;

      data_length += 1;
      for (p = bytes + data_length; p > bytes; --p) {
        *p = *(p - 1);
      }
      *p = by;

      _debug();
    }

    template <typename T> int appendT(T that)
    {
      int result = append(bv_non_endian::begin(that), bv_non_endian::dsize(that));

      _debug();
      return result;
    }

    template <typename T1, typename T2> int appendT(T1 that1, T2 that2)
    {
      append(bv_non_endian::begin(that1), bv_non_endian::dsize(that1));
      int result = append(bv_non_endian::begin(that2), bv_non_endian::dsize(that2));

      _debug();
      return result;
    }

    template <typename T1, typename T2, typename T3> int appendT(T1 that1, T2 that2, T3 & that3)
    {
      append(bv_non_endian::begin(that1), bv_non_endian::dsize(that1));
      append(bv_non_endian::begin(that2), bv_non_endian::dsize(that2));
      int result = append(bv_non_endian::begin(that3), bv_non_endian::dsize(that3));

      _debug();
      return result;
    }

    int append_str_sans_null(char const * sz) {
      int result = append((byte const *)sz, strlen(sz));

      _debug();
      return result;
    }

    int append_strs_sans_null(char const * sz) {
      int result = append((byte const *)sz, strlen(sz));

      _debug();
      return result;
    }

    int append_strs_sans_null(char const * sz1, char const * sz2) {
      append((byte const *)sz1, strlen(sz1));
      int result = append((byte const *)sz2, strlen(sz2));

      _debug();
      return result;
    }

    int append_strs_sans_null(char const * sz1, char const * sz2, char const * sz3) {
      append((byte const *)sz1, strlen(sz1));
      append((byte const *)sz2, strlen(sz2));
      int result = append((byte const *)sz3, strlen(sz3));

      _debug();
      return result;
    }

    int append_strs_sans_null(char const * sz1, char const * sz2, char const * sz3, char const * sz4) {
      append((byte const *)sz1, strlen(sz1));
      append((byte const *)sz2, strlen(sz2));
      append((byte const *)sz3, strlen(sz3));
      int result = append((byte const *)sz4, strlen(sz4));

      _debug();
      return result;
    }

    int append_strs_sans_null(char const * sz1, char const * sz2, char const * sz3, char const * sz4, char const * sz5) {
      append((byte const *)sz1, strlen(sz1));
      append((byte const *)sz2, strlen(sz2));
      append((byte const *)sz3, strlen(sz3));
      append((byte const *)sz4, strlen(sz4));
      int result = append((byte const *)sz5, strlen(sz5));

      _debug();
      return result;
    }

    int append(byte x) {
      int result = append(&x, sizeof(x));

      _debug();
      return result;
    }

    int append(uint16 x) {
      uint16 y = htons(x);
      int result = append((byte const *)&y, sizeof(y));

      _debug();
      return result;
    }

    int append(uint32 x) {
      uint32 y = htonl(x);
      int result = append((byte const *)&y, sizeof(y));

      _debug();
      return result;
    }

    int append(buffer_view_i const & that) {
      int result = append(that.const_begin(), that.dsize());

      _debug();
      return result;
    }

    int append(byte const * p, size_t num) {
      int result = (int)data_length;
      if (p == NULL || num == 0) { return result; }

      resize(data_length + num);
      memcpy(bytes + data_length, p, num);

      data_length += num;
      _debug();
      return result;
    }

    buffer_tt & lshift(byte * start, byte const * new_start) {

      if (start == NULL) {
        start = begin();
      }

      if (start >= new_start) { return *this; }

      size_t count = const_end() - new_start;
      ::memcpy(start, new_start, count);

      byte * dead = start + count;
      size_t num_removed = new_start - start;
      ::memset(dead, 0, num_removed);

      data_length -= num_removed;

      _debug();
      return *this;
    }

    buffer_tt & lshift(byte * start, int new_start_offset) {
      lshift(start, const_begin() + new_start_offset);

      _debug();
      return *this;
    }

    virtual byte * begin() {
      return bytes;
    }

    virtual byte * end() {
      return bytes + data_length;
    }

    virtual byte * const_begin() const {
      return bytes;
    }

    virtual byte * const_end() const {
      return bytes + data_length;
    }

    virtual size_t dsize() const {
      return data_length;
    }

    size_t remaining_after(size_t num_consumed) {
      return data_length - num_consumed;
    }

    void set(int offset, byte b) {
      *(bytes + offset) = b;
    }

    void set(int offset, uint16 s) {
      *(uint16 *)(bytes + offset) = htons(s);
    }

    void set(int offset, uint32 l) {
      *(uint32 *)(bytes + offset) = htonl(l);
    }

#if 0
    byte at(int offset) const {
      return *(bytes + offset);
    }

    uint16 uint16_at(int offset) const {
      return ntohs(*(uint16 *)(bytes + offset));
    }

    uint32 uint32_at(int offset) const {
      return ntohl(*(uint32 *)(bytes + offset));
    }
#endif

    void dumpXml(char const * msg = NULL, size_t offset = 0) const
    {
      if (msg) {
        log_v(msg);
      }
      log_v((char*)(bytes + offset));
    }

  };
  typedef buffer_tt<unsigned char> buffer_t;

  // A view into a buffer-ish object
  //
  //      !!!   You cannot add a new type if that type has byte-ordering issues   !!!
  //
  namespace bv_non_endian {
    byte const * begin(buffer_t const & buffer);
    byte const *   end(buffer_t const & buffer);
    size_t       dsize(buffer_t const & buffer);
  };

  struct buffer_view_t : public buffer_view_i
  {
    virtual byte *   const_begin() const;
    virtual byte *     const_end() const;
    virtual size_t         dsize() const;

    virtual byte *         begin();
    virtual byte *           end();

  /*protected:*/
    buffer_view_t();
    buffer_view_t(byte * begin, byte * end);
    buffer_view_t(byte const * begin, byte const * end);
    byte * _begin, *_end;
  };

  struct buffer_range_view_t : public buffer_view_t {
    buffer_range_view_t();
    buffer_range_view_t(byte * begin, byte * end);
    buffer_range_view_t(byte const * begin, byte const * end);
    buffer_range_view_t & operator=(buffer_range_view_t const &);
  };

  struct chunk_t
  {
    buffer_t *           data;
    buffer_range_view_t  view;
    chunk_t(buffer_t * data, buffer_view_i const & view);
    ~chunk_t();
  };
  typedef std::deque<chunk_t *> chunks_t;
  string join(chunks_t const & chunks, char const * sep);
  void push(chunks_t & chunks, buffer_t * data, buffer_view_i const & view);

  namespace sock_opt {
    enum e_option {
      // Socket-level
      so_level_first = 1,
      so_broadcast,
      so_keepalive,
      so_level_last,

      // IP level
      ip_level_first,
      ip_multicast_if,
      ip_add_membership,
      ip_ttl,
      ip_multicast_ttl,
      ip_level_last
    };

    int level(e_option optname);
    int opt_name(e_option optname);

    char *       opt_val(e_option optname, void * poptval, int & intoptval, u_char & uchoptval);
    socklen_t    opt_len(e_option optname, socklen_t optlen);
  };
  using namespace sock_opt;

  // Get an IP from a server name
  extern string lookup_ip(char const * name);
  extern bool   is_ip_addr(char const * str);

  template <typename T>
  struct network_node_tt {

    struct sockaddr_in  addr;
    int                 tcp_fd;
    int                 udp_fd;

    int                 last_error;

    // Read-only
    string              ip;
    uint16              port;

    bool                raw;

    network_node_tt()
      : tcp_fd(0), udp_fd(0), last_error(0), port(0), raw(false)
    {
      memset(&addr, 0, sizeof(addr));
    }

    network_node_tt(char const * name, uint16 port)
      : tcp_fd(0), udp_fd(0), last_error(0), ip(name), port(port), raw(false)
    {
      _init(port);

      if (!is_ip_addr(ip.c_str())) {
        ip = lookup_ip(ip.c_str());
      }

      inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    }

    network_node_tt(string const & name, uint16 port)
      : tcp_fd(0), udp_fd(0), last_error(0), ip(name), port(port), raw(false)
    {
      _init(port);

      if (!is_ip_addr(ip.c_str())) {
        ip = lookup_ip(ip.c_str());
      }

      inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    }

    void _init(uint16 port) {
      memset(&addr, 0, sizeof(addr));

      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);

      short_timeout.tv_sec = 0;
      short_timeout.tv_usec = 20000;

      long_timeout.tv_sec = 0;
      long_timeout.tv_usec = 400000;

      zero_timeout.tv_sec = 0;
      zero_timeout.tv_usec = 0;

    }

    network_node_tt & operator=(struct sockaddr_in & that_addr) {
      addr = that_addr;

      char ip_[INET_ADDRSTRLEN + 1];
      ip = string(inet_ntop(AF_INET, &addr.sin_addr, ip_, INET_ADDRSTRLEN));

      port = ntohs(addr.sin_port);

      tcp_fd = udp_fd = 0;

      return *this;
    }

    int the_fd() {
      if (udp_fd != 0) { return udp_fd; }
      return tcp_fd;
    }

    int udp() {
      if (udp_fd != 0) { return udp_fd; }

      if (!raw) {
        struct sockaddr_in myaddr;
        memset(&myaddr, 0, sizeof(myaddr));
        myaddr.sin_family = AF_INET;
        myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        myaddr.sin_port = htons(0);

        if ((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
          last_error = get_last_network_error();
          return 0;
        }

        int bind_result = 0;
        if ((bind_result = bind(udp_fd, (struct sockaddr *)&myaddr, sizeof(myaddr))) < 0) {
          mwp_socket_close(udp_fd);
          //int x = errno;
          //uint32 y = GetLastError();
          last_error = get_last_network_error();
          return 0;
        }
//      } else {
//
//        if ((udp_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0) {
//          last_error = get_last_network_error();
//          return 0;
//        }

      }

      return udp_fd;
    }

    static char * format_mac(char * temp, byte * p) {
      snprintf(temp, 32, "%02x:%02x:%02x:%02x:%02x:%02x", *p++, *p++, *p++, *p++, *p++, *p++);
      return temp;
    }

    void report_mac(char const * ip_, char const * mac) {

      // 00:00:00:00:00:00 doesnt count
      if (string(mac) == "00:00:00:00:00:00") { return; }

      /* otherwise */
      //log_d(1, "", "MAC for %s : %s", ip_, mac);
      mac_addresses.insert(std::make_pair(ip_, mac));
      new_mac_addresses.insert(std::make_pair(ip_, mac));
    }

    void get_mac(struct sockaddr_in & addr) {

#if HAVE_SIOCGARP
      char temp[32];
      char ip_[INET_ADDRSTRLEN + 1];
      inet_ntop(AF_INET, &addr.sin_addr, ip_, INET_ADDRSTRLEN);

      // Dont go through the effort if we already have the mac
      if (mac_addresses.find(ip_) != mac_addresses.end()) { return; }

      uint16 that_port = ntohs(addr.sin_port);

      struct arpreq req;
      memset(&req, 0, sizeof(req));

      ((struct sockaddr_in*)(&req.arp_pa))->sin_family = AF_INET;
      ((struct sockaddr_in*)(&req.arp_ha))->sin_family = ARPHRD_ETHER;

      memcpy((struct sockaddr_in*)(&req.arp_pa), &addr, sizeof(struct sockaddr_in));

      strcpy(req.arp_dev, "eth0");

      if (ioctl(the_fd(), SIOCGARP, (caddr_t)&req) >= 0) {
        report_mac(ip_, format_mac(temp, (byte*)req.arp_ha.sa_data));
      } else {
        strcpy(req.arp_dev, "wlan0");

        if (ioctl(the_fd(), SIOCGARP, (caddr_t)&req) >= 0) {
          report_mac(ip_, format_mac(temp, (byte*)req.arp_ha.sa_data));
        //} else {
        //  log_d(1, "", "ioctl for ARP failed for %s:%d", ip_, that_port);
        }
      }
#endif
    }

    int set_sockopt(sock_opt::e_option optname, void * poptval = NULL, socklen_t optlen = 0, int intoptval = -999, u_char uchoptval = 99) {
      char * pvalue = opt_val(optname, poptval, intoptval, uchoptval);
      int result = setsockopt(the_fd(), level(optname), opt_name(optname), pvalue, opt_len(optname, optlen));
      //printf("setsockopt: %d -- %d %d %d %d %d %d %d\n", result, optname, level(optname), pvalue[0], pvalue[1], pvalue[2], pvalue[3], opt_len(optname, optlen));
      if (result < 0) {
        log_e(0, "socket", "Error setsockopt: %d -- %d\n", result, optname);
        return result;
      }

      /* otherwise -- read the value back */
      socklen_t len = opt_len(optname, optlen);
      /*int read_result =*/ getsockopt(the_fd(), level(optname), opt_name(optname), pvalue, &len);
      return result;
    }

    int set_sockopt(sock_opt::e_option optname, int intoptval) {
      return set_sockopt(optname, NULL, 0, intoptval);
    }

    int set_sockopt(sock_opt::e_option optname, u_char optval) {
      return set_sockopt(optname, NULL, 0, 0, optval);
    }

    int connect(int timeout_ms = -1) {
      if (tcp_fd != 0) { return tcp_fd; }

      struct sockaddr_in myaddr;
      memset(&myaddr, 0, sizeof(myaddr));
      myaddr.sin_family = AF_INET;
      myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
      myaddr.sin_port = htons(0);

      if ((tcp_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        last_error = get_last_network_error();
        return (tcp_fd = 0);
      }

      if (bind(tcp_fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        last_error = get_last_network_error();
        mwp_socket_close(tcp_fd);
        return (tcp_fd = 0);
      }

      // If there's no timeout
      if (timeout_ms == -1) {
        return _connect();
      }

      /* otherwise */
      return _connect(timeout_ms);
    }

    // Taken from http://developerweb.net/viewtopic.php?id=3196
    int _connect(int timeout_ms) {

      int res = 0, valopt = 0, num_selected = 0;
      struct timeval tv = {0};
      fd_set myfdset;
      socklen_t lon = sizeof(int);

      set_socket_non_blocking(tcp_fd);

      bool success = false;
      if ((res = ::connect(tcp_fd, (struct sockaddr *)&addr, sizeof(addr))) < 0) {
        last_error = get_last_network_error();
        if (connect_in_progress()) {
          tv.tv_sec = timeout_ms / 1000;
          tv.tv_usec = (timeout_ms % 1000) * 1000;
          FD_ZERO(&myfdset);
          FD_SET(tcp_fd, &myfdset);
          if ((num_selected = select(tcp_fd + 1, NULL, &myfdset, NULL, &tv)) > 0) {
            getsockopt(tcp_fd, SOL_SOCKET, SO_ERROR, (char*)(&valopt), &lon);
            success = !valopt;
          }
        }
      }

      if (!success) {
        mwp_socket_close(tcp_fd);
        return (tcp_fd = 0);
      }

      /* otherwise */
      set_socket_blocking(tcp_fd);

      return tcp_fd;
    }

    int _connect() {

      if (::connect(tcp_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        last_error = get_last_network_error();
        mwp_socket_close(tcp_fd);
        return (tcp_fd = 0);
      }

      return tcp_fd;
    }

    int close() {
      if (tcp_fd != 0) {
        return mwp_socket_close(tcp_fd);
      }

      if (udp_fd != 0) {
        return mwp_socket_close(udp_fd);
      }

      return 0;
    }

    static bool _act_like_unreliable_udp(char mod) {
#if 1
      return false;
#else
      if (mod == 'X') {
        return false;
      }

      srand(get_tick_count());
      bool result = (rand() % 100) > 10;

      if (result) {
        printf("^o%co^", mod);
      }

      return result;
#endif
    }

    int send_udp_to(buffer_t const & packet, char mod = 'X') {
      if (_act_like_unreliable_udp(mod)) { return (int)packet.data_length; }

      socklen_t	addrlen = sizeof(addr);
      int result = (int)sendto(udp_fd, (char*)packet.bytes, packet.data_length, 0, (struct sockaddr*)&addr, addrlen);
      last_error = get_last_network_error();

      return result;
    }

    int send_udp_to(buffer_t const & packet, string const & ip_, uint16 port, char mod = 'X')
    {
      string ip(ip_);
      struct sockaddr_in local_addr;
      socklen_t	addrlen = sizeof(local_addr);

      memset(&local_addr, 0, addrlen);

      local_addr.sin_family = AF_INET;
      local_addr.sin_port = htons(port);

      if (!is_ip_addr(ip.c_str())) {
        ip = lookup_ip(ip.c_str());
      }

      inet_pton(AF_INET, ip.c_str(), &local_addr.sin_addr);

      if (_act_like_unreliable_udp(mod)) { return (int)packet.data_length; }
      int result = (int)sendto(udp_fd, (char*)packet.bytes, packet.data_length, 0, (struct sockaddr*)&local_addr, addrlen);
      last_error = get_last_network_error();

      return result;
    }

    int recv_udp(buffer_t & recv_result, int hint_size = -1) {
      struct sockaddr_in  addr;

      memset(&addr, 0, sizeof(addr));

      if (hint_size > 0) {
        recv_result.resize(hint_size);
      }

      int result = (recv_result.data_length = recv(udp_fd, (char*)recv_result.bytes, recv_result.mem_length, 0));
      last_error = get_last_network_error();

      return result;
    }

    int recv_udp_from(buffer_t & recv_result, network_node_tt<T> & sender, int hint_size = -1) {
      struct sockaddr_in  addr;
      socklen_t	addrlen = sizeof(addr);

      memset(&addr, 0, sizeof(addr));

      int orig_size = (int)recv_result.data_length;

      if (hint_size == -1) {
        hint_size = 2 * 1024;
      }
      recv_result.resize(orig_size + hint_size);

      int    result     = 0;
      int    buf_size   = recv_result.mem_length - orig_size;
      char * buffer     = (char*)recv_result.bytes + orig_size;

      recv_result.data_length += (result = (int)recvfrom(udp_fd, buffer, buf_size, 0, (struct sockaddr*)&addr, &addrlen));
      last_error = get_last_network_error();

      sender = addr;

      if (raw) {
        log_d(1, "", "NumRead: %d", result);
        byte * p = (byte*)buffer;
        log_d(1, "", "Dest MAC: %02x:%02x:%02x:%02x:%02x:%02x", *p++, *p++, *p++, *p++, *p++, *p++);
        log_d(1, "", "Src  MAC: %02x:%02x:%02x:%02x:%02x:%02x", *p++, *p++, *p++, *p++, *p++, *p++);

        log_d(1, "", "len/type: %02x %02x", *p++, *p++);

        p += 12;
        log_d(1, "", "Dest IP : %d.%d.%d.%d", *p++, *p++, *p++, *p++);
        log_d(1, "", "Src  IP : %d.%d.%d.%d", *p++, *p++, *p++, *p++);

        log_d(1, "", "Sender: %s:%d", sender.ip.c_str(), sender.port);

        memcpy(buffer, buffer + 14 + 24, buf_size - (14 + 24));
      }

      get_mac(addr);

      return result;
    }

    static bool _act_like_tiny_tcp(char mod) {
#if 1
      return false;
#else
      if (mod != 's') {
        return false;
      }

      srand(get_tick_count());
      bool result = (rand() % 100) > 10;

      if (result) {
        printf("^o%co^", mod);
      }

      return result;
#endif
    }

    int send_tcp(buffer_view_i const & packet, char mod) {
      char const * begin = (char*)packet.const_begin();

      int len = (int)packet.dsize();
      if (_act_like_tiny_tcp(mod)) {
        len = min(len, 1024);
      }

      int result = (int)send(tcp_fd, begin, len, 0);
      last_error = get_last_network_error();

      return result;
    }

    int recv_tcp(buffer_t & recv_result, int hint_size) {
      struct sockaddr_in  addr;
      memset(&addr, 0, sizeof(addr));

      int orig_size = (int)recv_result.data_length;

      if (hint_size == -1) {
        hint_size = 2 * 1024;
      }
      recv_result.resize(orig_size + hint_size);

      int result = 0;
      recv_result.data_length += (result = (int)recv(tcp_fd, (char*)recv_result.bytes + orig_size, recv_result.mem_length - orig_size, 0));
      last_error = get_last_network_error();

      return result;
    }

    int recv_tcp(buffer_t & recv_result) {
      struct sockaddr_in  addr;
      memset(&addr, 0, sizeof(addr));

      int result = 0;
      recv_result.data_length += (result = (int)recv(tcp_fd, (char*)recv_result.bytes, recv_result.mem_length, 0));
      last_error = get_last_network_error();

      return result;
    }

    bool readable(int fd, struct timeval * timeout = NULL) {

      int selected = 0;

      if (timeout == NULL) {
        timeout = &zero_timeout;
      }

      fd_set readable, exceptional;
      FD_ZERO(&readable);
      FD_ZERO(&exceptional);

      FD_SET(fd, &readable);
      FD_SET(fd, &exceptional);

      if ((selected = select(fd + 1, &readable, NULL, &exceptional, timeout)) == 0) {
        last_error = get_last_network_error();
        return false;
      }

      last_error = get_last_network_error();
      return !!FD_ISSET(fd, &readable);
    }

    bool writeable(int fd, struct timeval * timeout = NULL) {

      int selected = 0;

      if (timeout == NULL) {
        timeout = &zero_timeout;
      }

      fd_set writeable, exceptional;
      FD_ZERO(&writeable);
      FD_ZERO(&exceptional);

      FD_SET(fd, &writeable);
      FD_SET(fd, &exceptional);

      if ((selected = select(fd + 1, NULL, &writeable, &exceptional, timeout)) == 0) {
        last_error = get_last_network_error();
        return false;
      }

      last_error = get_last_network_error();
      return !!FD_ISSET(fd, &writeable);
    }

  protected:
    struct timeval      short_timeout;
    struct timeval      long_timeout;
    struct timeval      zero_timeout;
  };
  typedef network_node_tt<unsigned char> network_node_t;

  // Put requests in a queue
  typedef std::deque<buffer_t *> request_queue_t;

  struct buffer_to_dest_t {
    buffer_t const * buffer;
    string ip;
    int port;

    buffer_to_dest_t(buffer_t const * buffer, string const & ip, int port);
    buffer_to_dest_t(buffer_t const & buffer, string const & ip, int port);   // Copies the buffer

    ~buffer_to_dest_t();

    /* private */
    bool own_buffer;
  };
  typedef std::deque<buffer_to_dest_t *> request_queue_with_dest_t;

  extern strlist compact(strlist const & that);

  // Some literal-like functions
  template <typename T>
  std::deque<T> A(T a, T b) {
    std::deque<T> array;
    array.push_back(a);
    array.push_back(b);
    return compact(array);
  }

  template <typename T1>
  std::map<std::string, T1> D(char const *key1, T1 const & val1) {
    std::map<std::string, T1> result;

    result.insert(std::make_pair(key1, val1));

    return result;
  }

  template <typename T1, typename T2>
  std::map<std::string, T1> D(char const *key1, T1 const & val1, char const *key2, T2 const & val2) {
    std::map<std::string, T1> result;

    result.insert(std::make_pair(key1, val1));
    result.insert(std::make_pair(key2, val2));

    return result;
  }

  template <typename T1, typename T2, typename T3>
  std::map<std::string, T1> D(char const *key1, T1 const & val1, char const *key2, T2 const & val2, char const *key3, T3 const & val3) {
    std::map<std::string, T1> result;

    result.insert(std::make_pair(key1, val1));
    result.insert(std::make_pair(key2, val2));
    result.insert(std::make_pair(key3, val3));

    return result;
  }

  struct json_t
  {
    strmap                    str_attrs;
    std::map<string, int> *   int_attrs;
    std::map<string, bool> *  bool_attrs;

    json_t();
    json_t(bool fromParsing);
    json_t(json_t const & that);
    ~json_t();

    json_t & operator=(json_t const & that);

    json_t &        _init(bool fromParsing);

    json_t &       insert(string const & key, string const & value);
    json_t &       insert(string const & key, char const * value);
    json_t &       insert(string const & key, int value);
    json_t &       insert(string const & key, bool value);

    json_t &       insert(char const * key, string const & value);
    json_t &       insert(char const * key, char const * value);
    json_t &       insert(char const * key, int value);
    json_t &       insert(char const * key, bool value);

    bool              has(char const * key) const;
    bool       has_string(char const * key) const;
    bool          has_int(char const * key) const;
    bool         has_bool(char const * key) const;

    bool              has(string const & key) const;
    bool       has_string(string const & key) const;
    bool          has_int(string const & key) const;
    bool         has_bool(string const & key) const;

    string const & lookup(char const * key) const;
    string const & lookup(string const & key) const;

    int            lookup(char const * key, int def) const;
    int            lookup(string const & key, int def) const;
    int        lookup_int(char const * key) const;
    int        lookup_int(string const & key) const;

    bool           lookup(char const * key, bool def) const;
    bool           lookup(string const & key, bool def) const;
    bool      lookup_bool(char const * key) const;
    bool      lookup_bool(string const & key) const;

    string      stringify() const;

    json_t const &   dump(bool force = false) const;

    /* private */
    static string the_null_string;
    json_t &        _copy(json_t const & that);
  };

  struct json_array_t
  {
    std::map<int, json_t*> arr;

    json_array_t();
    ~json_array_t();

    bool            has(int n) const;
    json_t const *  get(int n) const;

    int             insert(string const & key, string const & value);
    int             insert(string const & key, char const * value);
    int             insert(string const & key, int value);
    int             insert(string const & key, bool value);

    int             insert(char const * key, string const & value);
    int             insert(char const * key, char const * value);
    int             insert(char const * key, int value);
    int             insert(char const * key, bool value);

    string          stringify() const;

    void            log_v_(int level, char const * mod, char const * pre) const;
  };

  struct serialization_json_t
  {
    static int string_type;
    static int int_type;
    static int bool_type;
    static int real_type;

    struct serialization_json_elt_t
    {
      string  value;
      //string  key;
      int     type;

      serialization_json_elt_t(string const &);
      serialization_json_elt_t(char const *);
      serialization_json_elt_t(int);
      serialization_json_elt_t(bool);
      serialization_json_elt_t(float);
      serialization_json_elt_t(double);

      string stringify();
    };

    typedef std::map<string, serialization_json_elt_t *> elements_t;
    typedef std::map<string, serialization_json_t *>     sub_elements_t;

    elements_t       elements;
    sub_elements_t   sub_elements;

    serialization_json_t();
    serialization_json_t(serialization_json_t const & that);

    ~serialization_json_t();

    serialization_json_t & getObject(string const & key_);
    string stringify() const;


    template <typename T>
    serialization_json_t & set(string const & key_, T const & value) {
      string       parent_key;
      string       key        = key_;

      // Is this a composite key?
      if (_normalize_keys(parent_key, key)) {
        return _set(parent_key, key, value);
      }

      // Free any memory we may already have been using
      if (_has(elements, key)) {
        delete elements[key]; /**/ num_allocations -= 1;
      }

      elements[key] = new serialization_json_elt_t(value); /**/ num_allocations += 1;
      return *this;
    }

    template <typename T>
    serialization_json_t & set(std::map<string, T> dict) {
      for (typename std::map<string, T>::const_iterator it = dict.begin(); it != dict.end(); ++it) {
        set(it->first, it->second);
      }

      return *this;
    }

    template <typename T>
    serialization_json_t & _set(string const & parent_key_, string const & key_, T const & value) {
      string parent_key    = parent_key_;
      string key           = key_;

      if (_normalize_keys(parent_key, key)) {
        if (!_has(sub_elements, parent_key)) {
          sub_elements[parent_key] = new serialization_json_t(); /**/ num_allocations += 1;
        }

        return sub_elements[parent_key]->set(key, value);
      }

      /* otherwise -- This wasn't really a composite key */
      return set(key, value);
    }

    // Returns true if the input was a composite key (so parent and key are both filled)
    // Returns false if the input was a non-compoiste key (had no '.'); only key is filled; parent is ""
    bool _normalize_keys(string & parent, string & key);



    template <typename T>
    bool _has(std::map<string, T> const & dict, string const & key) {
      return dict.find(key) != dict.end();
    }

    // This is not a general-purpose class
    private:
      serialization_json_t & operator=(serialization_json_t const &);
  };

#if 1
  template <typename T>
  vector<string> split_v(string const & str, T sep)
  {
    return split(str.c_str(), sep);
  }
#endif

  template <typename T>
  T const * _find(T const * str, T ch)
  {
    T const * result = str;
    while (*result != 0 && *result != ch) {
      result += 1;
    }
    return result;
  }

#if 1
  template <typename T>
  vector<string> split_v(T const * str, T sep)
  {
    vector<string> result;
    if (!*str) { return result; }

    T const * p1 = NULL, *p2 = str - 1;

    do {
      p1 = p2 + 1;
      p2 = _find(p1, sep);
      result.push_back(string(p1, p2 - p1));

    } while (*p2);

    return result;
  }
#endif

  template <typename T>
  int mwp_set_union(set<T> const & a, set<T> const & b, set<T> & result) {

    if (&a != &result) {
      for (typename set<T>::const_iterator it = a.begin(); it != a.end(); ++it) {
        result.insert(*it);
      }
    }

    if (&b != &result) {
      for (typename set<T>::const_iterator it = b.begin(); it != b.end(); ++it) {
        result.insert(*it);
      }
    }

    return (int)result.size();
  }

  template <typename T>
  set<T> mwp_set_union(set<T> const & a, set<T> const & b) {
    set<T> result;
    mwp_set_union(a, b, result);
    return result;
  }

};


// ---------------------------------------------------------------------------
// Example of hosting application-provided functions
#if 0

#ifdef WIN32

bool g_verbose = false;

int _tmain(int argc, _TCHAR const * argv[])
{
  args_t ARGV(argc, argv);
  g_verbose = ARGV.get_flag("verbose");

  core_api_t mario;

  mario.start();
}

void net_mobilewebprint::log_d(char const * msg, log_param_t)
{
  cout << msg;
}

void net_mobilewebprint::log_v(char const * msg, log_param_t)
{
  if (g_verbose) {
    net_mobilewebprint::log_d(msg, (log_param_t)NULL);
  }
}

void net_mobilewebprint::log_e(char const * msg, log_param_t)
{
  net_mobilewebprint::log_d(msg);
}

string net_mobilewebprint::platform_to_ascii_string(void const *str_)
{
  _TCHAR * str = (_TCHAR*)str_;

  int needed = WideCharToMultiByte(CP_ACP, 0, str, _tcslen(str), NULL, 0, NULL, NULL);
  char * buffer = new char[needed + 4];
  WideCharToMultiByte(CP_ACP, 0, str, -1, buffer, needed + 1, NULL, NULL);

  string result(buffer);
  delete[] buffer;

  return result;
}

char const win_lock[] = "windows_lock";
bool net_mobilewebprint::allocate_host_lock(char const * name, void ** result_lock_out)
{
  *result_lock_out = (void*)win_lock;
  return true;
}

bool net_mobilewebprint::host_lock(void * lock)
{
  return true;
}

bool net_mobilewebprint::host_unlock(void * lock)
{
  return true;
}

void net_mobilewebprint::free_host_lock(void * lock)
{
}

bool net_mobilewebprint::start_thread(void*(*pfn)(void*data), void*data)
{
  return (_beginthread((void(*)(void*))pfn, 0, data) != -1L);
}

bool net_mobilewebprint::interruptable_sleep(int msec)
{
  Sleep(msec);
  return true;
}

uint32 net_mobilewebprint::get_tick_count()
{
  return GetTickCount();
}

void * net_mobilewebprint::assert(void * x)
{
  return x;
}

int    net_mobilewebprint::assert(int x)
{
  return x;
}

bool   net_mobilewebprint::assert(bool x)
{
  return x;
}
#endif

#endif


#endif  // __MWP_TYPES_HPP__

