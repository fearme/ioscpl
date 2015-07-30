
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
//#include <string>
#include <cstddef>
#include <cstdlib>
#include <cctype>
#include <cstring>

namespace net_mobilewebprint {

  using std::size_t;
  //using std::string;
  using net_mobilewebprint::byte;
  using net_mobilewebprint::uint16;

  extern int    num_allocations;
  extern int    num_buffer_allocations;
  extern int    num_buf_bytes_allocations;
  extern int    num_buf_bytes;

  size_t write_hton(byte * mem, uint16 sh);
  size_t write_hton(byte * mem, uint32 n);

  struct buffer_view_t
  {
    typedef byte const * iterator;

    virtual byte const *       begin() const = 0;
    virtual byte const *         end() const = 0;
    virtual size_t             dsize() const = 0;

    byte                          at(int byte_offset) const;
    uint16                 uint16_at(int byte_offset) const;
    uint32                 uint32_at(int byte_offset) const;

    virtual iterator           first() const;
    virtual bool            is_valid(iterator const & it) const;

    virtual byte           read_byte(iterator & it) const;
    virtual uint16       read_uint16(iterator & it) const;
    virtual uint32       read_uint32(iterator & it) const;

//    virtual std::string       read_string(iterator & it) const;
//    virtual std::string    read_string_nz(iterator & it) const;
  };

  template <typename TT>
  struct buffer_tt
  {
    size_t mem_length;
    size_t data_length;
    size_t grow_size;
    byte * bytes;

    buffer_tt(byte   by)
      : mem_length(sizeof(by)), data_length(sizeof(by)), grow_size(128), bytes(NULL)
    {
      bytes = _fresh_bytes(mem_length);
      *bytes = by;
    }

    buffer_tt(uint16 sh)
      : mem_length(sizeof(sh)), data_length(sizeof(sh)), grow_size(128), bytes(NULL)
    {
      bytes = _fresh_bytes(mem_length);
      *(uint16*)(bytes) = htons(sh);
    }

    buffer_tt(uint32 n)
      : mem_length(sizeof(n)), data_length(sizeof(n)), grow_size(128), bytes(NULL)
    {
      bytes = _fresh_bytes(mem_length);
      *(uint32*)(bytes) = htons(n);
    }

    int size()
    {
      return data_length;
    }

    byte read_byte()
    {
      return (byte)*bytes;
    }

    template <typename T>
    buffer_tt<TT> & append(T t)
    {
      resize_to(data_length + sizeof(t));
      data_length += write_hton(bytes + data_length, t);
      return *this;
    }

    byte * _fresh_bytes(size_t num)
    {
      byte * result = new byte[num];
      memset(result, 0, num);
      return result;
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


};

#endif // __MWP_BUFFER_HPP__

