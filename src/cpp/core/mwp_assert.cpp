
#include "mwp_assert.hpp"

int net_mobilewebprint::num_failed_assertions = 0;

bool net_mobilewebprint::mwp_assert(bool assertion)
{
  if (!assertion) {
    num_failed_assertions += 1;
  }

  return assertion;
}

void net_mobilewebprint::reset_assert_count()
{
  num_failed_assertions = 0;
}

int net_mobilewebprint::num_asserts()
{
  return num_failed_assertions;
}


