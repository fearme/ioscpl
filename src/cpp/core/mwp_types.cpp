
#include "mwp_types.hpp"
#include "mwp_controller.hpp"
#include "mwp_utils.hpp"
#include <string>
#include <cstdio>

using std::string;
using net_mobilewebprint::strlist;
using net_mobilewebprint::is_num;
using net_mobilewebprint::mwp_atoi;
using net_mobilewebprint::join;

int   net_mobilewebprint::num_allocations           = 0;
int   net_mobilewebprint::num_buffer_allocations    = 0;
int   net_mobilewebprint::num_buf_bytes_allocations = 0;
int   net_mobilewebprint::num_buf_bytes             = 0;
int   net_mobilewebprint::num_chunk_allocations     = 0;

net_mobilewebprint::strmap net_mobilewebprint::mac_addresses;
net_mobilewebprint::strmap net_mobilewebprint::new_mac_addresses;

net_mobilewebprint::buffer_view_t::buffer_view_t()
  : _begin(NULL), _end(NULL)
{
}

net_mobilewebprint::buffer_view_t::buffer_view_t(byte * begin, byte * end)
  : _begin(begin), _end(end)
{
}

net_mobilewebprint::buffer_view_t::buffer_view_t(byte const * begin, byte const * end)
  : _begin((byte*)begin), _end((byte*)end)
{
}

net_mobilewebprint::byte * net_mobilewebprint::buffer_view_t::begin()
{
  return _begin;
}

net_mobilewebprint::byte * net_mobilewebprint::buffer_view_t::end()
{
  return _end;
}

net_mobilewebprint::byte * net_mobilewebprint::buffer_view_t::const_begin() const
{
  return _begin;
}

net_mobilewebprint::byte * net_mobilewebprint::buffer_view_t::const_end() const
{
  return _end;
}

size_t net_mobilewebprint::buffer_view_t::dsize() const
{
  return _end - _begin;
}

net_mobilewebprint::byte net_mobilewebprint::buffer_view_i::at(int offset) const
{
  return *(const_begin() + offset);
}

uint16 net_mobilewebprint::buffer_view_i::uint16_at(int offset) const
{
  return ntohs(*(uint16 *)(const_begin() + offset));
}

uint32 net_mobilewebprint::buffer_view_i::uint32_at(int offset) const
{
  return ntohl(*(uint32 *)(const_begin() + offset));
}

//typedef byte * iterator;
net_mobilewebprint::buffer_view_i::const_iterator net_mobilewebprint::buffer_view_i::first() const
{
  return const_begin();
}

bool net_mobilewebprint::buffer_view_i::is_valid(const_iterator it) const
{
  return it < const_end();
}

net_mobilewebprint::byte net_mobilewebprint::buffer_view_i::read_byte(const_iterator & it) const
{
  mwp_assert(is_valid(it));

  return *it++;
}

uint16 net_mobilewebprint::buffer_view_i::read_uint16(const_iterator & it) const
{
  mwp_assert(is_valid(it));

  uint16 result = ntohs(*(uint16*)it);
  it += sizeof(uint16);
  return result;
}

uint32 net_mobilewebprint::buffer_view_i::read_uint32(const_iterator & it) const
{
  mwp_assert(is_valid(it));

  uint32 result = ntohl(*(uint32*)it);
  it += sizeof(uint32);
  return result;
}

std::string net_mobilewebprint::buffer_view_i::read_string_nz(const_iterator & it, size_t count) const
{
  mwp_assert(is_valid(it + count - 1));

  std::string result(it, it + count);

  it += count;

  return result;
}

std::string net_mobilewebprint::buffer_view_i::read_string(const_iterator & it) const
{
//  mwp_assert(is_valid(it + count - 1));

  std::string result((char const *)it);

  it += strlen((char const *)it) + 1;

  return result;
}

bool net_mobilewebprint::buffer_view_i::read_string_nz(const_iterator & it, size_t count, char const *& begin_, char const *& end_) const
{
  begin_ = (char const *)it;
  end_   = (char const *)min(it + count, const_end());

  if (!mwp_assert(is_valid((iterator)(end_ - 1)))) { return false; }

  it += count;

  return true;
}

std::string net_mobilewebprint::buffer_view_i::read_ip(const_iterator & it) const
{
  char buffer[32];
  std::string result;

  inet_ntop(AF_INET, (void*)it, buffer, sizeof(buffer));
  it += 4;
  result = buffer;
  return result;
}

// Use at your own risk - caller must know it is a string
std::string net_mobilewebprint::buffer_view_i::str() const {
  return string((char*)const_begin(), dsize());
}

std::string net_mobilewebprint::buffer_view_i::to_lower() const {
  string str_ = str();
  std::transform(str_.begin(), str_.end(), str_.begin(), (int(*)(int))std::tolower);
  return str_;
}

