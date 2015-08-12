
#include "mwp_buffer.hpp"
#include "mwp_assert.hpp"
#include "mwp_types.hpp"
#include <cstdio>

using net_mobilewebprint::buffer_range_t;
using net_mobilewebprint::buffer_view_t;
using net_mobilewebprint::byte;
using net_mobilewebprint::uint16;
using net_mobilewebprint::uint32;

using std::string;

int net_mobilewebprint::num_allocations               = 0;
int net_mobilewebprint::num_buffer_allocations        = 0;
int net_mobilewebprint::num_buf_bytes_allocations     = 0;
int net_mobilewebprint::num_buf_bytes                 = 0;

buffer_range_t::buffer_range_t(byte const * begin, byte const * end)
  : begin_(begin), end_(end)
{
}

//buffer_range_t::buffer_range_t(byte const * begin, size_t length)
//  : begin_(begin), end(begin + length)
//{
//}

buffer_range_t::buffer_range_t()
  : begin_(NULL), end_(NULL)
{
}

buffer_range_t::buffer_range_t(buffer_view_t const & view)
  : begin_(view.begin()), end_(view.end())
{
}

byte const * buffer_range_t::buffer_range_t::begin() const
{
  return begin_;
}

byte const * buffer_range_t::buffer_range_t::end() const
{
  return end_;
}

size_t buffer_range_t::buffer_range_t::dsize() const
{
  return end_ - begin_;
}

buffer_range_t net_mobilewebprint::mk_buffer_range(buffer_reader_t const & reader)
{
  return buffer_range_t(reader.p, reader.buffer.end());
}

//struct buffer_view_t
//{
//  typedef byte const * iterator;
//
//  virtual byte const *       begin() const = 0;
//  virtual byte const *         end() const = 0;
//  virtual size_t             dsize() const = 0;

//byte net_mobilewebprint::buffer_view_t::at(int byte_offset, bool & valid) const
//{
//  iterator p = begin() + byte_offset;
//  if (!(valid = mwp_assert(is_valid(p)))) {
//    return 0;
//  }
//
//  return *p;
//}
//
//uint16 net_mobilewebprint::buffer_view_t::uint16_at(int byte_offset, bool & valid) const
//{
//  return 0;
//}
//
//uint32 net_mobilewebprint::buffer_view_t::uint32_at(int byte_offset, bool & valid) const
//{
//  return 0;
//}


buffer_view_t::iterator net_mobilewebprint::buffer_view_t::first() const
{
  return begin();
}

bool net_mobilewebprint::buffer_view_t::is_valid(iterator const & it) const
{
  return (begin() <= it) && (it < end());
}

bool net_mobilewebprint::buffer_view_t::is_valid(iterator const & it, size_t size) const
{
  return is_valid(it) && is_valid(it + (size - 1));
}


byte net_mobilewebprint::buffer_view_t::read_byte(iterator & it, bool & valid, bool no_assert) const
{
  byte * p = (byte*)it;
  valid    = is_valid(p);

  if (!no_assert) {
    mwp_assert(valid, "p out of range in read_byte");
  }

  if (!valid) {
    return 0;
  }

  it += sizeof(*p);
  return *p;
}

uint16 net_mobilewebprint::buffer_view_t::read_uint16(iterator & it, bool & valid, bool no_assert) const
{
  uint16 * p = (uint16*)it;
  valid      = is_valid((byte*)p, sizeof(*p));

  if (!no_assert) {
    mwp_assert(valid, "p out of range in read_uint16");
  }

  if (!valid) {
    return 0;
  }

  it += sizeof(*p);
  return ntohs(*p);
}

uint32 net_mobilewebprint::buffer_view_t::read_uint32(iterator & it, bool & valid, bool no_assert) const
{
  uint32 * p = (uint32*)it;
  valid      = is_valid((byte*)p, sizeof(*p));

  if (!no_assert) {
    mwp_assert(valid, "p out of range in read_uint32");
  }

  if (!valid) {
    return 0;
  }

  it += sizeof(*p);
  return ntohl(*p);
}

