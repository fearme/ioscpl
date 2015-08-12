
#include "catch.hpp"

#include "mwp_buffer.hpp"
#include "mwp_assert.hpp"

using namespace net_mobilewebprint;

using net_mobilewebprint::byte;
using net_mobilewebprint::buffer_view_t;

const byte test_buffer1[] = {
  0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0xff
};

const size_t test_buffer1_dsize = sizeof(test_buffer1) - 1;

const char   a_string[]        = "the quick brown fox\0jumped over the lazy dog";
const byte * a_string_buffer   = (byte*)&a_string[0];
const size_t a_string_nz_size  = ::strlen(a_string);

TEST_CASE("buffer_t handles strings", "[buffer_t]")
{
  reset_assert_count();
  REQUIRE( num_asserts() == 0 );

  bool     is_valid = true;
  buffer_t buffer(a_string_buffer, sizeof(a_string));

  SECTION("can read NULL-terminated string")
  {
    buffer_view_t::iterator p = buffer.first();
    REQUIRE(buffer.read_string(p, is_valid) == "the quick brown fox");
  }

  SECTION("can read more than one NULL-terminated string")
  {
    buffer_view_t::iterator p = buffer.first();
    REQUIRE(buffer.read_string(p, is_valid) == "the quick brown fox");
    REQUIRE(buffer.read_string(p, is_valid) == "jumped over the lazy dog");
  }

  SECTION("can read a string without NULL")
  {
    buffer_view_t::iterator p = buffer.first();
    REQUIRE(buffer.read_string_nz(p, a_string_nz_size, is_valid) == "the quick brown fox");
  }

  SECTION("can read two strings without NULL")
  {
    buffer_view_t::iterator p = buffer.first();
    REQUIRE(buffer.read_string_nz(p, a_string_nz_size, is_valid) == "the quick brown fox");
    REQUIRE(buffer.read_byte(p, is_valid) == 0);
    REQUIRE(buffer.read_string(p, is_valid) == "jumped over the lazy dog");
  }

  REQUIRE( num_asserts() == 0 );
}

TEST_CASE("buffer_t handles strings, and knows when it is being used wrongly", "[buffer_t]")
{
  reset_assert_count();
  REQUIRE( num_asserts() == 0 );

  bool     is_valid = true;
  buffer_t buffer(a_string_buffer, a_string_nz_size);

  SECTION("can read NULL-terminated string")
  {
    buffer_view_t::iterator p = buffer.first();
    REQUIRE(buffer.read_string(p, is_valid) == "");
    REQUIRE( num_asserts() == 1 );
    REQUIRE(is_valid == false);
  }

  SECTION("cannot read past end")
  {
    buffer_view_t::iterator p = buffer.end();
    REQUIRE(buffer.read_string(p, is_valid) == "");
    REQUIRE( num_asserts() == 1 );
    REQUIRE(is_valid == false);
  }

  SECTION("cannot read past end, for nz string")
  {
    buffer_view_t::iterator p = buffer.end();
    REQUIRE(buffer.read_string_nz(p, a_string_nz_size, is_valid) == "");
    REQUIRE( num_asserts() == 1 );
    REQUIRE(is_valid == false);
  }

}

TEST_CASE("buffer_t handles strings, and knows when it is being used wrongly, but asserts can be silenced", "[buffer_t]")
{
  reset_assert_count();
  REQUIRE( num_asserts() == 0 );

  bool     is_valid = true;
  buffer_t buffer(a_string_buffer, a_string_nz_size);

  SECTION("can read NULL-terminated string")
  {
    buffer_view_t::iterator p = buffer.first();
    REQUIRE(buffer.read_string(p, is_valid, true) == "");
    REQUIRE(is_valid == false);
  }

  SECTION("cannot read past end")
  {
    buffer_view_t::iterator p = buffer.end();
    REQUIRE(buffer.read_string(p, is_valid, true) == "");
    REQUIRE(is_valid == false);
  }

  SECTION("cannot read past end, for nz string")
  {
    buffer_view_t::iterator p = buffer.end();
    REQUIRE(buffer.read_string_nz(p, a_string_nz_size, is_valid, true) == "");
    REQUIRE(is_valid == false);
  }

  REQUIRE( num_asserts() == 0 );
}

TEST_CASE("buffer_t works", "[buffer_t]")
{
  reset_assert_count();
  REQUIRE( num_asserts() == 0 );

  buffer_t buffer(test_buffer1, test_buffer1_dsize);

  SECTION("first() on a buffer is valid")
  {
    buffer_view_t::iterator p = buffer.first();
    REQUIRE(buffer.is_valid(p));
  }

  SECTION("the end()-1 byte is valid")
  {
    buffer_view_t::iterator p = buffer.end() - 1;
    REQUIRE(buffer.is_valid(p));
  }

  REQUIRE( num_asserts() == 0 );
}

TEST_CASE("buffer_t knows when it is used wrong", "[buffer_t]")
{
  reset_assert_count();
  REQUIRE( num_asserts() == 0 );

  buffer_t buffer(test_buffer1, test_buffer1_dsize);

  SECTION("the end() byte is invalid")
  {
    buffer_view_t::iterator p = buffer.end();
    REQUIRE(!buffer.is_valid(p));
  }

  SECTION("reading byte past last is wrong")
  {
    bool is_valid = true;
    buffer_view_t::iterator p = buffer.end();
    REQUIRE(buffer.read_byte(p, is_valid) == 0);
    REQUIRE(!is_valid);
    REQUIRE( num_asserts() == 1 );
  }

  SECTION("reading uint16 past last is wrong")
  {
    bool is_valid = true;
    buffer_view_t::iterator p = buffer.end() - 1;
    REQUIRE(buffer.read_uint16(p, is_valid) == 0);
    REQUIRE(!is_valid);
    REQUIRE( num_asserts() == 1 );
  }

  SECTION("reading uint32 past last is wrong")
  {
    bool is_valid = true;
    buffer_view_t::iterator p = buffer.end() - 1;
    REQUIRE(buffer.read_uint32(p, is_valid) == 0);
    REQUIRE(!is_valid);
    REQUIRE( num_asserts() == 1 );
  }
}

