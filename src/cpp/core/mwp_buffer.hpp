
#ifndef __MWP_BUFFER_HPP__
#define __MWP_BUFFER_HPP__

#ifdef WIN32
#include <Winsock2.h>
#include <Ws2tcpip.h>

#else
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#endif

#ifndef mwp_max
#define mwp_max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef mwp_min
#define mwp_min(a,b) (((a) < (b)) ? (a) : (b))
#endif


#include "mwp_types.hpp"
#include "mwp_utils.hpp"
#include <string>
#include <cstddef>
#include <cstdlib>
#include <cctype>
#include <cstring>

namespace net_mobilewebprint {

  using std::size_t;
  using std::string;
  using net_mobilewebprint::byte;
  using net_mobilewebprint::uint16;

  extern int    num_allocations;
  extern int    num_buffer_allocations;
  extern int    num_buf_bytes_allocations;
  extern int    num_buf_bytes;

  size_t   write_hton(byte * mem, byte   by);
  size_t   write_hton(byte * mem, uint16 sh);
  size_t   write_hton(byte * mem, uint32 n);
  string   mwp_ntop(byte const * p);

  struct buffer_view_t
  {
    typedef byte const * iterator;

    virtual byte const *       begin() const = 0;
    virtual byte const *         end() const = 0;
    virtual size_t             dsize() const = 0;

    virtual iterator           first() const;
    virtual bool            is_valid(iterator const & it) const;
    virtual bool            is_valid(iterator const & it, size_t) const;

    virtual byte           read_byte(iterator & it, bool & valid, bool no_assert = false) const;
    virtual uint16       read_uint16(iterator & it, bool & valid, bool no_assert = false) const;
    virtual uint32       read_uint32(iterator & it, bool & valid, bool no_assert = false) const;

    virtual string       read_string(iterator & it, bool & valid, bool no_assert = false) const;
    virtual string    read_string_nz(iterator & it, size_t count, bool & valid, bool no_assert = false) const;

    virtual string           read_ip(iterator & it, bool & valid, bool no_assert = false) const;
    virtual void                dump(char const * msg) const;
  };

  template <typename TT>
  struct buffer_tt : public buffer_view_t
  {
    size_t mem_length;
    size_t data_length;
    size_t grow_size;
    byte * bytes;

    buffer_tt()
      : mem_length(128), data_length(0), grow_size(128), bytes(NULL)
    {
      bytes = _fresh_bytes(mem_length);
    }

    buffer_tt(byte const * pby, size_t data_length_)
      : mem_length(data_length_), data_length(data_length_), grow_size(128), bytes(NULL)
    {
      bytes = _fresh_bytes(mem_length);
      ::memcpy(bytes, pby, data_length);
    }

    virtual ~buffer_tt()
    {
      delete bytes;
      bytes = NULL;
    }

    virtual byte const *       begin() const
    {
      return bytes;
    }

    virtual byte const *         end() const
    {
      return bytes + dsize();
    }

    virtual size_t             dsize() const
    {
      return data_length;
    }

    template <typename T>
    buffer_tt<TT> & append(T t)
    {
      resize_to(data_length + sizeof(t));
      data_length += write_hton(bytes + data_length, t);
      return *this;
    }

    void append(void * p, size_t length)
    {
      resize_by(length);

      memcpy(bytes + data_length, p, length);
      data_length += length;
    }

    void append_string_nz(char const * str)
    {
      int length = strlen(str);
      resize_by(length);

      memcpy(bytes + data_length, str, length);
      data_length += length;
    }

    void append_byte_and_string_nz(char const * str)
    {
      append((byte)strlen(str));
      append_string_nz(str);
    }

    void zero_pad(size_t length)
    {
      resize_by(length);

      memset(bytes + data_length, 0, length);
      data_length += length;
    }

    template <typename T>
    void set_at(size_t offset, T t)
    {
      write_hton(bytes + offset, t);
    }

    void dump(char const * msg)
    {
      mem_dump(bytes, mem_length, msg, data_length, -1, 8);
    }

    byte * _fresh_bytes(size_t num)
    {
      byte * result = new byte[num];
      memset(result, 0, num);
      return result;
    }

    byte * resize_by(int additional_bytes)
    {
      return resize_to(data_length + additional_bytes);
    }

    byte * resize_to(size_t new_mem_length)
    {
      size_t orig_mem_length = mem_length;
      if (new_mem_length <= mem_length) {
        // We alerady have the right size
        return bytes;
      }

      /* otherwise */
      size_t num_to_increase_by = mwp_max(new_mem_length - mem_length, grow_size);
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

  };
  typedef buffer_tt<byte> buffer_t;

  struct buffer_range_t : public buffer_view_t
  {
    byte const * begin_;
    byte const * end_;

    buffer_range_t(byte const * begin, byte const * end);
    buffer_range_t(byte const * begin, size_t length);
    buffer_range_t();
    buffer_range_t(buffer_view_t const &);

    virtual byte const *       begin() const;
    virtual byte const *         end() const;
    virtual size_t             dsize() const;
  };

  struct buffer_reader_t
  {
    buffer_range_t             buffer;
    bool                       valid;
    buffer_view_t::iterator    p;
    bool                       no_assert;

    buffer_reader_t(buffer_view_t const &, bool no_assert = false);
    buffer_reader_t(buffer_reader_t const &);

    virtual byte           read_byte();
    virtual uint16       read_uint16();
    virtual uint32       read_uint32();

    virtual string       read_string();
    virtual string    read_string_nz(size_t count);

    virtual string           read_ip();

    virtual void                seek(int count);
    virtual void             seek_to(size_t place);

    virtual int                 tell();

    virtual bool              at_end();

    virtual bool            is_valid();
  };

  buffer_range_t mk_buffer_range(buffer_reader_t const &);
};

#endif // __MWP_BUFFER_HPP__

