
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
  strmap & _add_kv(strmap & map, string const & str, char const * sep);
  strmap &    _add(strmap & map, string const & key, string const & value);

  // ----- Memory -----
  strlist & mem_dump(strlist & result, byte const * p, size_t length, char const * msg, int a = -1, int b = -1, int width = 8);
  void mem_dump(byte const * p, size_t length, char const * msg, int a = -1, int b = -1, int width = 8);
};

#endif // __MWP_UTILS_HPP__


