
#include "mwp_assert.hpp"

#include <cstdio>

int net_mobilewebprint::num_failed_assertions = 0;

bool net_mobilewebprint::mwp_assert(bool assertion, char const * cause)
{
  if (!assertion) {
    num_failed_assertions += 1;
    if (cause != NULL) {
      printf("Assert(%02d): %s\n", num_failed_assertions, cause);
    }
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


