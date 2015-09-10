
#ifndef __MWP_TYPES_HPP__
#define __MWP_TYPES_HPP__

#include <map>
#include <string>
#include <deque>
#include <set>

#define mwp_snprintf snprintf

namespace net_mobilewebprint {

  typedef unsigned char  byte;
  typedef unsigned short uint16;
  typedef unsigned int   uint32;

  typedef std::map<std::string, std::string>    strmap;
  typedef std::pair<std::string, std::string>   strmap_entry;
  typedef std::deque<std::string>               strlist;
  typedef std::set<std::string>                 strset;

  template<typename T>
  void _A_accum(std::deque<T> & result, T a) {
    result.push_back(a);
  }

  template <typename T>
  std::deque<T> A(T a) {
    std::deque<T> result;

    _A_accum(result, a);

    return result;
  };

  template <typename T>
  std::deque<T> A(T a, T b) {
    std::deque<T> result;

    _A_accum(result, a);
    _A_accum(result, b);

    return result;
  };

  template <typename T>
  std::deque<T> A(T a, T b, T c) {
    std::deque<T> result;

    _A_accum(result, a);
    _A_accum(result, b);
    _A_accum(result, c);

    return result;
  };

};

#endif // __MWP_TYPES_HPP__

