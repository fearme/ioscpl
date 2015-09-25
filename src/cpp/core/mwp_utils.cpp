
#include "mwp_utils.hpp"
#include "mwp_types.hpp"
#include <cstdlib>

using std::map;
using std::make_pair;
using std::string;
using std::set;

using net_mobilewebprint::strmap;
using net_mobilewebprint::strlist;
using net_mobilewebprint::log_vs;

bool          is(char const *&p,  const char *&end, char const * str);
bool          match(char const *&p_, const char *&end, bool(*char_class)(char));

bool _identifier_(char ch);
bool _num_(char ch);
bool _ip_addr_(char ch);

string net_mobilewebprint::args_t::none = "";

net_mobilewebprint::args_t::args_t()
{
}

net_mobilewebprint::args_t & net_mobilewebprint::args_t::merge(args_t const & that)
{
  _extend(args, that.args);
  flags = _union(flags, that.flags);

  return *this;
}

string const & net_mobilewebprint::args_t::operator[](char const * key)
{
  map<string, string>::const_iterator it = args.find(key);
  if (it != args.end()) {
    return it->second;
  }
  return none;
}

string const & net_mobilewebprint::args_t::get(char const * key, string const & def)
{
  string const & result = this->operator[](key);
  if (result != none) { return result; }

  return def;
}

bool net_mobilewebprint::args_t::get_flag(char const * key)
{
  if (antiFlags.find(key) != antiFlags.end()) {
    return false;
  }

  /* otherwise */
  return (flags.find(key) != flags.end());
}

bool net_mobilewebprint::args_t::get_flag(char const * key, bool def)
{
  if (antiFlags.find(key) != antiFlags.end()) {
    return false;
  }

  if (flags.find(key) != flags.end()) {
    return true;
  }

  return def;
}

net_mobilewebprint::args_t & net_mobilewebprint::args_t::set_arg(char const * key, char const * value)
{
  return _insert(key, value);
}

net_mobilewebprint::args_t & net_mobilewebprint::args_t::set_arg(char const * key, string const & value)
{
  return _insert(key, value);
}

net_mobilewebprint::args_t & net_mobilewebprint::args_t::set_flag(char const *flag, bool value)
{
  return _insert_bool(flag, value);
}

net_mobilewebprint::args_t & net_mobilewebprint::args_t::clear_flag(char const *flag)
{
  flags.erase(flag);
  antiFlags.erase(flag);
  return *this;
}

net_mobilewebprint::args_t & net_mobilewebprint::args_t::_insert(string key, string value)
{
  if (args.find(key) == args.end()) {
    args.insert(make_pair(key, value));
  } else {
    args[key] = value;
  }
  return *this;
}

net_mobilewebprint::args_t & net_mobilewebprint::args_t::_insert_bool(string flag, bool value)
{
  flag = replace_chars(flag, "-", "_");

  // Special processing
  if (value) {
    if (flag == "vvvvvvvverbose")        { _insert("v_log_level", "8"); flag = "verbose"; }
    else if (flag == "vvvvvvverbose")    { _insert("v_log_level", "7"); flag = "verbose"; }
    else if (flag == "vvvvvverbose")     { _insert("v_log_level", "6"); flag = "verbose"; }
    else if (flag == "vvvvverbose")      { _insert("v_log_level", "5"); flag = "verbose"; }
    else if (flag == "vvvverbose")       { _insert("v_log_level", "4"); flag = "verbose"; }
    else if (flag == "vvverbose")        { _insert("v_log_level", "3"); flag = "verbose"; }
    else if (flag == "vverbose")         { _insert("v_log_level", "2"); flag = "verbose"; }
    else if (flag == "verbose")          { _insert("v_log_level", "1"); flag = "verbose"; }

    flags.insert(flag);
    antiFlags.erase(flag);

  } else {
    antiFlags.insert(flag);
    flags.erase(flag);
  }

  return *this;
}

bool net_mobilewebprint::args_t::_parse(char const * str, string & key, string & value)
{
  if (no_more_args) { return false; }

  int len = (int)strlen(str);
  char const *p = str, *key_begin = NULL, *key_end = str + len, *begin = NULL, *end = str + len;
  if (is(p, key_begin, "--")) {

    // -- means no more args
    if (*p == 0) {
      no_more_args = true;
      return false;
    }

    // --foo=bar (foo must be an "identifier")
    if (match(p, key_end, _identifier_)) {

      key = replace_chars(key_begin, key_end, "-", "_");

      if (is(p, begin, "=")) {
        value = string(begin, end);

        return true;
      } else {
        // No "=", so this is a flag

        //flags.insert(key);
        _insert_bool(key);
        return false;
      }
    }
  }

  return false;
}

