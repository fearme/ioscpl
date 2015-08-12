
#ifndef __MWP_ASSERT_HPP__
#define __MWP_ASSERT_HPP__

namespace net_mobilewebprint {

  extern int num_failed_assertions;
  void reset_assert_count();

  int num_asserts();

  bool mwp_assert(bool, char const * cause);
};

#endif // __MWP_ASSERT_HPP__

