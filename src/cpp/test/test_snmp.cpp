
#include "catch.hpp"
#include "mwp_mdns.hpp"
#include "mwp_assert.hpp"

//#include <pcap.h>

using namespace net_mobilewebprint;

TEST_CASE("snmp_t can parse packet", "[snmp]")
{
  reset_assert_count();

  SECTION("read pcap file") {
  }

  REQUIRE( num_asserts() == 0 );
}


