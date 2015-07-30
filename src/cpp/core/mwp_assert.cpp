
#include "mwp_assert.hpp"

int net_mobilewebprint::num_failed_assertions = 0;

bool net_mobilewebprint::mwp_assert(bool assertion)
{
  if (!assertion) {
    num_failed_assertions += 1;
  }

  return assertion;
}

int net_mobilewebprint::num_asserts()
{
  return num_failed_assertions;
}


