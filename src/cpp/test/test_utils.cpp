
#include "catch.hpp"
#include "mwp_utils.hpp"
#include "mwp_assert.hpp"

#include <string>

using namespace net_mobilewebprint;

using std::string;

const char   a_string[]     = "the quick brown fox\0jumped over the lazy dog";
const char   a_kv[]         = "a=b";
const char   a_mkv[]        = "foo;bar;baz;quxx";

TEST_CASE("utils works", "[utils]")
{
  reset_assert_count();

  REQUIRE( num_asserts() == 0 );

  SECTION("_accumulate works with one") {
    string str;
    _accumulate(str, "foo", ".");
    REQUIRE(str == "foo");
  }

  SECTION("_accumulate separates") {
    string str = "foo";

    _accumulate(str, "bar", ".");

    REQUIRE(str == "foo.bar");
  }

  SECTION("_find finds easy") {
    REQUIRE(_find(a_string, "quick") - a_string == 4);
    REQUIRE(_find(a_string, "xquick") == NULL);

    char const * fox = _find(a_string, "fox");
    REQUIRE(_find(a_string, "fox", fox + 3) != NULL);
    REQUIRE(_find(a_string, "fox", fox + 2) == NULL);

    REQUIRE(_find(a_string, "fox", fox + 2, 3) == NULL);
    REQUIRE(_find(a_string, "fox", fox + 2, 2) != NULL);
  }

  SECTION("_split splits singly on single-char separator") {

    strlist list = _split(a_kv, "=");

    strlist::const_iterator it = list.begin();
    REQUIRE(list.size() >= 1);
    REQUIRE(*it == "a");

    it += 1;
    REQUIRE(list.size() >= 2);
    REQUIRE(*it == "b");
  }

  SECTION("_split splits multiple times on single-char separator") {

    strlist list = _split(a_mkv, ";");

    strlist::const_iterator it = list.begin();
    REQUIRE(list.size() == 4);

    REQUIRE(*it++ == "foo");
    REQUIRE(*it++ == "bar");
    REQUIRE(*it++ == "baz");
    REQUIRE(*it++ == "quxx");
  }

  SECTION("_split splits singly on single-char separator when multiple items exist") {

    strlist list = _split(a_mkv, ";", 1);

    strlist::const_iterator it = list.begin();
    REQUIRE(list.size() == 2);

    REQUIRE(*it++ == "foo");
    REQUIRE(*it++ == "bar;baz;quxx");
  }

  SECTION("_add_kv adds a single key/value pair") {
    strmap dict;
    _add_kv(dict, a_kv, "=");
    REQUIRE(dict.size() == 1);
    REQUIRE(dict["a"] == "b");
  }

  SECTION("_add_kv adds a single key/value pair, handles no value") {
    strmap dict;
    _add_kv(dict, "foo=", "=");
    REQUIRE(dict.size() == 1);
    REQUIRE(dict["foo"] == "");
  }

  SECTION("_add_kv adds a single key/value pair, handles no sep") {
    strmap dict;
    _add_kv(dict, "foo", "=");
    REQUIRE(dict.size() == 0);
    REQUIRE(dict.find("foo") == dict.end());
  }

  REQUIRE( num_asserts() == 0 );
}


