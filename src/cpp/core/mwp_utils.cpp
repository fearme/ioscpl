
#include "mwp_utils.hpp"
#include "mwp_buffer.hpp"
#include "mwp_assert.hpp"
#include <string>

using namespace net_mobilewebprint;

using net_mobilewebprint::strlist;
using net_mobilewebprint::strmap;
using net_mobilewebprint::log;
using std::string;
using std::make_pair;

string the_empty_string("");

// ---------------------------------------------------------------------------------------
// -------------------------------- string -----------------------------------------------
// ---------------------------------------------------------------------------------------

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

char const * net_mobilewebprint::_find(char const * str, char const * sub, char const * end_ /* = NULL */, int sub_len_ /* = -1 */)
{
  char const *       end = end_;
  int           sub_len = sub_len_;

  if (end == NULL) {
    end = str + ::strlen(str);
  }

  if (sub_len == -1) {
    sub_len = ::strlen(sub);
  }

  int               i = 0;
  char const *     p2 = NULL;
  char const *   sub2 = NULL;
  char const *      p = str;

  while (p < end) {
    for (p2 = p, sub2 = sub, i = 0; i < sub_len && p2 < end; ++i, ++p2, ++sub2) {
      if (*p2 != *sub2) {
        // They do not match
        break;
      }
    }

    // We get here on a match, and on not-a-match
    if (i == sub_len) {
      return p;
    }

    p += 1;
  }

  // If we get here, no match
  return NULL;
}

string net_mobilewebprint::_rtrim(string const & str, string const & to_remove)
{
  string result = str;
  size_t len    = to_remove.length();

  while (result.length() >= len && result.substr(result.length() - len) == to_remove) {
    result = result.substr(0, result.length() - len);
  }

  return result;
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

string net_mobilewebprint::mwp_itoa(int n)
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

// ---------------------------------------------------------------------------------------
// -------------------------------- strlist ----------------------------------------------
// ---------------------------------------------------------------------------------------

strlist net_mobilewebprint::_split(string const & str, char const * sep, int num_splits)
{
  int          max_size = num_splits + 1;
  size_t     sep_length = ::strlen(sep);
  size_t         length = str.length();
  char const *        p = str.c_str();
  char const *       p1 = p;
  char const *      end = p + length;

  strlist result;
  while (result.size() < max_size && p1 < end) {
    if ((p = _find(p1, sep, end, sep_length)) >= end || p == NULL) {
      break;
    }

    if (result.size() == max_size - 1) {
      p = end;
    }

    string value(p1, p);
    //printf("--%s--\n", value.c_str());
    result.push_back(value);
    p += sep_length;
    p1 = p;
  }

  if (p1 < end && p1 != NULL) {
    string value(p1, end);
    //printf("==%s==\n", value.c_str());
    result.push_back(value);
  }

  return result;
}

// ---------------------------------------------------------------------------------------
// -------------------------------- strmap -----------------------------------------------
// ---------------------------------------------------------------------------------------

/**
 *  Adds a key/value pair to the map.
 *
 *  For example, to add foo/bar, str should be "foo=bar" and sep should be "="
 */
strmap & net_mobilewebprint::_add_kv(strmap & map, string const & str, char const * sep)
{
  // Has to have separator
  if (_find(str.c_str(), sep) == NULL) {
    return map;
  }

  strlist                  list = _split(str, sep, 1);
  strlist::const_iterator    it = list.begin();

  if (list.size() < 1) {
    return map;
  }

  string key, value;
  if (it != list.end()) {
    key = *it++;
  }

  if (it != list.end()) {
    value = *it;
  }

  map.insert(make_pair(key, value));
  return map;
}

/**
 *  Adds a key/value pair to the map.
 *
 *  Essentially, is an easy way to call insert(make_pair(...))
 */
strmap & net_mobilewebprint::_add(strmap & map, string const & key, string const & value)
{
  map.insert(make_pair(key, value));
  return map;
}

/**
 *  Adds a key/value pair to the map.
 *
 *  Essentially, is an easy way to call insert(make_pair(...))
 */
strmap & net_mobilewebprint::_add(strmap & map, string const & key, uint16 value_)
{
  string value = mwp_itoa(value_);
  map.insert(make_pair(key, value));
  return map;
}

/**
 *  Returns the string for the key, or the empty string if the item is not present.
 */
string const & net_mobilewebprint::_get(strmap const & map, string const & key)
{
  strmap::const_iterator it = map.find(key);
  if (it != map.end()) {
    return (*it).second;
  }

  return the_empty_string;
}

// ---------------------------------------------------------------------------------------
// -------------------------------- Raw memory -------------------------------------------
// ---------------------------------------------------------------------------------------

strlist & net_mobilewebprint::mem_dump(strlist & result, byte const * p, size_t length, char const * msg, int a, int b, int width)
{
  buffer_range_t  buffer(p, length);
  buffer_reader_t as_hex(buffer);
  buffer_reader_t as_char(buffer);

  char   cbuf[64];
  char   ch = 0;
  string line;

  int i = 0, j = 0;
  for (i = 0; i < length; ) {

    line = "";

    mwp_snprintf(cbuf, sizeof(cbuf), "0x%08x(%08d):  ", i, i);
    line += cbuf;

    for (j = 0; j < width && i+j < length; ++j) {
      if (j == width/2) {
        line += "  ";
      }

      ch = ' ';
      if (i+j == a)         { ch = '['; }
      else if (i+j == b)    { ch = ']'; }

      mwp_snprintf(cbuf, sizeof(cbuf), "%c%02x", ch, (int)as_hex.read_byte());
      line += cbuf;
    }

    for (; j < width; ++j) {
      if (j == width/2) {
        line += "  ";
      }

      line += "   ";
    }

    line += "   ";

    for (j = 0; j < width && i < length; ++j, ++i) {
      if (j == width/2) {
        line += " ";
      }
      ch = as_char.read_byte();
      if (ch < 32 || ch > 126) {
        ch = '.';
      }
      mwp_snprintf(cbuf, sizeof(cbuf), "%c", (int)ch);
      line += cbuf;
    }

    result.push_back(line);
  }

  return result;
}

void net_mobilewebprint::mem_dump(byte const * p, size_t length, char const * msg, int a, int b, int width)
{
  log << msg << endl;
  //printf("%s\n", msg);

  strlist lines;
  mem_dump(lines, p, length, msg, a, b, width);

  for (strlist::const_iterator it = lines.begin(); it != lines.end(); ++it) {
    string const & str = *it;
    log << str << endl;
    //printf("%s\n", str.c_str());
  }

}

uint32 net_mobilewebprint::_time_since(uint32 then, uint32 now)
{
  if (now == 0) {
    now = host::get_tick_count();
  }

  return now - then;
}

