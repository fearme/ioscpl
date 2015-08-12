
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "mwp_assert.hpp"

using namespace net_mobilewebprint;

TEST_CASE("assert works")
{
  reset_assert_count();
  REQUIRE( num_asserts() == 0 );

  mwp_assert(false, NULL);

  REQUIRE( num_asserts() == 1 );
}