TEST_CASE("buffer_t knows when it is used wrong, but asserts can be silenced", "[buffer_t]")
{
  reset_assert_count();
  REQUIRE( num_asserts() == 0 );

  buffer_t buffer(test_buffer1, test_buffer1_dsize);

  SECTION("the end() byte is invalid")
  {
    buffer_view_t::iterator p = buffer.end();
    REQUIRE(!buffer.is_valid(p));
  }

  SECTION("reading byte past last is wrong")
  {
    bool is_valid = true;
    buffer_view_t::iterator p = buffer.end();
    REQUIRE(buffer.read_byte(p, is_valid, true) == 0);
    REQUIRE(!is_valid);
  }

  SECTION("reading uint16 past last is wrong")
  {
    bool is_valid = true;
    buffer_view_t::iterator p = buffer.end() - 1;
    REQUIRE(buffer.read_uint16(p, is_valid, true) == 0);
    REQUIRE(!is_valid);
  }

  SECTION("reading uint32 past last is wrong")
  {
    bool is_valid = true;
    buffer_view_t::iterator p = buffer.end() - 1;
    REQUIRE(buffer.read_uint32(p, is_valid, true) == 0);
    REQUIRE(!is_valid);
  }

  REQUIRE( num_asserts() == 0 );
}

TEST_CASE("buffer_t can read data", "[buffer_t]")
{
  bool is_valid = true;
  reset_assert_count();
  REQUIRE( num_asserts() == 0 );

  buffer_t buffer(test_buffer1, test_buffer1_dsize);

  SECTION("reading 16 bits gets byte-order right")
  {
    buffer_view_t::iterator p = buffer.first();
    REQUIRE(buffer.read_uint16(p, is_valid) == 0xff01);
  }

  SECTION("reading 32 bits gets byte-order right")
  {
    buffer_view_t::iterator p = buffer.first();
    REQUIRE(buffer.read_uint32(p, is_valid) == 0xff010203);
  }

  SECTION("reading 16 bits moves iterator 2 bytes")
  {
    buffer_view_t::iterator p = buffer.first();
    buffer.read_uint16(p, is_valid);
    REQUIRE(p == buffer.first() + sizeof(uint16));
  }

  SECTION("reading 32 bits moves iterator 4 bytes")
  {
    buffer_view_t::iterator p = buffer.first();
    buffer.read_uint32(p, is_valid);
    REQUIRE(p == buffer.first() + sizeof(uint32));
  }

  REQUIRE( num_asserts() == 0 );
  REQUIRE( is_valid );
}

TEST_CASE("buffer_reader_t can read data", "[buffer_t]")
{
  reset_assert_count();
  REQUIRE( num_asserts() == 0 );

  buffer_t buffer(test_buffer1, test_buffer1_dsize);
  buffer_reader_t reader(buffer);

  SECTION("reading bytes works")
  {
    REQUIRE(reader.read_byte() == 0xff);
    REQUIRE(reader.read_byte() == 0x01);
  }

  SECTION("reading 16 bits gets byte-order right")
  {
    REQUIRE(reader.read_uint16() == 0xff01);
    REQUIRE(reader.read_uint16() == 0x0203);
  }

  SECTION("reading 32 bits gets byte-order right")
  {
    REQUIRE(reader.read_uint32() == 0xff010203);
    REQUIRE(reader.read_uint32() == 0x04050607);
  }

  REQUIRE( num_asserts() == 0 );
  REQUIRE( reader.is_valid() );
}

TEST_CASE("buffer_reader_t can read data, and not assert", "[buffer_t]")
{
  reset_assert_count();
  REQUIRE( num_asserts() == 0 );

  buffer_t buffer(test_buffer1, test_buffer1_dsize);
  buffer_reader_t reader(buffer, true);

  SECTION("reading 32 bits gets byte-order right")
  {
    REQUIRE(reader.read_uint32() == 0xff010203);
    REQUIRE(reader.read_uint32() == 0x04050607);
    REQUIRE(reader.read_uint32() == 0x08090a0b);
    REQUIRE(reader.read_uint32() == 0x0c0d0e0f);

    REQUIRE(reader.read_uint32() == 0x10111213);
    REQUIRE(reader.read_uint32() == 0x14151617);
    REQUIRE(reader.read_uint32() == 0x18191a1b);
    REQUIRE(reader.read_uint32() == 0x1c1d1e1f);

    REQUIRE(reader.is_valid());

    REQUIRE(reader.read_uint32() == 0);
  }

  SECTION("mixing reading types still no assert")
  {
    REQUIRE(reader.read_uint32() == 0xff010203);
    REQUIRE(reader.read_uint32() == 0x04050607);
    REQUIRE(reader.read_uint32() == 0x08090a0b);
    REQUIRE(reader.read_uint32() == 0x0c0d0e0f);

    REQUIRE(reader.read_uint32() == 0x10111213);
    REQUIRE(reader.read_uint32() == 0x14151617);
    REQUIRE(reader.read_uint32() == 0x18191a1b);

    REQUIRE(reader.read_uint16() == 0x1c1d);
    REQUIRE(reader.read_byte()   == 0x1e);

    REQUIRE(reader.is_valid());

    REQUIRE(reader.read_uint32() == 0);
  }

  REQUIRE( num_asserts() == 0 );
  REQUIRE( !reader.is_valid() );
}