bool net_mobilewebprint::is_ip_addr(char const * str)
{
  char const *end = NULL;
  if (!match(str, end, _ip_addr_)) { return false; }

  /* otherwise */
  return (*end == 0);
}

bool net_mobilewebprint::is_num(string const & str)
{
  return is_num(str.c_str());
}

bool net_mobilewebprint::is_num(char const * str)
{
  char const *end = NULL;
  if (!match(str, end, _num_)) { return false; }

  /* otherwise */
  return (*end == 0);
}

void net_mobilewebprint::log_d(map<string, string> const & dict, string pre, string post)
{
  log_d(pre);
  for (map<string, string>::const_iterator it = dict.begin(); it != dict.end(); ++it) {
    log_d("  %s: %s", it->first.c_str(), it->second.c_str());
  }
  log_d(post);
}

net_mobilewebprint::strlist net_mobilewebprint::split_on_parens(string const & str, char left, char right)
{
  net_mobilewebprint::strlist result;

  char const * begin = NULL;
  char const * p = str.c_str();

  for (p = skip_ws(p); *p; p = skip_ws(p)) {
    if (*(p = find(p, left)) == 0)        { break; }
    if (*(begin = ++p) == 0)              { break; }

    p = find(p, right);

    if (p - begin > 0) {
      result.push_back(string(begin, p));
    }

    if (*p) {
      ++p;
    }
  }

  return result;
}

char const * net_mobilewebprint::skip_ws(char const *p)
{
  return skip_ws((char *)p);
}

char * net_mobilewebprint::skip_ws(char *p)
{
  while (*p && (*p == ' ' || *p == '\t' || *p == '\n')) {
    p++;
  }

  return p;
}

char const * net_mobilewebprint::skip_ws(char const *&p, char const *end)
{
  return skip_ws((char *&)p, (char*)end);
}

char * net_mobilewebprint::skip_ws(char *&p, char *end)
{
  while (p < end && *p && (*p == ' ' || *p == '\t' || *p == '\n')) {
    p++;
  }

  return p;
}

string net_mobilewebprint::skip_char(string const & str, char ch)
{
  return skip_char(str.c_str(), ch);
}

char const * net_mobilewebprint::skip_char(char const *p, char ch)
{
  return skip_char((char *)p, ch);
}

char * net_mobilewebprint::skip_char(char *p, char ch)
{
  while (*p && *p == ch) {
    p++;
  }

  return p;
}

char const * net_mobilewebprint::skip_past_double_newline(char const *sz, char const * end)
{
  for (; *sz != 0 && (sz+4) <= end; ++sz) {
    if (*sz == '\r' && *(sz + 1) == '\n') {
      //printf("skipping (%x): %02x%02x%02x%02x %c%c\n", sz, *sz, *(sz+1), *(sz+2), *(sz+3), *(sz+2), *(sz+3));
      if (*(sz + 2) == '\r' && *(sz + 3) == '\n') {
        return sz + 4;
      }
    }
  }

  return NULL;
}

char const * net_mobilewebprint::find(char const *p, char ch)
{
  while (*p != 0 && *p != ch) {
    p++;
  }

  return p;
}

char const * net_mobilewebprint::find(char const *p, char const * sz)
{
  char const * psz, *p2;
  for (;*p != 0; ++p) {
    for (psz = sz, p2 = p; *psz != 0 && *p2 != 0; ++psz, ++p2) {
      if (*psz != *p2) {
        break;
      }
    }

    // If we found the null terminator for sz, we matched
    if (*psz == 0) {
      return p;
    }
  }

  return NULL;
}

