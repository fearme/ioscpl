
#include "catch.hpp"
#include "mwp_json.hpp"
#include "mwp_assert.hpp"

#include <string>

using namespace net_mobilewebprint;

using std::string;

char const * json_one_of_each =
  "{"
  "  \"str\" : \"foo\","
  "  \"num\" : 123,"
  "  \"baz\" : true"
  "}";

char const * json_compound =
  "{"
  "  \"sub\" : {"
  "    \"str\" : \"foo\","
  "    \"num\" : 123,"
  "    \"baz\" : true"
  "  }"
  "}";

TEST_CASE("JSON complex stringifying for server", "[json]")
{
  reset_assert_count();

  REQUIRE( num_asserts() == 0 );

  json_for_server_t json;
  json_for_server_t & inner_json = json.getObject("inner");

  SECTION("stringifies") {

    inner_json.set("str", "foo");

    inner_json.set("int", 123)
        .set("bool", true);
    json.set("inner.float", (float)1.23)
        .set("double", (double)4.567);

    string json_str = json.stringify().c_str();

    //printf("%s\n", json_str.c_str());
    //printf("%s\n", inner_json.stringify().c_str());

    REQUIRE(json_str.find("\"inner\":{") != string::npos);

    REQUIRE(json_str.find("\"bool\":true") != string::npos);
    REQUIRE(json_str.find("\"double\":4.56") != string::npos);
    REQUIRE(json_str.find("\"float\":1.23") != string::npos);
    REQUIRE(json_str.find("\"int\":123") != string::npos);
    REQUIRE(json_str.find("\"str\":\"foo\"") != string::npos);

    REQUIRE(json_str.find("\"xbool\":true") == string::npos);
    REQUIRE(json_str.find("\"xdouble\":4.56") == string::npos);
    REQUIRE(json_str.find("\"xfloat\":1.23") == string::npos);
    REQUIRE(json_str.find("\"xint\":123") == string::npos);
    REQUIRE(json_str.find("\"xstr\":\"foo\"") == string::npos);

  }

  REQUIRE( num_asserts() == 0 );
}

TEST_CASE("JSON stringifying for server", "[json]")
{
  reset_assert_count();
  //printf("%s\n", json_one_of_each);

  REQUIRE( num_asserts() == 0 );

  json_for_server_t json;

  SECTION("stringifies") {

    json.set("str", "foo");

    json.set("int", 123)
        .set("bool", true)
        .set("float", (float)1.23)
        .set("double", (double)4.567);

    string json_str = json.stringify().c_str();

    //printf("%s\n", json_str.c_str());

    REQUIRE(json_str.find("\"bool\":true") != string::npos);
    REQUIRE(json_str.find("\"double\":4.56") != string::npos);
    REQUIRE(json_str.find("\"float\":1.23") != string::npos);
    REQUIRE(json_str.find("\"int\":123") != string::npos);
    REQUIRE(json_str.find("\"str\":\"foo\"") != string::npos);

    REQUIRE(json_str.find("\"xbool\":true") == string::npos);
    REQUIRE(json_str.find("\"xdouble\":4.56") == string::npos);
    REQUIRE(json_str.find("\"xfloat\":1.23") == string::npos);
    REQUIRE(json_str.find("\"xint\":123") == string::npos);
    REQUIRE(json_str.find("\"xstr\":\"foo\"") == string::npos);

  }

  REQUIRE( num_asserts() == 0 );
}

TEST_CASE("JSON parsing", "[json]")
{
  reset_assert_count();
  //printf("%s\n", json_one_of_each);

  REQUIRE( num_asserts() == 0 );

  parsed_json_t json;
  bool did_parse = JSON_parse(json, json_one_of_each);

  SECTION("parses valid object and finds strings") {

    REQUIRE(did_parse);
    REQUIRE(json.has("str"));
    REQUIRE(!json.has("strx"));
    REQUIRE(json.lookup("str") == "foo");
    REQUIRE(json.lookup("str") != "bar");
  }

  SECTION("parses valid object and finds ints") {

    REQUIRE(did_parse);
    REQUIRE(json.has("num"));
    REQUIRE(!json.has("numx"));
    REQUIRE(json.lookup_int("num") == 123);
    REQUIRE(json.lookup_int("num") != 456);
  }

  SECTION("parses valid object and finds bools") {

    REQUIRE(did_parse);
    REQUIRE(json.has("baz"));
    REQUIRE(!json.has("bazx"));
    REQUIRE(json.lookup_bool("baz") == true);
    REQUIRE(json.lookup_bool("baz") != false);
  }

  REQUIRE( num_asserts() == 0 );
}

TEST_CASE("JSON compound parsing", "[json]")
{
  reset_assert_count();
  //printf("%s\n", json_one_of_each);

  REQUIRE( num_asserts() == 0 );

  parsed_json_t json;
  bool did_parse = JSON_parse(json, json_compound);

  SECTION("parses valid object and finds strings") {

    REQUIRE(did_parse);
    REQUIRE(json.has("sub.str"));
    REQUIRE(!json.has("sub.strx"));
    REQUIRE(json.lookup("sub.str") == "foo");
    REQUIRE(json.lookup("sub.str") != "bar");
  }

  SECTION("parses valid object and finds ints") {

    REQUIRE(did_parse);
    REQUIRE(json.has("sub.num"));
    REQUIRE(!json.has("sub.numx"));
    REQUIRE(json.lookup_int("sub.num") == 123);
    REQUIRE(json.lookup_int("sub.num") != 456);
  }

  SECTION("parses valid object and finds bools") {

    REQUIRE(did_parse);
    REQUIRE(json.has("sub.baz"));
    REQUIRE(!json.has("sub.bazx"));
    REQUIRE(json.lookup_bool("sub.baz") == true);
    REQUIRE(json.lookup_bool("sub.baz") != false);
  }

  REQUIRE( num_asserts() == 0 );
}