void net_mobilewebprint::buffer_view_i::dump(char const * mod_name, char const * msg, int limit) const
{
  if (mod_name != NULL && !get_flag("log_packet_data")) { return; }
  if (mod_name != NULL && !get_flag((string("log_") + mod_name))) { return; }

  if (limit == -1) {
    limit = 64 * 1024;
  }

  char buf[128], *phex = buf, *pch = phex + (3 * 16 + 1 + 3 + 2), sbuf[128];    // phex is where the hex is written to, pch is where the char is written
  char ch = 0;

  if (msg != NULL) {
    log_d("%s", msg);
  }

  log_d("%x dlen: %d", const_begin(), dsize());

  // Should we do anything?
  if (const_begin() == NULL)  { return; }
  if (dsize() == 0)           { log_d("=== Empty"); return; }

  size_t i;
  for (i = 0; i < limit && i < dsize(); ++i) {

    // Start of new line?
    if (i % 16 == 0) {
      if (i != 0) {
        *pch = 0;
        log_d("%s", buf);
      }

      memset(buf, ' ', sizeof(buf));
      phex = buf;

      mwp_sprintf(sbuf, "%8p: ", const_begin() + i);
      ::strcpy(phex, sbuf);
      phex += 12;
      *phex++ = ' ';

      pch = phex + (3 * 16 + 1 + 3 + 2);

    } else if (i % 8 == 0) {
      *phex++ = ' ';
      *phex++ = ' ';
      *phex = ' ';

      *pch++ = ' ';

    } else if (i % 4 == 0) {
      *phex++ = ' ';
      *phex = ' ';
    }

    mwp_sprintf(sbuf, "%02x ", *(const_begin() + i));
    ::strcpy(phex, sbuf);
    phex += 3;
    *phex = ' ';

    ch = *(char*)(const_begin() + i);
    if (ch < 32 || ch > 126) {
      ch = '.';
    }

    *pch++ = ch;
  }

  *pch = 0;
  log_d("%s", buf);

  if (i < dsize()) {
    char moreMsg[128];
    mwp_sprintf(moreMsg, " ----- %d MORE -----", (int)(dsize() - i));
    log_d(moreMsg);
  }
}

net_mobilewebprint::buffer_range_view_t::buffer_range_view_t()
  : buffer_view_t((byte const *)NULL, (byte const *)NULL)
{
}

net_mobilewebprint::buffer_range_view_t::buffer_range_view_t(byte * begin, byte * end)
  : buffer_view_t(begin, end)
{
}

net_mobilewebprint::buffer_range_view_t::buffer_range_view_t(byte const * begin, byte const * end)
  : buffer_view_t(begin, end)
{
}

net_mobilewebprint::buffer_range_view_t & net_mobilewebprint::buffer_range_view_t::operator=(net_mobilewebprint::buffer_range_view_t const & that)
{
  _begin = that._begin;
  _end = that._end;

  return *this;
}

net_mobilewebprint::byte const * net_mobilewebprint::bv_non_endian::begin(buffer_t const & buffer)
{
  return buffer.bytes;
}

net_mobilewebprint::byte const * net_mobilewebprint::bv_non_endian::end(buffer_t const & buffer)
{
  return buffer.const_end();
}

size_t net_mobilewebprint::bv_non_endian::dsize(buffer_t const & buffer)
{
  return buffer.data_length;
}

net_mobilewebprint::byte const * net_mobilewebprint::bv_non_endian::begin(buffer_view_i const & buffer)
{
  return buffer.const_begin();
}

net_mobilewebprint::byte const * net_mobilewebprint::bv_non_endian::end(buffer_view_i const & buffer)
{
  return buffer.const_end();
}

size_t net_mobilewebprint::bv_non_endian::dsize(buffer_view_i const & buffer)
{
  return buffer.dsize();
}

net_mobilewebprint::byte const * net_mobilewebprint::bv_non_endian::begin(string const & str)
{
  return (byte*)str.c_str();
}

net_mobilewebprint::byte const * net_mobilewebprint::bv_non_endian::end(string const & str)
{
  return (byte*)(str.c_str() + str.length() + 1);
}

size_t net_mobilewebprint::bv_non_endian::dsize(string const & str)
{
  return str.length() + 1;
}

net_mobilewebprint::byte const * net_mobilewebprint::bv_non_endian::begin(char const * str)
{
  return (byte*)str;
}

net_mobilewebprint::byte const * net_mobilewebprint::bv_non_endian::end(char const * str)
{
  return (byte*)(str + ::strlen(str) + 1);
}

size_t net_mobilewebprint::bv_non_endian::dsize(char const * str)
{
  return ::strlen(str) + 1;
}

net_mobilewebprint::byte const * net_mobilewebprint::bv_non_endian::begin(char str[MWP_STRLEN_BUFFER_STRING])
{
  return (byte*)str;
}

net_mobilewebprint::byte const * net_mobilewebprint::bv_non_endian::end(char str[MWP_STRLEN_BUFFER_STRING])
{
  return begin(str) + dsize(str);
}

size_t net_mobilewebprint::bv_non_endian::dsize(char str[MWP_STRLEN_BUFFER_STRING])
{
  return MWP_STRLEN_BUFFER_STRING;
}

net_mobilewebprint::byte const * net_mobilewebprint::bv_non_endian::begin(std::pair<char const *, char const *> const & str_ish)
{
  return (byte const*)str_ish.first;
}

net_mobilewebprint::byte const * net_mobilewebprint::bv_non_endian::end(std::pair<char const *, char const *> const & str_ish)
{
  return (byte const*)str_ish.second;
}

size_t net_mobilewebprint::bv_non_endian::dsize(std::pair<char const *, char const *> const & str_ish)
{
  return end(str_ish) - begin(str_ish);
}

net_mobilewebprint::byte const * net_mobilewebprint::bv_non_endian::begin(std::pair<byte const *, byte const *> const & str_ish)
{
  return (byte const*)str_ish.first;
}

net_mobilewebprint::byte const * net_mobilewebprint::bv_non_endian::end(std::pair<byte const *, byte const *> const & str_ish)
{
  return (byte const*)str_ish.second;
}

size_t net_mobilewebprint::bv_non_endian::dsize(std::pair<byte const *, byte const *> const & str_ish)
{
  return end(str_ish) - begin(str_ish);
}

net_mobilewebprint::byte const * net_mobilewebprint::bv_non_endian::begin(byte const * pby)
{
  return pby;
}

net_mobilewebprint::byte const * net_mobilewebprint::bv_non_endian::end(byte const * pby)
{
  return pby + 1;
}

