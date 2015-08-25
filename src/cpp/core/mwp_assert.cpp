
#include "mwp_assert.hpp"

#include <cstdio>

using std::string;

int    net_mobilewebprint::num_failed_assertions  = 0;
bool   net_mobilewebprint::silent                 = false;

string net_mobilewebprint::endl("\n");


net_mobilewebprint::logger net_mobilewebprint::log;

bool net_mobilewebprint::mwp_assert(bool assertion, char const * cause)
{
  if (!assertion) {
    num_failed_assertions += 1;
    if (!silent && cause != NULL) {
      printf("!!!!!!---------------------ASSERT(%02d): %s\n", num_failed_assertions, cause);
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


void net_mobilewebprint::mwp_silent_assert(bool silent_)
{
  net_mobilewebprint::silent = silent_;
}

