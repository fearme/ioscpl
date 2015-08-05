
#include "catch.hpp"

#include "mwp_buffer.hpp"
#include "mwp_assert.hpp"

using namespace net_mobilewebprint;

TEST_CASE( "stoopid/1=1", "Prove that one equals 1")
{
  int one = 1;
  REQUIRE( one == 1 );
  REQUIRE( num_asserts() == 0 );
}

TEST_CASE( "assert works" )
{
  reset_assert_count();
  REQUIRE( num_asserts() == 0 );

  mwp_assert(false);

  REQUIRE( num_asserts() == 1 );
}

TEST_CASE( "buffer/can-hold-one-byte", "Prove that a buffer_t can hold a byte")
{
  buffer_t x((byte)1);

  REQUIRE( x.size() == 1 );
  REQUIRE( x.read_byte() == 1);
}

TEST_CASE( "buffer/can-hold-two-bytes", "Prove that a buffer_t can hold two bytes")
{
  buffer_t x((uint16)0x0202);

  REQUIRE( x.size() == 2 );
  REQUIRE( x.read_byte() == 2);
}

TEST_CASE( "buffer/can-hold-eight-bytes", "Prove that a buffer_t can hold eight bytes")
{
  uint16 a = 2;
  buffer_t x(a);

  x.append(a);
  x.append(a);
  x.append(a);

  REQUIRE( x.size() >= 8 );
}

TEST_CASE( "buffer/can-hold-sixteen-bytes", "Prove that a buffer_t can hold sixteen bytes")
{
  uint32 a = 2;
  buffer_t x(a);

  x.append(a);
  x.append(a);
  x.append(a);

  REQUIRE( x.size() >= 16 );
}