size_t       net_mobilewebprint::bv_non_endian::dsize(byte const * pby)
{
  return 1;
}

net_mobilewebprint::byte const * net_mobilewebprint::bv_non_endian::begin(byte const & n)
{
  return (byte const *)&n;
}

net_mobilewebprint::byte const * net_mobilewebprint::bv_non_endian::end(byte const & n)
{
  return begin(n) + sizeof(n);
}

size_t       net_mobilewebprint::bv_non_endian::dsize(byte const & n)
{
  return 1;
}

//----------------------------------------------------------------------------
//
//  buffer_to_dest_t
//
//----------------------------------------------------------------------------
net_mobilewebprint::buffer_to_dest_t::buffer_to_dest_t(buffer_t const * buffer_, string const & ip_, int port_)
  : buffer(buffer_), ip(ip_), port(port_), own_buffer(false)
{
}

net_mobilewebprint::buffer_to_dest_t::buffer_to_dest_t(buffer_t const & buffer_, string const & ip_, int port_)
  : buffer(NULL), ip(ip_), port(port_), own_buffer(true)
{
  buffer = new buffer_t(buffer_); /**/ num_buffer_allocations += 1;
}

net_mobilewebprint::buffer_to_dest_t::~buffer_to_dest_t()
{
  if (own_buffer) {
    delete buffer; /**/ num_buffer_allocations -= 1;
  }
}

//----------------------------------------------------------------------------
//
//  chunk_t
//
//----------------------------------------------------------------------------
net_mobilewebprint::chunk_t::chunk_t(buffer_t * data_, buffer_view_i const & view_)
  : data(data_), view(view_.const_begin(), view_.const_end())
{
}

net_mobilewebprint::chunk_t::~chunk_t()
{
  delete data; /**/ num_buffer_allocations -= 1;
  data = NULL;
}

std::string net_mobilewebprint::join(std::deque<chunk_t *> const & chunks, char const * sep)
{
  string result, chunk_str;
  chunk_t const * chunk = NULL;

  for (std::deque<chunk_t *>::const_iterator it = chunks.begin(); it != chunks.end(); ++it) {
    chunk     = *it;
    chunk_str = "";

    if (chunk != NULL) {
      chunk_str = chunk->view.str();
    }
    result += chunk_str;

    // Append separator
    if (it + 1 != chunks.end()) {
      result += sep;
    }
  }

  return result;
}

void net_mobilewebprint::push(net_mobilewebprint::chunks_t & chunks, net_mobilewebprint::buffer_t * data, net_mobilewebprint::buffer_view_i const & view)
{
  chunks.push_back(new net_mobilewebprint::chunk_t(data, view)); /**/ num_chunk_allocations += 1;
}

//----------------------------------------------------------------------------
//
//  sock_opt
//
//----------------------------------------------------------------------------
int net_mobilewebprint::sock_opt::level(e_option optname)
{
  if (optname >= so_level_first && optname <= so_level_last) { return SOL_SOCKET; }
  if (optname >= ip_level_first && optname <= ip_level_last) { return IPPROTO_IP; }

  return SOL_SOCKET;
}

int net_mobilewebprint::sock_opt::opt_name(e_option optname)
{
  switch (optname) {

  // SOCKET-level
  case so_broadcast:        return SO_BROADCAST;
  case so_keepalive:        return SO_KEEPALIVE;

  // IP-level
  case ip_multicast_if:     return IP_MULTICAST_IF;
  case ip_add_membership:   return IP_ADD_MEMBERSHIP;
  case ip_ttl:              return IP_TTL;
  case ip_multicast_ttl:    return IP_MULTICAST_TTL;

  // default
  default:                  return -1;
  }
  return -1;
}

char * net_mobilewebprint::sock_opt::opt_val(e_option optname, void * poptval_, int & intoptval, u_char & uchoptval)
{
  char * poptval = (char *)poptval_;

  switch (optname) {

  // Default int == 1
  case so_broadcast:
  case so_keepalive:
  case ip_ttl:
    if (intoptval == -999) {
      intoptval = 1;
    }
    return (char *)&intoptval;

  case ip_multicast_ttl:
    if (uchoptval == 99) {
      uchoptval = 1;
    }
    return (char *)&uchoptval;

  // Custom
  case ip_add_membership:
  case ip_multicast_if:
    return poptval;

  default:
    return poptval;
  }

  return poptval;
}

socklen_t net_mobilewebprint::sock_opt::opt_len(e_option optname, socklen_t optlen)
{
  switch (optname) {
  case so_broadcast:
  case so_keepalive:
  case ip_ttl:
    return sizeof(int);

  case ip_multicast_ttl:
    return sizeof(u_char);

  // Custom
  case ip_add_membership:
  case ip_multicast_if:
    return optlen;

  default:
    return optlen;
  }

  return optlen;
}

net_mobilewebprint::json_t::json_t()
  : int_attrs(NULL), bool_attrs(NULL)
{
  _init(false);
}

net_mobilewebprint::json_t::json_t(bool fromParsing)
  : int_attrs(NULL), bool_attrs(NULL)
{
  _init(fromParsing);
}

net_mobilewebprint::json_t::json_t(json_t const & that)
  : int_attrs(NULL), bool_attrs(NULL)
{
  _copy(that);
}

net_mobilewebprint::json_t::~json_t()
{
  if (int_attrs != NULL) {
    delete int_attrs;
    int_attrs = NULL;
  }

  if (bool_attrs != NULL) {
    delete bool_attrs;
    bool_attrs = NULL;
  }

}

net_mobilewebprint::json_t & net_mobilewebprint::json_t::operator=(json_t const & that)
{
  return _copy(that);
}

