
#include "catch.hpp"
#include "mwp_utils.hpp"
#include "mwp_assert.hpp"

#include <string>

using namespace net_mobilewebprint;

using std::string;

const char   a_string[]     = "the quick brown fox\0jumped over the lazy dog";
const char   a_kv[]         = "a=b";
const char   a_mkv[]        = "foo;bar;baz;quxx";

static string fqdn("dev.mobilewebprint.net");
static string fqdnA("dev");
static string fqdnB("mobilewebprint");
static string fqdnC("net");

static string keyA("foo");
static string keyB("bar");

static string empty("");

TEST_CASE("utils parses uris", "[utils]")
{
  reset_assert_count();

  REQUIRE( num_asserts() == 0 );

  string protocol, user, host, port, path, fragment;
  strmap query;

  SECTION("parse proxy") {
    char const * uri = "http://proxy.houston.hp.com:8080/";
    parse_uri(uri, protocol, user, host, port, path, query, fragment);

    REQUIRE(protocol == "http");
    REQUIRE(user == "");
    REQUIRE(host == "proxy.houston.hp.com");
    REQUIRE(port == "8080");
    REQUIRE(path == "/");
    REQUIRE(fragment == "");
  }

  SECTION("parse our url") {
    char const * uri = "http://dev.mobiledevprint.net/path/to/resource";
    parse_uri(uri, protocol, user, host, port, path, query, fragment);

    REQUIRE(protocol == "http");
    REQUIRE(user == "");
    REQUIRE(host == "dev.mobiledevprint.net");
    REQUIRE(port == "");
    REQUIRE(path == "/path/to/resource");
    REQUIRE(fragment == "");
  }

  SECTION("understand empty string is no uri") {
    REQUIRE(!parse_uri("", protocol, user, host, port, path, query, fragment));

    REQUIRE(protocol == "");
    REQUIRE(user == "");
    REQUIRE(host == "");
    REQUIRE(port == "");
    REQUIRE(path == "");
    REQUIRE(fragment == "");
  }

  REQUIRE( num_asserts() == 0 );
}

TEST_CASE("utils parses args", "[utils]")
{
  reset_assert_count();

  REQUIRE( num_asserts() == 0 );

  strmap dict;
  strset flags;

  SECTION("parse args") {

    parse_args("--foo", dict, flags);
    REQUIRE(dict.size() == 0);
    REQUIRE(flags.size() == 1);

    REQUIRE(flags.find("foo") != flags.end());
    REQUIRE(flags.find("xfoo") == flags.end());

    parse_args("--Foo-bar=Baz", dict, flags);
    REQUIRE(dict.size() == 1);
    REQUIRE(flags.size() == 1);

    REQUIRE(dict.find("foo_bar") != dict.end());
    REQUIRE(dict["foo_bar"] == "Baz");

    parse_args("--foo-Bar", dict, flags);
    REQUIRE(dict.size() == 1);
    REQUIRE(flags.size() == 2);

    REQUIRE(flags.find("foo_bar") != flags.end());
    REQUIRE(flags.find("xfoo_bar") == flags.end());

  }

  SECTION("parse many args") {

    parse_args("--foo --BART=Mart --Roo-bar=quxx --bart-fooligan", dict, flags);
    REQUIRE(dict.size() == 2);
    REQUIRE(flags.size() == 2);

    REQUIRE(flags.find("foo") != flags.end());
    REQUIRE(flags.find("bart_fooligan") != flags.end());

    REQUIRE(dict.find("roo_bar") != dict.end());
    REQUIRE(dict["roo_bar"] == "quxx");

    REQUIRE(dict.find("bart") != dict.end());
    REQUIRE(dict["bart"] == "Mart");

  }

  REQUIRE( num_asserts() == 0 );
}

TEST_CASE("utils compact join", "[utils]")
{
  reset_assert_count();

  REQUIRE( num_asserts() == 0 );

  SECTION("split_kv 1") {
    strmap_entry kv;

    REQUIRE(_split_kv(kv, join(_compact(A(keyA, keyB)), "."), "."));

    REQUIRE(kv.first == keyA);
    REQUIRE(kv.second == keyB);
  }

  SECTION("split_kv 2") {
    strmap_entry kv;

    REQUIRE(_split_kv(kv, join(_compact(A(fqdnA, fqdnB, fqdnC)), "."), "."));

    REQUIRE(kv.first == fqdnA);
    REQUIRE(kv.second == join(A(fqdnB, fqdnC), "."));
  }

  SECTION("split_kv 3") {
    strmap_entry kv;

    REQUIRE(!_split_kv(kv, join(_compact(A(empty, keyB)), "."), "."));
  }

  SECTION("A and join") {
    REQUIRE(join(A(fqdnB), ".") == "mobilewebprint");
    REQUIRE(join(A(fqdnB, fqdnC), ".") == "mobilewebprint.net");
    REQUIRE(join(A(fqdnA, fqdnB, fqdnC), ".") == "dev.mobilewebprint.net");
  }

  SECTION("A and join and compact") {
    REQUIRE(join(_compact(A(empty, keyB)), ".") == "bar");
    REQUIRE(join(_compact(A(keyA, empty)), ".") == "foo");
    REQUIRE(join(_compact(A(keyA, keyB)), ".") == "foo.bar");
  }

  REQUIRE( num_asserts() == 0 );
}

TEST_CASE("utils is_num works", "[utils]")
{
  reset_assert_count();

  REQUIRE( num_asserts() == 0 );

  SECTION("_is_num validates number") {
    REQUIRE(_is_num(string("123")));
  }

  SECTION("_is_num invalidates number") {
    REQUIRE(!_is_num(string("bigbird")));
    REQUIRE(!_is_num(string("123bigbird")));
    REQUIRE(!_is_num(string("123 bigbird")));
    REQUIRE(!_is_num(string(" 123")));
  }

  REQUIRE( num_asserts() == 0 );
}


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