string  net_mobilewebprint::buffer_view_t::read_string(iterator & it, bool & valid, bool no_assert) const
{
  char const * start    = (char const *)it;
  char const * p        = start;
  char const * last     = (char const *)end();

  int len = 0;
  for (len = 0; p < last && *p != 0; ++p) {
    len += 1;
  }

  valid = (p < last);
  if (!no_assert) {
    mwp_assert(valid, "p out of range in read_string");
  }

  if (!valid) {
    return "";
  }

  it += len + 1;
  return string(start, len);
}

string  net_mobilewebprint::buffer_view_t::read_string_nz(iterator & it, size_t count, bool & valid, bool no_assert) const
{
  char const * start    = (char const *)it;

  valid = is_valid(it) && is_valid(it, count);
  if (!no_assert) {
    mwp_assert(valid, "p out of range in read_string_nz");
  }

  if (!valid) {
    return "";
  }

  it += count;
  return string(start, count);
}

string net_mobilewebprint::buffer_view_t::read_ip(iterator & it, bool & valid, bool no_assert) const
{
  byte const * start = it;

  valid = is_valid(it) && is_valid(it + 4);
  if (!no_assert) {
    mwp_assert(valid, "p out of range in read_ip");
  }

  if (!valid) {
    return "";
  }

  it += 4;
  return mwp_ntop(start);
}

net_mobilewebprint::buffer_reader_t::buffer_reader_t(buffer_view_t const & buffer_, bool no_assert_)
  : buffer(buffer_), valid(true), p(buffer_.first()), no_assert(no_assert_)
{
}

net_mobilewebprint::buffer_reader_t::buffer_reader_t(buffer_reader_t const & that)
  : buffer(that.buffer), valid(that.valid), p(that.p), no_assert(that.no_assert)
{
}

bool net_mobilewebprint::buffer_reader_t::is_valid()
{
  return valid;
}

void net_mobilewebprint::buffer_reader_t::seek(int count)
{
  p += count;
}

void net_mobilewebprint::buffer_reader_t::seek_to(size_t place)
{
  p = buffer.first() + place;
}

int net_mobilewebprint::buffer_reader_t::tell()
{
  return p - buffer.first();
}

byte net_mobilewebprint::buffer_reader_t::read_byte()
{
  byte result = buffer.read_byte(p, valid, no_assert);
  return result;
}

uint16 net_mobilewebprint::buffer_reader_t::read_uint16()
{
  uint16 result = buffer.read_uint16(p, valid, no_assert);
  return result;
}

uint32 net_mobilewebprint::buffer_reader_t::read_uint32()
{
  uint32 result = buffer.read_uint32(p, valid, no_assert);
  return result;
}

string net_mobilewebprint::buffer_reader_t::read_string()
{
  return buffer.read_string(p, valid, no_assert);
}

string net_mobilewebprint::buffer_reader_t::read_string_nz(size_t count)
{
  return buffer.read_string_nz(p, count, valid, no_assert);
}

string net_mobilewebprint::buffer_reader_t::read_ip()
{
  return buffer.read_ip(p, valid, no_assert);
}

bool net_mobilewebprint::buffer_reader_t::at_end()
{
  return p >= buffer.end();
}

size_t net_mobilewebprint::write_hton(byte * mem, uint16 sh)
{
  *(uint16*)(mem) = htons(sh);
  return sizeof(sh);
}

size_t net_mobilewebprint::write_hton(byte * mem, uint32 n)
{
  *(uint32*)(mem) = htons(n);
  return sizeof(n);
}

string net_mobilewebprint::mwp_ntop(byte const * p)
{
  char buffer[32];

  ::memset(buffer, 0, sizeof(buffer));

  mwp_snprintf(buffer, sizeof(buffer), "%d.%d.%d.%d", (int)(*p), (int)(*(p+1)), (int)(*(p+2)), (int)(*(p+3)));
  return buffer;
}

