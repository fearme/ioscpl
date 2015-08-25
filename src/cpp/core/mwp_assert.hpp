
#ifndef __MWP_ASSERT_HPP__
#define __MWP_ASSERT_HPP__

#include "mwp_host.hpp"
#include <sstream>

using net_mobilewebprint::host::send_log;
using std::string;
using std::hex;
using std::dec;
using std::showbase;
using std::noshowbase;

namespace net_mobilewebprint {

  extern int           num_failed_assertions;
  void                 reset_assert_count();
  void                 mwp_silent_assert(bool silent = true);

  int                  num_asserts();

  bool                 mwp_assert(bool, char const * cause);

  extern bool          silent;

  // Logging
  struct logger {
    std::stringstream  ss;
  };

  extern logger log;

  template <typename T>
  logger & operator<<(logger & log, T x)
  {
    log.ss << x;

    send_log(0, log.ss);
    log.ss.str("");

    return log;
  }

  extern string endl;

};

#endif // __MWP_ASSERT_HPP__