net_mobilewebprint::json_t & net_mobilewebprint::json_t::_copy(json_t const & that)
{
  if (this != &that) {
    str_attrs = that.str_attrs;
    if (int_attrs != NULL)  { delete int_attrs;  int_attrs = NULL; }
    if (bool_attrs != NULL) { delete bool_attrs; bool_attrs = NULL; }

    if (that.int_attrs != NULL) {
      int_attrs = new std::map<string, int>();
      *int_attrs = *that.int_attrs;
    }

    if (that.bool_attrs != NULL) {
      bool_attrs = new std::map<string, bool>();
      *bool_attrs = *that.bool_attrs;
    }
  }

  return *this;
}

net_mobilewebprint::json_t & net_mobilewebprint::json_t::_init(bool fromParsing)
{
  if (fromParsing) {
    str_attrs   = strmap();
    int_attrs   = new std::map<string, int>();
    bool_attrs  = new std::map<string, bool>();
  }

  return *this;
}

static std::string fixup_json_key(std::string const &key)
{
  return net_mobilewebprint::ltrim(key, '.');
}

net_mobilewebprint::json_t & net_mobilewebprint::json_t::insert(char const * key, string const & value)
{
  str_attrs.insert(make_pair(fixup_json_key(key), value));
  return *this;
}

net_mobilewebprint::json_t & net_mobilewebprint::json_t::insert(char const * key, char const * value)
{
  str_attrs.insert(make_pair(fixup_json_key(key), value));
  return *this;
}

net_mobilewebprint::json_t & net_mobilewebprint::json_t::insert(char const * key, int value)
{
  if (int_attrs == NULL) { int_attrs = new std::map<string, int>(); }

  int_attrs->insert(make_pair(fixup_json_key(key), value));

  return *this;
}

net_mobilewebprint::json_t & net_mobilewebprint::json_t::insert(char const * key, bool value)
{
  if (bool_attrs == NULL) { bool_attrs = new std::map<string, bool>(); }

  bool_attrs->insert(make_pair(fixup_json_key(key), value));

  return *this;
}

net_mobilewebprint::json_t & net_mobilewebprint::json_t::insert(string const & key, string const & value)
{
  str_attrs.insert(make_pair(fixup_json_key(key), value));
  return *this;
}

net_mobilewebprint::json_t & net_mobilewebprint::json_t::insert(string const & key, char const * value)
{
  str_attrs.insert(make_pair(fixup_json_key(key), value));
  return *this;
}

net_mobilewebprint::json_t & net_mobilewebprint::json_t::insert(string const & key, int value)
{
  if (int_attrs == NULL) { int_attrs = new std::map<string, int>(); }

  int_attrs->insert(make_pair(fixup_json_key(key), value));

  return *this;
}

net_mobilewebprint::json_t & net_mobilewebprint::json_t::insert(string const & key, bool value)
{
  if (bool_attrs == NULL) { bool_attrs = new std::map<string, bool>(); }

  bool_attrs->insert(make_pair(fixup_json_key(key), value));

  return *this;
}

bool net_mobilewebprint::json_t::has(char const * key) const
{
  return has_string(key) || has_int(key) || has_bool(key);
}

bool net_mobilewebprint::json_t::has_string(char const * key) const
{
  return str_attrs.find(key) != str_attrs.end();
}

bool net_mobilewebprint::json_t::has_int(char const * key) const
{
  if (int_attrs == NULL) { return false; }
  return int_attrs->find(key) != int_attrs->end();
}

bool net_mobilewebprint::json_t::has_bool(char const * key) const
{
  if (bool_attrs == NULL) { return false; }
  return bool_attrs->find(key) != bool_attrs->end();
}

bool net_mobilewebprint::json_t::has(string const & key) const
{
  return str_attrs.find(key) != str_attrs.end();
}

bool net_mobilewebprint::json_t::has_string(string const & key) const
{
  if (int_attrs == NULL) { return false; }
  return int_attrs->find(key) != int_attrs->end();
}

bool net_mobilewebprint::json_t::has_int(string const & key) const
{
  if (bool_attrs == NULL) { return false; }
  return bool_attrs->find(key) != bool_attrs->end();
}

std::string net_mobilewebprint::json_t::the_null_string = "";

std::string const & net_mobilewebprint::json_t::lookup(char const * key) const
{
  strmap::const_iterator it = str_attrs.find(key);
  if (it != str_attrs.end()) {
    return it->second;
  }

  return the_null_string;
}

std::string const & net_mobilewebprint::json_t::lookup(string const & key) const
{
  strmap::const_iterator it = str_attrs.find(key);
  if (it != str_attrs.end()) {
    return it->second;
  }

  return the_null_string;
}

int net_mobilewebprint::json_t::lookup(char const * key_, int def) const
{
  std::string key(key_);
  return lookup(key, def);
}

int net_mobilewebprint::json_t::lookup(string const & key, int def) const
{
  std::map<std::string, int>::const_iterator it = int_attrs->find(key);
  if (it != int_attrs->end()) {
    return it->second;
  }

  /* otherwise */
  if (has_string(key)) {
    return mwp_atoi(lookup(key));
  }

  return def;
}

int net_mobilewebprint::json_t::lookup_int(char const * key) const
{
  return lookup(key, 0);
}

int net_mobilewebprint::json_t::lookup_int(string const & key) const
{
  return lookup(key, 0);
}

bool net_mobilewebprint::json_t::lookup(char const * key, bool def) const
{
  std::map<std::string, bool>::const_iterator it = bool_attrs->find(key);
  if (it != bool_attrs->end()) {
    return it->second;
  }

  return def;
}

