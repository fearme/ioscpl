
#ifndef __MWP_UTILS_HPP__
#define __MWP_UTILS_HPP__

#include "mwp_types.hpp"

#include <deque>
#include <string>
#include <climits>

namespace net_mobilewebprint {

  using std::string;
  using net_mobilewebprint::strlist;

  // ----- Strings -----
  string & _accumulate(string & str, string const & part,     char const * sep);
  string & _accumulate(string & str, char const * part, char const * sep);

  char const * _find(char const * str, char const * sub, char const * end = NULL, int sub_len = -1);

  string _rtrim(string const & str, string const & to_remove);

  string mwp_itoa(int);

  // ----- Lists -----
  strlist _split(string const & str, char const * sep, int num_splits = INT_MAX - 1);

  template <typename T>
  T* _pull(std::deque<T *> & list) {
    if (list.empty()) { return NULL; }

    /* otherwise */
    T * result = list.front();
    list.pop_front();
    return result;
  }

  // ----- Maps -----

  template <typename T>
  bool           _has(std::map<string, T> const & map, string const & key)
  {
    return map.find(key) != map.end();
  }

  strmap &    _add_kv(strmap & map, string const & str, char const * sep);
  strmap &       _add(strmap & map, string const & key, string const & value);
  strmap &       _add(strmap & map, string const & key, uint16 value);

  string const & _get(strmap const & map, string const & key);

  // ----- Memory -----
  strlist & mem_dump(strlist & result, byte const * p, size_t length, char const * msg, int a = -1, int b = -1, int width = 16);
  void      mem_dump(byte const * p, size_t length, char const * msg, int a = -1, int b = -1, int width = 16);

  // ----- Time -----
  uint32 _time_since(uint32 then, uint32 now = 0);
};

#endif // __MWP_UTILS_HPP__


