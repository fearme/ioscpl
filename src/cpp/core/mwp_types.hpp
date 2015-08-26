
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
  typedef std::deque<std::string>               strlist;
  typedef std::set<std::string>                 strset;

};

#endif // __MWP_TYPES_HPP__