bool net_mobilewebprint::json_t::lookup(string const & key, bool def) const
{
  std::map<std::string, bool>::const_iterator it = bool_attrs->find(key);
  if (it != bool_attrs->end()) {
    return it->second;
  }

  /* otherwise */
  if (has_string(key)) {
    return lookup(key) == "true";
  }

  return def;
}

bool net_mobilewebprint::json_t::lookup_bool(char const * key) const
{
  return lookup(key, false);
}

bool net_mobilewebprint::json_t::lookup_bool(string const & key) const
{
  return lookup(key, false);
}


std::string net_mobilewebprint::json_t::stringify() const
{
  return JSON_stringify(str_attrs, int_attrs, bool_attrs);
}

net_mobilewebprint::json_t const & net_mobilewebprint::json_t::dump(bool force) const
{
  if (!force) {
    if (!get_flag("verbose"))               { return *this; }
    if (get_option("v_log_level", 0) < 4)   { return *this; }
  }

  log_d(1, "", "-------------\n");
  log_d(1, "", "strings(%d):\n", (int)str_attrs.size());
  for (strmap::const_iterator it = str_attrs.begin(); it != str_attrs.end(); ++it) {
    log_d(1, "", "  %s: %s\n", it->first.c_str(), it->second.c_str());
  }

  log_d(1, "", "ints(%d):\n", (int)int_attrs->size());
  for (std::map<std::string, int>::const_iterator it = int_attrs->begin(); it != int_attrs->end(); ++it) {
    log_d(1, "", "  %s: %d\n", it->first.c_str(), it->second);
  }

  log_d(1, "", "bools(%d):\n", (int)bool_attrs->size());
  for (std::map<std::string, bool>::const_iterator it = bool_attrs->begin(); it != bool_attrs->end(); ++it) {
    log_d(1, "", "  %s: %d\n", it->first.c_str(), (int)it->second);
  }
  log_d(1, "", "-------------\n");

  return *this;
}

static int fixup_json_key2(std::string &key)
{
  int       index = 0;
  string     key_ = net_mobilewebprint::ltrim(key, '.');
  strlist   parts = net_mobilewebprint::split(key_, '.');

  if (is_num(parts.front())) {
    index = mwp_atoi(parts.front());
    parts.pop_front();
  }

  key = join(parts, ".");
  return index;
}

using net_mobilewebprint::json_t;

json_t * _lookupp(std::map<int, json_t*> & arr, int index)
{
  json_t * result = NULL;

  std::map<int, json_t*>::iterator it = arr.find(index);
  if (it == arr.end()) {
    result = new json_t();
    result->_init(true);
    arr.insert(make_pair(index, result));
  } else {
    result = it->second;
  }

  return result;
}

net_mobilewebprint::json_array_t::json_array_t()
{
}

net_mobilewebprint::json_array_t::~json_array_t()
{
  for (std::map<int, json_t*>::iterator it = arr.begin(); it != arr.end(); ++it) {
    delete it->second;
  }
}

std::string net_mobilewebprint::json_array_t::stringify() const
{
  string result;
  for (std::map<int, json_t*>::const_iterator it = arr.begin(); it != arr.end(); ++it) {
    result += (*it).second->stringify();
  }

  return result;
}

bool net_mobilewebprint::json_array_t::has(int n) const
{
  return arr.size() > n;
}

json_t const *  net_mobilewebprint::json_array_t::get(int n) const
{
  if (!has(n)) { return NULL; }

  /* otherwise */
  return (*arr.find(n)).second;
}

void net_mobilewebprint::json_array_t::log_v_(int level, char const * mod, char const * pre) const
{
  log_v(level, mod, "%s", pre);

  for (int i = 0; i < arr.size(); ++i) {
    log_v(level, mod, "%s", get(i)->stringify().c_str());
  }
}

int net_mobilewebprint::json_array_t::insert(string const & key_, string const & value)
{
  string key = key_;
  int index = fixup_json_key2(key);
  //log_d(1, "", "4444444444444444444444444444444json_array_t::insert([%d] %s, %s)", index, key.c_str(), value.c_str());
  _lookupp(arr, index)->insert(key, value);
}

int net_mobilewebprint::json_array_t::insert(string const & key_, char const * value)
{
  string key = key_;
  int index = fixup_json_key2(key);
  _lookupp(arr, index)->insert(key, value);
}

int net_mobilewebprint::json_array_t::insert(string const & key_, int value)
{
  string key = key_;
  int index = fixup_json_key2(key);
  std::map<int, json_t*>::iterator it = arr.find(index);
  _lookupp(arr, index)->insert(key, value);
}

int net_mobilewebprint::json_array_t::insert(string const & key_, bool value)
{
  string key = key_;
  int index = fixup_json_key2(key);
  //log_d(1, "", "7444444444444444444444444444444json_array_t::insert([%d] %s, %d)", index, key.c_str(), (int)value);
  _lookupp(arr, index)->insert(key, value);
}

int net_mobilewebprint::json_array_t::insert(char const * key_, string const & value)
{
  string key = key_;
  int index = fixup_json_key2(key);
  _lookupp(arr, index)->insert(key, value);
}

int net_mobilewebprint::json_array_t::insert(char const * key_, char const * value)
{
  string key = key_;
  int index = fixup_json_key2(key);
  _lookupp(arr, index)->insert(key, value);
}

int net_mobilewebprint::json_array_t::insert(char const * key_, int value)
{
  string key = key_;
  int index = fixup_json_key2(key);
  _lookupp(arr, index)->insert(key, value);
}

int net_mobilewebprint::json_array_t::insert(char const * key_, bool value)
{
  string key = key_;
  int index = fixup_json_key2(key);
  _lookupp(arr, index)->insert(key, value);
}