unsigned char char_lc[] = {
     0,    1,    2,    3,    4,    5,    6,    7,    8,    9, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
  0x40,  'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
   'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'a', 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
  0x60,  'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
   'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z', 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
  0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xaa, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
  0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xba, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
  0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xca, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
  0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xda, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
  0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xea, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xfa, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

bool net_mobilewebprint::eq(char const * sz1, char const * sz2)
{
  for (; *sz1 != 0 && *sz2 != 0; ++sz1, ++sz2) {
    if (char_lc[(int)*sz1] != char_lc[(int)*sz2]) {
      break;
    }
  }

  return *sz1 == *sz2;
}

bool net_mobilewebprint::eq(string const & s1, char const * sz2)
{
  return eq(s1.c_str(), sz2);
}

bool net_mobilewebprint::_starts_with(string const & str, char const * sz_start)
{
  return _starts_with(str.c_str(), sz_start);
}

bool net_mobilewebprint::_starts_with(char const * sz, char const * sz_start)
{
  for (char const * p = sz_start; *p != 0; ++p, ++sz) {
    if (*p != *sz) {
      return false;
    }
  }

  return true;
}

static char blank[] = "";
char const *  net_mobilewebprint::or_blank(uint8 const * p)
{
  if (p == NULL) { return blank; }

  /* otherwise */
  return (char const *)p;
}

static char hex_digits[] = "0123456789abcdef";
string net_mobilewebprint::random_string(size_t length)
{
  string result = "";
  srand(get_tick_count());

  while (result.length() < length) {
    result += hex_digits[rand() % ::strlen(hex_digits)];
  }

  return result;
}

string & net_mobilewebprint::_accumulate(string & str, string const & part, char const * sep)
{
  return _accumulate(str, part.c_str(), sep);
}

string & net_mobilewebprint::_accumulate(string & str, char const * part, char const * sep)
{
  if (str.length() > 0 && sep != NULL) {
    str += sep;
  }

  str += part;

  return str;
}


bool net_mobilewebprint::_normalize_keys(string & parent, string & key)
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

net_mobilewebprint::strlist net_mobilewebprint::split(string const & str, char sep)
{
  return split(str.c_str(), sep);
}

net_mobilewebprint::strlist net_mobilewebprint::split(char const * str, char sep)
{
  strlist result;
  if (!*str) { return result; }

  char const * p1 = NULL, *p2 = str - 1;

  do {
    p1 = p2 + 1;
    p2 = find(p1, sep);
    result.push_back(string(p1, p2 - p1));

  } while (*p2);

  return result;
}

net_mobilewebprint::strlist net_mobilewebprint::split(char const * sz, char const * sep, char const * end)
{
  if (end == NULL) {
    end = sz + ::strlen(sz);
  }

  strlist result;
  size_t       sep_len = ::strlen(sep);
  char const * line_end = NULL;

  while (sz < end) {
    if ((line_end = find(sz, sep)) == NULL) { break; }

    /* otherwise */
    result.push_back(string(sz, line_end));
    sz = line_end + sep_len;
  }

  // Still need to add in the final item
  if (sz < end) {
    result.push_back(string(sz, end));
  }

  return result;
}

net_mobilewebprint::strlist net_mobilewebprint::split(char const * sz, char const * sep, int max_num_splits, char const * end)
{
  if (end == NULL) {
    end = sz + ::strlen(sz);
  }

  strlist result;
  size_t       sep_len = ::strlen(sep);
  char const * line_end = NULL;

  while (sz < end && result.size() < max_num_splits) {
    if ((line_end = find(sz, sep)) == NULL) { break; }

    /* otherwise */
    result.push_back(string(sz, line_end));
    sz = line_end + sep_len;
  }

  if (sz < end) {
    result.push_back(string(sz, end));
  }

  return result;
}

std::string net_mobilewebprint::parse_on(string & str, char const * sep, char const * end)
{
  string result;

  strlist list = split(str.c_str(), sep, 1, end);

  if (list.size() > 0) {
    result = list.front();
    list.pop_front();
  }

  if (list.size() > 0) {
    str = join(list, sep);
  }

  return result;
}

net_mobilewebprint::strvlist net_mobilewebprint::splitv(string const & str, char sep)
{
  return splitv(str.c_str(), sep);
}

net_mobilewebprint::strvlist net_mobilewebprint::splitv(char const * str, char sep)
{
  strvlist result;
  if (!*str) { return result; }

  char const * p1 = NULL, *p2 = str - 1;

  do {
    p1 = p2 + 1;
    p2 = find(p1, sep);
    result.push_back(string(p1, p2 - p1));

  } while (*p2);

  return result;
}

net_mobilewebprint::strvlist net_mobilewebprint::splitv(char const * sz, char const * sep, char const * end)
{
  if (end == NULL) {
    end = sz + ::strlen(sz);
  }

  strvlist result;
  size_t       sep_len = ::strlen(sep);
  char const * line_end = NULL;

  while (sz < end) {
    if ((line_end = find(sz, sep)) != NULL) {
      result.push_back(string(sz, line_end));
      sz = line_end + sep_len;
    }
  }

  return result;
}

net_mobilewebprint::strlist net_mobilewebprint::compact(strlist const & that)
{
  strlist result;

  strlist::const_iterator it;
  for (it = that.begin(); it != that.end(); ++it) {
    if (it->length() > 0) {
      result.push_back(*it);
    }
  }

  return result;
}

net_mobilewebprint::strvlist net_mobilewebprint::compact(strvlist const & that)
{
  strvlist result;

  strvlist::const_iterator it;
  for (it = that.begin(); it != that.end(); ++it) {
    if (it->length() > 0) {
      result.push_back(*it);
    }
  }

  return result;
}

net_mobilewebprint::strset net_mobilewebprint::setify(strvlist const & that)
{
  strset result;

  strvlist::const_iterator it;
  for (it = that.begin(); it != that.end(); ++it) {
    result.insert(*it);
  }

  return result;
}

net_mobilewebprint::strset net_mobilewebprint::splits(string const & str, char sep)
{
  return setify(compact(splitv(str, sep)));
}

net_mobilewebprint::strset net_mobilewebprint::splits(char const * sz, char sep)
{
  return setify(compact(splitv(sz, sep)));
}

net_mobilewebprint::strset net_mobilewebprint::splits(char const * sz, char const * sep, char const * end)
{
  return setify(compact(splitv(sz, sep, end)));
}

strlist & net_mobilewebprint::append_to(strlist & to, strlist const & addl)
{
  for (strlist::const_iterator it = addl.begin(); it != addl.end(); ++it) {
    to.push_back(*it);
  }

  return to;
}

void net_mobilewebprint::dump(strlist const & list)
{
  strlist::const_iterator it;
  for (it = list.begin(); it != list.end(); ++it) {
    log_d(1, "utils", "%s", it->c_str());
  }
}

bool net_mobilewebprint::_has(strmap const & dict, char const * key)
{
  return (dict.find(key) != dict.end());
}

int net_mobilewebprint::split_kv(net_mobilewebprint::strmap & result, string const & str, char sep1, char sep2, strmap * result_lc_out)
{
  strlist list = split(str, sep1);
  return parse_kv(list.begin(), list.end(), result, sep2, result_lc_out);
}

bool net_mobilewebprint::split_kv(std::pair<string, string> & result, string const & str, char sep, string * other)
{

  char const * begin = str.c_str(), *key_end = NULL, *value_begin = NULL;
  if (*begin) {
    if (*(key_end = find(begin, sep)) != 0) {
      string value, key(begin, key_end);
      if (*(value_begin = key_end+1) != 0) {
        value = string(value_begin, find(value_begin, '\0'));
      }

      result = make_pair(key, value);
      return true;

    } else if (other != NULL) {
      // *key_end is 0; there is no '='
      *other = begin;
    }
  }

  return false;
}

bool net_mobilewebprint::split_kv(std::pair<string, string> & result, string const & str, char const * sep)
{

  char const * begin = str.c_str(), *key_end = NULL, *value_begin = NULL;
  if (*begin) {
    if ((key_end = find(begin, sep)) != NULL && *key_end != 0) {
      string value, key(begin, key_end);
      if (*(value_begin = key_end+1) != 0) {
        value = string(value_begin, find(value_begin, '\0'));
      }

      result = make_pair(key, value);
      return true;
    }
  }

  return false;
}

string net_mobilewebprint::join(strmap const & that, char sep1, char sep2_)
{
  char sep2[5];
  sep2[0] = sep2_;
  sep2[1] = 0;

  strlist result;

  for (strmap::const_iterator it = that.begin(); it != that.end(); ++it) {
    result.push_back(it->first + sep1 + it->second);
  }

  return join(result, sep2);
}

bool net_mobilewebprint::_extend(strmap & self, strmap const & that)
{
  for (strmap::const_iterator it = that.begin(); it != that.end(); ++it) {
    if (!_has(self, it->first)) {
      self.insert(*it);
    } else {
      self[it->first] = it->second;
    }
  }

  return true;
}

string net_mobilewebprint::join(strlist const & list, char const * sep)
{
  string result;

  for (strlist::const_iterator it = list.begin(); it != list.end(); ++it) {
    result += *it;
    if (it + 1 != list.end()) {
      result += sep;
    }
  }

  return result;
}

net_mobilewebprint::strmap & net_mobilewebprint::add_kv(strmap & result, string const & key, string const & value)
{
  result.insert(make_pair(key, value));
  return result;
}

net_mobilewebprint::strmap & net_mobilewebprint::add_kv(strmap & result, char const * key, string const & value)
{
  return add_kv(result, string(key), value);
}

net_mobilewebprint::strmap & net_mobilewebprint::add_kv(strmap & result, string const & key, int32 value)
{
  result.insert(make_pair(key, mwp_itoa(value)));
  return result;
}

net_mobilewebprint::strmap & net_mobilewebprint::add_kv(strmap & result, char const * key, int32 value)
{
  return add_kv(result, string(key), value);
}

string & net_mobilewebprint::value(strmap::iterator it)
{
  return it->second;
}

string const & net_mobilewebprint::value(strmap::const_iterator it)
{
  return it->second;
}

string net_mobilewebprint::_lookup(strmap const & dict, string const & key)
{
  return _lookup(dict, key, "");
}

string net_mobilewebprint::_lookup(strmap const & dict, string const & key, char const * def)
{
  strmap::const_iterator it;
  if ((it = dict.find(key)) == dict.end()) { return def != NULL ? def : ""; }

  /* otherwise */
  return it->second;
}

int net_mobilewebprint::_lookup(strmap const & dict, string const & key, int def)
{
  string str_value = _lookup(dict, key, "");

  if (str_value.length() == 0) { return def; }

  /* otherwise */
  return mwp_atoi(str_value);
}

strlist & net_mobilewebprint::add_kvs(strlist & list, strmap const & dict)
{
  string str;

  for (strmap::const_iterator it = dict.begin(); it != dict.end(); ++it) {
    //log_d("******************************** Error in kv %s: %s", it->first.c_str(), it->second.c_str());
    str = string("\"") + it->first + "\" : \"" + it->second + "\"";
    list.push_back(str);
  }

  return list;
}

net_mobilewebprint::boolmap net_mobilewebprint::true_map(strlist const & list)
{
  boolmap result;

  for (strlist::const_iterator it = list.begin(); it != list.end(); ++it) {
    result.insert(make_pair(*it, true));
  }

  return result;
}

void net_mobilewebprint::dump(strmap const & dict)
{
  strmap::const_iterator it;
  for (it = dict.begin(); it != dict.end(); ++it) {
    string const & key = it->first;
    string const & val = it->second;

    log_d(2, "strmap", "%s: %s", key.c_str(), val.c_str());
  }
}

void net_mobilewebprint::dump(std::map<string, int> const & dict)
{
  std::map<string, int>::const_iterator it;
  for (it = dict.begin(); it != dict.end(); ++it) {
    string const & key = it->first;
    int val = it->second;

    log_d(2, "strmap", "%s: %d", key.c_str(), val);
  }
}

bool is(char const *& p, const char *&end, char const * str_class)
{
  int len = (int)strlen(str_class);
  if ((int)strlen(p) < len) { return false; }

  for (int i = len - 1; i > 0; --i) {
    if (str_class[i] != p[i]) { return false; }
  }

  // It matches!
  p += len;
  end = p;
  return true;
}

bool match(char const *&p_, const char *&end, bool(*char_class)(char))
{
  char const * p = p_;

  for (p = p_; *p; ++p) {
    if (!char_class(*p)) {
      break;
    }
  }

  if (p == p_) { return false; }

  end = p_ = p;
  return true;
}

bool _identifier_(char ch)
{
  if (ch == '_') { return true; }
  if (ch == '-') { return true; }
  if (ch >= 'a' && ch <= 'z') { return true; }
  if (ch >= 'A' && ch <= 'Z') { return true; }
  if (ch >= '0' && ch <= '9') { return true; }

  return false;
}

bool _num_(char ch)
{
  if (ch >= '0' && ch <= '9') { return true; }

  return false;
}

bool _ip_addr_(char ch)
{
  if (ch == '.') { return true; }
  if (ch >= '0' && ch <= '9') { return true; }

  return false;
}

string net_mobilewebprint::replace_chars(char const * str, char const * find, const char * repl)
{
  int len = (int)strlen(str), num = (int)strlen(find);

  string result;
  for (int i = 0; i < len; ++i) {
    char ch = str[i];
    for (int j = 0; j < num; ++j) {
      if (ch == find[j]) {
        ch = repl[j];
        break;
      }
    }

    result += ch;
  }

  return result;
}

string net_mobilewebprint::replace_chars(string const & str, char const * find, const char * repl)
{
  return replace_chars(str.c_str(), find, repl);
}

string net_mobilewebprint::replace_chars(char const * str_begin, char const * str_end, char const * find, const char * repl)
{
  int num = (int)strlen(find);
  string result;

  for (char const * p = str_begin; p != str_end; ++p) {
    char ch = *p;
    for (int j = 0; j < num; ++j) {
      if (ch == find[j]) {
        ch = repl[j];
        break;
      }
    }

    result += ch;
  }

  return result;
}

extern char const * net_mobilewebprint::ltrim(char const * sz)
{
  return skip_ws(sz);
}

extern char const * net_mobilewebprint::ltrim(char const * sz, char ch)
{
  return skip_char(sz, ch);
}

int net_mobilewebprint::tok_len(char const * sz)
{
  if (sz == NULL) { return -1; }

  char const * p = sz + strlen(sz) - 1;
  while (p >= sz && (*p == ' ' || *p == '\t' || *p == '\n')) {
    --p;
  }

  return (int)(p - sz);
}

int net_mobilewebprint::tok_len(char const * sz, char ch)
{
  if (sz == NULL) { return -1; }

  char const * p = sz + strlen(sz) - 1;
  while (p >= sz && *p == ch) {
    --p;
  }

  return (int)(p - sz);
}

char const * net_mobilewebprint::end_of(char const * sz)
{
  return sz + strlen(sz);
}

int net_mobilewebprint::tok_len(string const & str)
{
  char const * p = str.c_str();
  return tok_len(p);
}

int net_mobilewebprint::tok_len(string const & str, char ch)
{
  char const * p = str.c_str();
  return tok_len(p, ch);
}

string net_mobilewebprint::trim(string const & str)
{
  char const * start = ltrim(str.c_str());
  return string(start, tok_len(start) + 1);
}

string net_mobilewebprint::rtrim(string const & str)
{
  return string(str, 0, tok_len(str) + 1);
}

string net_mobilewebprint::rtrim(string const & str, char ch)
{
  return string(str, 0, tok_len(str, ch) + 1);
}

string net_mobilewebprint::dashify_key(string const & key)
{
  string result(key);
  char * p = (char *)result.c_str();

  for (; *p != 0; ++p) {
    if (*p == '.') { *p = '-'; }
  }

  return result;
}

string net_mobilewebprint::ltrim(string const & str)
{
  return string(skip_ws(str.c_str()));
}

string net_mobilewebprint::ltrim(string const & str, char ch)
{
  return string(skip_char(str.c_str(), ch));
}

void net_mobilewebprint::make_lower(string & str)
{
  const int diff = 'A' - 'a';

  size_t len = str.length();
  for (size_t i = 0; i < len; ++i) {
    char ch = str[i];
    if (ch >= 'A' && ch <= 'Z') {
      str[i] = ch - diff;
    }
  }
}

string net_mobilewebprint::_lower(string const & str)
{
  string result(str);
  make_lower(result);
  return result;
}

int net_mobilewebprint::mwp_atoi(char const * sz)
{
  int result = 0;

  if (!sz || !*sz) { return result; }

  if (!*(sz = skip_ws(sz))) { return result; }

  for (; *sz && *sz >= '0' && *sz <= '9'; ++sz) {
    result *= 10;
    result += (*sz - '0');
  }

  return result;
}

string net_mobilewebprint::replace(string const & str, char const *find, string const & replacement)
{
  size_t pos = str.find(find);
  if (pos == string::npos) { return str; }

  /* otherwise */
  string result(str);
  return result.replace(pos, ::strlen(find), replacement);
}

int    net_mobilewebprint::mwp_atoi(string const & str)
{
  return mwp_atoi(str.c_str());
}

/* reverse:  reverse string s in place -- From K&R */
void reverse(char s[])
{
  int i, j;
  char c;

  for (i = 0, j = (int)(strlen(s)-1); i<j; i++, j--) {
    c = s[i];
    s[i] = s[j];
    s[j] = c;
  }
}

std::string net_mobilewebprint::mwp_itoa(int n)
{
  char s[22];

  int i, sign;

  if ((sign = n) < 0) {  /* record sign */
    n = -n;              /* make n positive */
  }

  i = 0;
  do {                       /* generate digits in reverse order */
    s[i++] = n % 10 + '0';   /* get next digit */
  } while ((n /= 10) > 0);   /* delete it */

  if (sign < 0) {
    s[i++] = '-';
  }

  s[i] = '\0';
  reverse(s);

  return s;
}

std::string net_mobilewebprint::mwp_itoa(int n, int length)
{
  std::string result = mwp_itoa(n);

  std::string zero("0");
  while ((int)result.length() < length) {
    result = zero + result;
  }

  return result;
}

std::string net_mobilewebprint::mwp_ftoa(float n)
{
  char buffer[64];
  sprintf(buffer, "%f", n);
  return buffer;
}

std::string net_mobilewebprint::mwp_dtoa(double n)
{
  char buffer[64];
  sprintf(buffer, "%lf", n);
  return buffer;
}

/**
 *  Returns true if the timeout has elapsed.
 *
 *  If the timeout has elapsed, time is updated
 */
bool net_mobilewebprint::_has_elapsed(uint32 & time, uint32 timeout, uint32 current_time)
{
  if (current_time > time + timeout) {
    time = current_time;
    return true;
  }

  /* otherwise -- not elapsed */
  return false;
}

uint32 net_mobilewebprint::_time_since(uint32 time, uint32 start)
{
  if (start == 0) {
    start = get_tick_count();
  }

  return start - time;
}

using net_mobilewebprint::skip_ws;
using net_mobilewebprint::strmap;
using net_mobilewebprint::json_t;
using net_mobilewebprint::json_array_t;
using net_mobilewebprint::mwp_itoa;
using net_mobilewebprint::mwp_atoi;

static const char * skip_past(char ch, char const *& p_, char const * end /* in */)
{
  char const * p = skip_ws(p_, end);
  if (p < end && *p == ch) {
    return p_ = p + 1;
  }

  return NULL;
}

static const char * JSON_parse_string(char const *& p_, char const *& end /* in/out */)
{
  char const * p = p_, *start = p_;
  char const * mem_end = end;

  if (skip_ws(p, mem_end) != mem_end && *p != 0) {
    // Needs to be double-quoted
    if (*p != '"') { return NULL; }

    /* otherwise -- Look for an un-escaped ending double-quote */
    p += 1;
    start = p;
    for (;p < mem_end && *p; ++p) {
      if (*p == '\\') {
        p += 1;
        if (p >= mem_end || *p == 0) { return NULL; }

        /* otherwise */
        continue;
      } else if (*p == '"') {
        end = p;
        p_ = end + 1;
        return start;
      }
    }
  }

  return NULL;
}

template <typename T>
static bool JSON_parse_sub_unit(T & out, char const *& p, char const *& end_ /* in/out */, string unit_key)
{
  char const *     mem_end = end_;
  char const *         end = end_;
  char const *       start = NULL;
  string      sub_unit_key = unit_key + ".";

  string           key;

  // Look for the start of a sub-unit
  if (skip_ws(p, mem_end) != mem_end && *p != 0) {
    if (*p == '{') {
      p += 1;

      for (;;) {
        if (p >= mem_end || *p == 0) { return false; }
        end = mem_end;
        if ((start = JSON_parse_string(p, end)) != NULL) {
          key = string(start, end);
          if (skip_past(':', p, mem_end)) {
            if (p >= mem_end || *p == 0) { return false; }
            if (JSON_parse_sub_unit(out, p, mem_end, sub_unit_key + key)) {
              skip_ws(p, mem_end);
              if (p >= mem_end || *p == 0) { return false; }

              char ch = *p++;
              if (ch == ',') {
                continue;
              } else if (ch == '}') {
                break;
              }

              return false;
            } else {
              // Error - this is not valid JSON
              return false;
            }
          }
        } else {
          skip_ws(p, mem_end);
          if (*p == '}') {
            break;
          } else {
            // Error -- this is not valid JSON
            return false;
          }
        }
      }

      return true;
    } else if (*p == '"') {
      // A string value
      if ((start = JSON_parse_string(p, end)) != NULL) {

        out.insert(unit_key, string(start, end));
        return true;

      } else {
        // Error -- this is not valid JSON
        return false;
      }

    } else if (*p == '[') {
      p += 1;

      for (int i = 0; ; ++i) {
        if (p >= mem_end || *p == 0) { return false; }
        if (JSON_parse_sub_unit(out, p, mem_end, sub_unit_key + mwp_itoa(i))) {
          skip_ws(p, mem_end);
          if (p >= mem_end || *p == 0) { return false; }

          char ch = *p++;
          if (ch == ',') {
            continue;
          } else if (ch == ']') {
            break;
          }

          return false;
        } else {
          skip_ws(p, mem_end);
          if (*p == ']') {
            break;
          } else {
            // Error -- this is not valid JSON
            return false;
          }
        }
      }

      return true;
    } else {
      // The only thing left that we support is a number and true and false
      start = p;
      if (p + 4 < mem_end && *p == 't' && *(p+1) == 'r' && *(p+2) == 'u' && *(p+3) == 'e') {
        end = (p += 4);
        if (start == end) { return false; }
        out.insert(unit_key, true);

      } else if (p + 5 < mem_end && *p == 'f' && *(p+1) == 'a' && *(p+2) == 'l' && *(p+3) == 's' && *(p+4) == 'e') {
        end = (p += 5);
        if (start == end) { return false; }
        out.insert(unit_key, false);

      } else {
        for (; p < mem_end && ((*p >= '0' && *p <= '9') || *p == '.'); ++p) {
        }
        end = p;
        if (start == end) { return false; }
        out.insert(unit_key, mwp_atoi(string(start, end)));

      }

      return true;
    }
  }

  return false;
}

bool net_mobilewebprint::JSON_parse(json_t & out, string const & json_str)
{

  out._init(true);
  char const* start = json_str.c_str(), *end = start + json_str.length();
  if (JSON_parse_sub_unit(out, start, end, "")) {
    return true;
  }

  return false;
}

bool net_mobilewebprint::JSON_parse_array(json_array_t & out, string const & json_str)
{
  buffer_t json_buffer(json_str.c_str());

  char const* begin = (char const *)json_buffer.begin(), *end = (char const *)json_buffer.end();
  if (JSON_parse_sub_unit(out, begin, end, "")) {
    return true;
  }

  return false;
}

string net_mobilewebprint::JSON_debug_string(strmap const & dict, strlist const & skip_keys_, strlist const & key_order, std::map<string, int> const * numbers, std::map<string, bool> const * bools)
{
  string  result;

  string  quote("\"");
  boolmap skip_keys = true_map(skip_keys_);
  strmap  intermediate;

  if (bools != NULL) {
    for (std::map<string, bool>::const_iterator it = bools->begin(); it != bools->end(); ++it) {
      if (_has(skip_keys, it->first)) { continue; }
      intermediate.insert(make_pair(it->first, it->second ? " true" : "false"));
    }
  }

  for (strmap::const_iterator it = dict.begin(); it != dict.end(); ++it) {
    if (_has(skip_keys, it->first)) { continue; }
    intermediate.insert(make_pair(it->first, quote + it->second + quote));
  }

  if (numbers != NULL) {
    for (std::map<string, int>::const_iterator it = numbers->begin(); it != numbers->end(); ++it) {
      if (_has(skip_keys, it->first)) { continue; }
      intermediate.insert(make_pair(it->first, mwp_itoa(it->second)));
    }
  }

  // Output the keys from key_order

  for (strlist::const_iterator it = key_order.begin(); it != key_order.end(); ++it) {
    if (_has(intermediate, *it)) {
      _accumulate(result, intermediate.find(*it)->first + ": " + intermediate.find(*it)->second, ", ");
    }
  }

  boolmap key_order_map = true_map(key_order);
  for (strmap::const_iterator it = intermediate.begin(); it != intermediate.end(); ++it) {
    if (!_has(key_order_map, it->first)) {
      _accumulate(result, it->first + ": " + it->second, ", ");
    }
  }

  result = string("{") + result + "}";

  return result;
}

string net_mobilewebprint::JSON_stringify(strmap const & dict, std::map<string, int> const * numbers, std::map<string, bool> const * bools)
{
  string result;
  int    num_attrs = 0, num_metas = 0;

  result += "{";

  for (strmap::const_iterator it = dict.begin(); it != dict.end(); ++it) {
    if (num_attrs != 0){
      result += ",";
    }
    if (!_starts_with(it->first, "meta.")) {
      result += "\"" + it->first + "\" : \"" + it->second + "\"";
      num_attrs += 1;
    } else {
      num_metas += 1;
    }
  }

  if (numbers != NULL) {
    for (std::map<string, int>::const_iterator it = numbers->begin(); it != numbers->end(); ++it) {
      if (num_attrs != 0){
        result += ",";
      }
      if (!_starts_with(it->first, "meta.")) {
        result += "\"" + it->first + "\" : \"" + mwp_itoa(it->second) + "\"";
        num_attrs += 1;
      } else {
        num_metas += 1;
      }
    }
  }

  if (bools != NULL) {
    for (std::map<string, bool>::const_iterator it = bools->begin(); it != bools->end(); ++it) {
      if (num_attrs != 0){
        result += ",";
      }
      if (!_starts_with(it->first, "meta.")) {
        result += "\"" + it->first + "\" : " + (it->second ? "true" : "false");
        num_attrs += 1;
      } else {
        num_metas += 1;
      }
    }
  }

  // Now build the meta sub-obect
  if (num_metas > 0) {
    result += "\"meta\" : {";
    for (strmap::const_iterator it = dict.begin(); it != dict.end(); ++it) {
      if (num_attrs != 0){
        result += ",";
      }
      if (_starts_with(it->first, "meta.")) {
        result += "\"" + it->first.substr(5) + "\" : \"" + it->second + "\"";
        num_attrs += 1;
      }
    }

    if (numbers != NULL) {
      for (std::map<string, int>::const_iterator it = numbers->begin(); it != numbers->end(); ++it) {
        if (num_attrs != 0){
          result += ",";
        }
        if (_starts_with(it->first, "meta.")) {
          result += "\"" + it->first.substr(5) + "\" : \"" + mwp_itoa(it->second) + "\"";
          num_attrs += 1;
        }
      }
    }

    if (bools != NULL) {
      for (std::map<string, bool>::const_iterator it = bools->begin(); it != bools->end(); ++it) {
        if (num_attrs != 0){
          result += ",";
        }
        if (_starts_with(it->first, "meta.")) {
          result += "\"" + it->first.substr(5) + "\" : " + (it->second ? "true" : "false");
          num_attrs += 1;
        }
      }
    }

    result += "}";
  }


  result += "}";

  return result;
}




// Dangerous!
char * net_mobilewebprint::rtrim(char * sz)
{
  char * p = sz + strlen(sz) - 1;
  while (p >= sz && (*p == ' ' || *p == '\t' || *p == '\n')) {
    --p;
  }
  *p = 0;
  return sz;
}

