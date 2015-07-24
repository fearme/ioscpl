
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "mwp_buffer.hpp"

using namespace net_mobilewebprint;

TEST_CASE( "stoopid/1=2", "Prove that one equals 2")
{
  int one = 1;
  REQUIRE( one == 1 );
}

TEST_CASE( "buffer/can-hold-one-byte", "Prove that a buffer_ct can hold a byte")
{
  buffer_ct x((byte)1);

  REQUIRE( x.size() == 1 );
}

TEST_CASE( "buffer/can-hold-two-bytes", "Prove that a buffer_ct can hold two bytes")
{
  buffer_ct x((uint16)1);

  REQUIRE( x.size() == 2 );
}