int net_mobilewebprint::serialization_json_t::string_type = 1;
int net_mobilewebprint::serialization_json_t::int_type    = 2;
int net_mobilewebprint::serialization_json_t::bool_type   = 3;
int net_mobilewebprint::serialization_json_t::real_type   = 4;

net_mobilewebprint::serialization_json_t::serialization_json_t()
{
}

net_mobilewebprint::serialization_json_t::serialization_json_t(serialization_json_t const & that)
{
  for (elements_t::const_iterator it = that.elements.begin(); it != that.elements.end(); ++it) {
    elements.insert(make_pair(it->first, new serialization_json_elt_t(*(it->second))));
  }

  for (sub_elements_t::const_iterator it = that.sub_elements.begin(); it != that.sub_elements.end(); ++it) {
    sub_elements.insert(make_pair(it->first, new serialization_json_t(*(it->second))));
  }

  for (sub_list_t::const_iterator it = that.sub_list.begin(); it != that.sub_list.end(); ++it) {
    sub_list.insert(make_pair(it->first, new serialization_json_list_t(*(it->second))));
  }
}

net_mobilewebprint::serialization_json_t::~serialization_json_t()
{
  for (elements_t::const_iterator it = elements.begin(); it != elements.end(); ++it) {
    if (it->second != NULL) {
      delete it->second;
    }
  }

  for (sub_elements_t::const_iterator it = sub_elements.begin(); it != sub_elements.end(); ++it) {
    if (it->second != NULL) {
      delete it->second;
    }
  }
}

net_mobilewebprint::serialization_json_t & net_mobilewebprint::serialization_json_t::getObject(string const & key_)
{
  string parent_key;
  string key        = key_;

  if (_normalize_keys(parent_key, key)) {
    if (!_has(sub_elements, parent_key)) {
      sub_elements[parent_key] = new serialization_json_t();
    }

    return sub_elements[parent_key]->getObject(key);
  }

  /* otherwise */
  if (!_has(sub_elements, key)) {
    sub_elements[key] = new serialization_json_t();
  }

  return *sub_elements[key];
}

net_mobilewebprint::serialization_json_t::serialization_json_list_t & net_mobilewebprint::serialization_json_t::getList(string const & key_)
{
  log_v(2, "", "getList: %s", key_.c_str());

  string parent_key;
  string key        = key_;

  if (_normalize_keys(parent_key, key)) {
    // The key was parent.key;  Must return List from parent
    if (!_has(sub_elements, parent_key)) {
      sub_elements[parent_key] = new serialization_json_t();
    }

    return sub_elements[parent_key]->getList(key);
  }

  /* otherwise */
  if (!_has(sub_list, key)) {
    sub_list[key] = new serialization_json_list_t();
  }

  return *sub_list[key];
}

net_mobilewebprint::serialization_json_t::serialization_json_list_t & net_mobilewebprint::serialization_json_t::serialization_json_list_t::push_back(serialization_json_t const & item)
{
  list.push_back(item);
  return *this;
}

string net_mobilewebprint::serialization_json_t::serialization_json_list_t::stringify()
{
  strlist strlistItems;
  for (std::deque<serialization_json_t>::const_iterator it = list.begin(); it != list.end(); ++it) {
    strlistItems.push_back(it->stringify());
  }

  return string("[") + join(strlistItems, ",") + "]";
}

net_mobilewebprint::serialization_json_t & net_mobilewebprint::serialization_json_t::operator<<(serialization_json_t const & that)
{
  if (this != &that) {
    for (elements_t::const_iterator it = that.elements.begin(); it != that.elements.end(); ++it) {
      elements.insert(make_pair(it->first, new serialization_json_elt_t(*(it->second))));
    }

    for (sub_elements_t::const_iterator it = that.sub_elements.begin(); it != that.sub_elements.end(); ++it) {
      sub_elements.insert(make_pair(it->first, new serialization_json_t(*(it->second))));
    }

    for (sub_list_t::const_iterator it = that.sub_list.begin(); it != that.sub_list.end(); ++it) {
      sub_list.insert(make_pair(it->first, new serialization_json_list_t(*(it->second))));
    }
  }

  return *this;
}

std::string net_mobilewebprint::serialization_json_t::stringify() const
{
  string  quote("\"");
  strlist list;

  // First, my elements:
  for (elements_t::const_iterator it = elements.begin(); it != elements.end(); ++it) {
    list.push_back(quote + it->first + "\":" + it->second->stringify());
  }

  // Then the sub-elements
  for (sub_elements_t::const_iterator it = sub_elements.begin(); it != sub_elements.end(); ++it) {
    list.push_back(quote + it->first + "\":" + it->second->stringify());
  }

  // Then the sub-lists
  for (sub_list_t::const_iterator it = sub_list.begin(); it != sub_list.end(); ++it) {
    list.push_back(quote + it->first + "\":" + it->second->stringify());
  }

  return string("{") + join(list, ",") + "}";
}

void net_mobilewebprint::serialization_json_t::sjson_log_v(int log_level, char const * tags, int disp_level) const
{
  string  quote("\"");
  strlist list;

  string indent = "    ";
  for (int i = 0; i < disp_level; ++i) {
    indent += "  ";
  }

  // First, my elements:
  for (elements_t::const_iterator it = elements.begin(); it != elements.end(); ++it) {
    log_v(log_level, "", "%s%s: %s", indent.c_str(), it->first.c_str(), it->second->stringify().c_str());
  }

  // Then the sub-elements
  for (sub_elements_t::const_iterator it = sub_elements.begin(); it != sub_elements.end(); ++it) {
    log_v(log_level, "", "%s%s", indent.c_str(), it->first.c_str());
    it->second->sjson_log_v(log_level, tags, disp_level+1);
  }

  // Then the sub-lists
  for (sub_list_t::const_iterator it = sub_list.begin(); it != sub_list.end(); ++it) {
    log_v(log_level, "", "%s%s[]", indent.c_str(), it->first.c_str());
    serialization_json_list_t const * list = it->second;
    int index = 0;
    for (jsonlist::const_iterator itItem = list->list.begin(); itItem != list->list.end(); ++itItem, ++index) {
      log_v(log_level, "", "%s[%d]", indent.c_str(), index);
      itItem->sjson_log_v(log_level, tags, disp_level+2);
    }
  }

}

