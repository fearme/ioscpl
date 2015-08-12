
#ifndef __MWP_UTILS_HPP__
#define __MWP_UTILS_HPP__

#include "mwp_types.hpp"

#include <string>
#include <climits>

namespace net_mobilewebprint {

  using std::string;
  using net_mobilewebprint::strlist;

  string & _accumulate(string & str, string const & part,     char const * sep);
  string & _accumulate(string & str, char const * part, char const * sep);

  char const * _find(char const * str, char const * sub, char const * end = NULL, int sub_len = -1);

  strlist _split(string const & str, char const * sep, int num_splits = INT_MAX - 1);

  strmap & _add_kv(strmap & map, string const & str, char const * sep);
};

#endif // __MWP_UTILS_HPP__


