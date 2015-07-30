
#ifndef __MWP_ASSERT_HPP__
#define __MWP_ASSERT_HPP__

namespace net_mobilewebprint {

  extern int num_failed_assertions;

  int num_asserts();

  bool mwp_assert(bool);
};

#endif // __MWP_ASSERT_HPP__