std::string net_mobilewebprint::serialization_json_t::serialization_json_elt_t::stringify()
{
  if (type == string_type) {
    return string("\"") + value + "\"";
  }

  /* otherwise */
  return value;
}

bool net_mobilewebprint::serialization_json_t::_normalize_keys(string & parent, string & key)
{
  bool result = false;
  strmap_entry kv;
  if (split_kv(kv, join(A(parent, key), "."), ".")) {
    if (kv.first.length() > 0) {
      parent = kv.first;
      key    = kv.second;
      result = true;
    } else {
      // This key wasn't composite
      parent = "";
      key    = kv.second;
    }
  }

  return result;
}

net_mobilewebprint::serialization_json_t::serialization_json_elt_t::serialization_json_elt_t(string const & value_)
{
  value = value_;
  type  = string_type;
}

net_mobilewebprint::serialization_json_t::serialization_json_elt_t::serialization_json_elt_t(char const * value_)
{
  value = value_;
  type  = string_type;
}

net_mobilewebprint::serialization_json_t::serialization_json_elt_t::serialization_json_elt_t(int value_)
{
  value = mwp_itoa(value_);
  type  = int_type;
}

net_mobilewebprint::serialization_json_t::serialization_json_elt_t::serialization_json_elt_t(bool value_)
{
  value = (value_ ? "true" : "false");
  type  = bool_type;
}

net_mobilewebprint::serialization_json_t::serialization_json_elt_t::serialization_json_elt_t(float value_)
{
  value = mwp_ftoa(value_);
  type  = real_type;
}

net_mobilewebprint::serialization_json_t::serialization_json_elt_t::serialization_json_elt_t(double value_)
{
  value = mwp_dtoa(value_);
  type  = real_type;
}


std::string net_mobilewebprint::lookup_ip(char const * name)
{
  int status = 0;

  struct addrinfo hints, *res = NULL, *p = NULL;
  ::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((status = getaddrinfo(name, NULL, &hints, &res)) != 0) {
    return "";
  }

  void * addr = NULL;

  bool found = false;
  for (p = res; p != NULL; p = p->ai_next) {
    if (p->ai_family == AF_INET) {    // IPv4
      struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
      addr = &(ipv4->sin_addr);
      found = true;
      break;
    }
  }

  if (!found) { return ""; }

  /* otherwise */
  char ipstr[INET6_ADDRSTRLEN];
  inet_ntop(AF_INET, addr, ipstr, sizeof(ipstr));
  freeaddrinfo(res);

  return ipstr;
}

using namespace net_mobilewebprint;

//-------------------------------

static string tags_to_log_;
static net_mobilewebprint::strset tags_to_log;

static std::map<string, net_mobilewebprint::strset> tag_sets;

static bool _should_log(char const * tags) {
#if 1
  return true;
#else
  string const & user_tags = get_option("log_tags");
  if (user_tags.length() == 0) { return true; }

  if (user_tags != tags_to_log_) {
    tags_to_log_ = user_tags;
    tags_to_log = splits(tags_to_log_, ':');
  }

  if (tag_sets.find(tags) == tag_sets.end()) {
    tag_sets[tags] = splits(tags, ':');
  }

  return (_intersection(tags_to_log, tag_sets[tags]).size() > 0);
#endif
}

void net_mobilewebprint::log_d(string const & msg)
{
  if (get_flag("quiet")) { return; }
  log_d(msg.c_str(), (log_param_t)NULL);
}

void net_mobilewebprint::log_w(string const & msg)
{
  if (get_flag("no_warn")) { return; }
  log_w(msg.c_str(), (log_param_t)NULL);
}

void net_mobilewebprint::log_v(string const & msg)
{
  if (!get_flag("verbose"))               { return; }
  if (get_option("v_log_level", 0) < 4)   { return; }

  log_v(msg.c_str(), (log_param_t)NULL);
}

void net_mobilewebprint::log_e(string const & msg)
{
  log_e(msg.c_str(), (log_param_t)NULL);
}

//void net_mobilewebprint::log_d(char const * tags, string const & msg)
//{
//  if (get_flag("quiet"))  { return; }
//  if (!_should_log(tags)) { return; }
//
//  log_d(msg.c_str(), (log_param_t)NULL);
//}

void net_mobilewebprint::log_w(char const * tags, string const & msg)
{
  if (get_flag("no_warn")) { return; }
  if (!_should_log(tags))  { return; }

  log_w(msg.c_str(), (log_param_t)NULL);
}

void net_mobilewebprint::log_v(int level, char const * tags, string const & msg)
{
  if (!get_flag("verbose"))                   { return; }
  if (level > get_option("v_log_level", 0))   { return; }
  if (!_should_log(tags))                     { return; }

  log_v(msg.c_str(), (log_param_t)NULL);
}

void net_mobilewebprint::log_e(char const * tag, string const & msg)
{
  log_e(msg.c_str(), (log_param_t)NULL);
}

//-------------------------------

