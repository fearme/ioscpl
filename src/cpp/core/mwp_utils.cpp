
#include "mwp_utils.hpp"

#include <string>

using net_mobilewebprint::strlist;
using net_mobilewebprint::strmap;
using std::string;
using std::make_pair;

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