void net_mobilewebprint::log_d(char const * format, ...)
{
  //return;
  if (get_flag("quiet")) { return; }

  va_list argList;

  char buffer[2048];

  va_start(argList, format);
  vsprintf(buffer, format, argList);
  va_end(argList);

  log_d(buffer, (log_param_t)NULL);
}

#if 0
void net_mobilewebprint::log_w(char const * format, ...)
{
  if (get_flag("no_warn")) { return; }

  va_list argList;

  char buffer[2048];

  va_start(argList, format);
  vsprintf(buffer, format, argList);
  va_end(argList);

  log_w(buffer, (log_param_t)NULL);
}
#endif

void net_mobilewebprint::log_v(char const * format, ...)
{
  //return;
  if (!get_flag("verbose"))               { return; }
  if (get_option("v_log_level", 0) < 4)   { return; }

  va_list argList;

  char buffer[2048];

  va_start(argList, format);
  vsprintf(buffer, format, argList);
  va_end(argList);

  log_v(buffer, (log_param_t)NULL);
}

#if 0
void net_mobilewebprint::log_e(char const * format, ...)
{
  va_list argList;

  char buffer[2048];

  va_start(argList, format);
  vsprintf(buffer, format, argList);
  va_end(argList);

  log_e(buffer, (log_param_t)NULL);
}
#endif

void net_mobilewebprint::log_d(int level, char const * tags, char const * format, ...)
{
  return;
  if (get_flag("quiet"))  { return; }
  if (!_should_log(tags)) { return; }

  va_list argList;

  char buffer[2048];

  va_start(argList, format);
  vsprintf(buffer, format, argList);
  va_end(argList);

  log_d(buffer, (log_param_t)NULL);
}

void net_mobilewebprint::log_d(char level, char const * tag, char const * format, ...)
{
  //return;
  if (get_flag("quiet"))                            { return; }
  if (level - '0' > get_option("v_log_level", 0))   { return; }

  va_list argList;

  char buffer[2048];

  va_start(argList, format);
  vsprintf(buffer, format, argList);
  va_end(argList);

  log_d(buffer, tag, (log_param_t)NULL);
}

void net_mobilewebprint::log_w(char const * tags, char const * format, ...)
{
  return;
  if (get_flag("no_warn")) { return; }
  if (!_should_log(tags))  { return; }

  va_list argList;

  char buffer[2048];

  va_start(argList, format);
  vsprintf(buffer, format, argList);
  va_end(argList);

  log_w(buffer, (log_param_t)NULL);
}

void net_mobilewebprint::log_v(int level, char const * tags, char const * format, ...)
{
  //return;
  if (!get_flag("verbose"))                   { return; }
  if (level > get_option("v_log_level", 0))   { return; }
  if (!_should_log(tags))                     { return; }

  va_list argList;

  char buffer[2048];

  va_start(argList, format);
  vsprintf(buffer, format, argList);
  va_end(argList);

  log_v(buffer, (log_param_t)NULL);
}

void net_mobilewebprint::log_vs(int level, char const * tag, char const * format, string const & big_str)
{
  if (!get_flag("verbose"))                         { return; }
  if (get_option("v_log_level", 0) < level - '0')   { return; }

  int len = ::strlen(format) + big_str.length() + 10;

  char * buffer = new char[len];
  ::memset(buffer, 0, len);
  ::strcpy(buffer, big_str.c_str());
  //sprintf(buffer, "%s", big_str.c_str());

  char * p     = buffer;
  char * pnull = p + 100;
  char * pend  = p + len + 1;

  do {
    if (pnull < pend) {
      char saved = *pnull;
      *pnull = 0;
      log_v(level, tag, format, p);
      *pnull = saved;
    } else {
      log_v(level, tag, format, p);
    }

    p     = pnull;
    pnull = p + 100;
  } while (p < pend);

  delete[] buffer;
}

void net_mobilewebprint::log_vs(int level, char const * tags, char const * format, string const & s1, string const & s2)
{
  //return;
  if (!get_flag("verbose"))                   { return; }
  if (level > get_option("v_log_level", 0))   { return; }
  if (!_should_log(tags))                     { return; }

  int len = ::strlen(format) + 100 + s1.length() + s2.length();

  // If the log entry will be small enough, just use the normal
  if (len < 1800) {
    log_v(level, tags, format, s1.c_str(), s2.c_str());
    return;
  }

  /* otherwise -- allocate a buffer and use that */
  char * buffer = new char[len];
  ::memset(buffer, 0, len);
  sprintf(buffer, format, s1.c_str(), s2.c_str());
  log_v("%s", buffer);
  delete[] buffer;
}

void net_mobilewebprint::log_vs(int level, char const * tags, char const * format, string const & s1, string const & s2, string const & s3)
{
  //return;
  if (!get_flag("verbose"))                   { return; }
  if (level > get_option("v_log_level", 0))   { return; }
  if (!_should_log(tags))                     { return; }

  int len = ::strlen(format) + 100 + s1.length() + s2.length() + s3.length();

  // If the log entry will be small enough, just use the normal
  if (len < 1800) {
    log_v(level, tags, format, s1.c_str(), s2.c_str(), s3.c_str());
    return;
  }

  /* otherwise -- allocate a buffer and use that */
  char * buffer = new char[len];
  ::memset(buffer, 0, len);
  sprintf(buffer, format, s1.c_str(), s2.c_str(), s3.c_str());
  log_v("%s", buffer);
  delete[] buffer;
}

void net_mobilewebprint::log_e(char const * tags, char const * format, ...)
{
  //return;
  if (!_should_log(tags)) { return; }

  va_list argList;

  char buffer[2048];

  va_start(argList, format);
  vsprintf(buffer, format, argList);
  va_end(argList);

  log_e(buffer, (log_param_t)NULL);
}

