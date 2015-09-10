
#include "mwp_controller.hpp"
#include "mwp_json.hpp"
#include "mwp_utils.hpp"
#include "mwp_assert.hpp"

using namespace net_mobilewebprint;

using std::string;
using std::make_pair;

//---------------------------------------------------------------------------------------------------
//------------------------------------- json_for_server_t -------------------------------------------
//---------------------------------------------------------------------------------------------------

int net_mobilewebprint::json_for_server_t::string_type = 1;
int net_mobilewebprint::json_for_server_t::int_type    = 2;
int net_mobilewebprint::json_for_server_t::bool_type   = 3;
int net_mobilewebprint::json_for_server_t::real_type   = 4;

net_mobilewebprint::json_for_server_t::json_for_server_t()
{
}

net_mobilewebprint::json_for_server_t::json_for_server_t(json_for_server_t const & that)
{
  if (this != &that) {
    for (elements_t::const_iterator it = that.elements.begin(); it != that.elements.end(); ++it) {
      elements.insert(make_pair(it->first, new json_elt_for_server_t(*(it->second))));
    }

    for (sub_elements_t::const_iterator it = that.sub_elements.begin(); it != that.sub_elements.end(); ++it) {
      sub_elements.insert(make_pair(it->first, new json_for_server_t(*(it->second))));
    }
  }
}

net_mobilewebprint::json_for_server_t::~json_for_server_t()
{
  for (elements_t::const_iterator it = elements.begin(); it != elements.end(); ++it) {
    if (it->second != NULL) {
      delete it->second;
    }
  }

  for (sub_elements_t::const_iterator it = sub_elements.begin(); it != sub_elements.end(); ++it) {
    if (it->second != NULL) {
      delete it->second;
    }
  }
}

net_mobilewebprint::json_for_server_t & net_mobilewebprint::json_for_server_t::getObject(string const & key_)
{
  string parent_key;
  string key        = key_;

  if (_normalize_keys(parent_key, key)) {
    if (!_has(sub_elements, parent_key)) {
      sub_elements[parent_key] = new json_for_server_t();
    }

    return sub_elements[parent_key]->getObject(key);
  }

  /* otherwise */
  if (!_has(sub_elements, key)) {
    sub_elements[key] = new json_for_server_t();
  }

  return *sub_elements[key];
}

std::string net_mobilewebprint::json_for_server_t::stringify() const
{
  string  quote("\"");
  strlist list;

  // First, my elements:
  for (elements_t::const_iterator it = elements.begin(); it != elements.end(); ++it) {
    list.push_back(quote + it->first + "\":" + it->second->stringify());
  }

  // Then the sub-elements
  for (sub_elements_t::const_iterator it = sub_elements.begin(); it != sub_elements.end(); ++it) {
    list.push_back(quote + it->first + "\":" + it->second->stringify());
  }

  return string("{") + join(list, ",") + "}";
}

std::string net_mobilewebprint::json_for_server_t::json_elt_for_server_t::stringify()
{
  if (type == string_type) {
    return string("\"") + value + "\"";
  }

  /* otherwise */
  return value;
}

bool net_mobilewebprint::json_for_server_t::_normalize_keys(string & parent, string & key)
{
  bool result = false;
  strmap_entry kv;
  if (_split_kv(kv, join(_compact(A(parent, key)), "."), ".")) {
    if (kv.first.length() > 0) {
      parent = kv.first;
      key    = kv.second;
      result = true;
    } else {
      // This key wasn't composite
      parent = "";
      key    = kv.second;
    }
  }

  return result;
}

net_mobilewebprint::json_for_server_t::json_elt_for_server_t::json_elt_for_server_t(string const & value_)
{
  value = value_;
  type  = string_type;
}

net_mobilewebprint::json_for_server_t::json_elt_for_server_t::json_elt_for_server_t(char const * value_)
{
  value = value_;
  type  = string_type;
}

net_mobilewebprint::json_for_server_t::json_elt_for_server_t::json_elt_for_server_t(int value_)
{
  value = mwp_itoa(value_);
  type  = int_type;
}

net_mobilewebprint::json_for_server_t::json_elt_for_server_t::json_elt_for_server_t(bool value_)
{
  value = (value_ ? "true" : "false");
  type  = bool_type;
}

net_mobilewebprint::json_for_server_t::json_elt_for_server_t::json_elt_for_server_t(float value_)
{
  value = mwp_ftoa(value_);
  type  = real_type;
}

net_mobilewebprint::json_for_server_t::json_elt_for_server_t::json_elt_for_server_t(double value_)
{
  value = mwp_dtoa(value_);
  type  = real_type;
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- parsed_json_t -----------------------------------------------
//---------------------------------------------------------------------------------------------------

net_mobilewebprint::parsed_json_t::parsed_json_t()
  : int_attrs(NULL), bool_attrs(NULL)
{
  _init(false);
}

net_mobilewebprint::parsed_json_t::parsed_json_t(bool fromParsing)
  : int_attrs(NULL), bool_attrs(NULL)
{
  _init(fromParsing);
}

net_mobilewebprint::parsed_json_t::parsed_json_t(parsed_json_t const & that)
  : int_attrs(NULL), bool_attrs(NULL)
{
  _copy(that);
}

net_mobilewebprint::parsed_json_t::~parsed_json_t()
{
  if (int_attrs != NULL) {
    delete int_attrs;
    int_attrs = NULL;
  }

  if (bool_attrs != NULL) {
    delete bool_attrs;
    bool_attrs = NULL;
  }

}

net_mobilewebprint::parsed_json_t & net_mobilewebprint::parsed_json_t::operator=(parsed_json_t const & that)
{
  return _copy(that);
}

net_mobilewebprint::parsed_json_t & net_mobilewebprint::parsed_json_t::_copy(parsed_json_t const & that)
{
  if (this != &that) {
    str_attrs = that.str_attrs;
    if (int_attrs != NULL)  { delete int_attrs;  int_attrs = NULL; }
    if (bool_attrs != NULL) { delete bool_attrs; bool_attrs = NULL; }

    if (that.int_attrs != NULL) {
      int_attrs = new std::map<string, int>();
      *int_attrs = *that.int_attrs;
    }

    if (that.bool_attrs != NULL) {
      bool_attrs = new std::map<string, bool>();
      *bool_attrs = *that.bool_attrs;
    }
  }

  return *this;
}

net_mobilewebprint::parsed_json_t & net_mobilewebprint::parsed_json_t::_init(bool fromParsing)
{
  if (fromParsing) {
    str_attrs   = strmap();
    int_attrs   = new std::map<string, int>();
    bool_attrs  = new std::map<string, bool>();
  }

  return *this;
}

static std::string fixup_json_key(std::string const &key)
{
  return net_mobilewebprint::_ltrim(key, '.');
}

net_mobilewebprint::parsed_json_t & net_mobilewebprint::parsed_json_t::insert(char const * key, string const & value)
{
  str_attrs.insert(make_pair(fixup_json_key(key), value));
  return *this;
}

net_mobilewebprint::parsed_json_t & net_mobilewebprint::parsed_json_t::insert(char const * key, char const * value)
{
  str_attrs.insert(make_pair(fixup_json_key(key), value));
  return *this;
}

net_mobilewebprint::parsed_json_t & net_mobilewebprint::parsed_json_t::insert(char const * key, int value)
{
  if (int_attrs == NULL) { int_attrs = new std::map<string, int>(); }

  int_attrs->insert(make_pair(fixup_json_key(key), value));

  return *this;
}

net_mobilewebprint::parsed_json_t & net_mobilewebprint::parsed_json_t::insert(char const * key, bool value)
{
  if (bool_attrs == NULL) { bool_attrs = new std::map<string, bool>(); }

  bool_attrs->insert(make_pair(fixup_json_key(key), value));

  return *this;
}

net_mobilewebprint::parsed_json_t & net_mobilewebprint::parsed_json_t::insert(string const & key, string const & value)
{
  str_attrs.insert(make_pair(fixup_json_key(key), value));
  return *this;
}

net_mobilewebprint::parsed_json_t & net_mobilewebprint::parsed_json_t::insert(string const & key, char const * value)
{
  str_attrs.insert(make_pair(fixup_json_key(key), value));
  return *this;
}

net_mobilewebprint::parsed_json_t & net_mobilewebprint::parsed_json_t::insert(string const & key, int value)
{
  if (int_attrs == NULL) { int_attrs = new std::map<string, int>(); }

  int_attrs->insert(make_pair(fixup_json_key(key), value));

  return *this;
}

net_mobilewebprint::parsed_json_t & net_mobilewebprint::parsed_json_t::insert(string const & key, bool value)
{
  if (bool_attrs == NULL) { bool_attrs = new std::map<string, bool>(); }

  bool_attrs->insert(make_pair(fixup_json_key(key), value));

  return *this;
}

bool net_mobilewebprint::parsed_json_t::has(char const * key) const
{
  return has_string(key) || has_int(key) || has_bool(key);
}

bool net_mobilewebprint::parsed_json_t::has_string(char const * key) const
{
  return str_attrs.find(key) != str_attrs.end();
}

bool net_mobilewebprint::parsed_json_t::has_int(char const * key) const
{
  if (int_attrs == NULL) { return false; }
  return int_attrs->find(key) != int_attrs->end();
}

bool net_mobilewebprint::parsed_json_t::has_bool(char const * key) const
{
  if (bool_attrs == NULL) { return false; }
  return bool_attrs->find(key) != bool_attrs->end();
}

bool net_mobilewebprint::parsed_json_t::has(string const & key) const
{
  return str_attrs.find(key) != str_attrs.end();
}

bool net_mobilewebprint::parsed_json_t::has_string(string const & key) const
{
  if (int_attrs == NULL) { return false; }
  return int_attrs->find(key) != int_attrs->end();
}

bool net_mobilewebprint::parsed_json_t::has_int(string const & key) const
{
  if (bool_attrs == NULL) { return false; }
  return bool_attrs->find(key) != bool_attrs->end();
}

std::string net_mobilewebprint::parsed_json_t::the_null_string = "";

std::string const & net_mobilewebprint::parsed_json_t::lookup(char const * key) const
{
  strmap::const_iterator it = str_attrs.find(key);
  if (it != str_attrs.end()) {
    return it->second;
  }

  return the_null_string;
}

std::string const & net_mobilewebprint::parsed_json_t::lookup(string const & key) const
{
  strmap::const_iterator it = str_attrs.find(key);
  if (it != str_attrs.end()) {
    return it->second;
  }

  return the_null_string;
}

int net_mobilewebprint::parsed_json_t::lookup(char const * key_, int def) const
{
  std::string key(key_);
  return lookup(key, def);
}

int net_mobilewebprint::parsed_json_t::lookup(string const & key, int def) const
{
  std::map<std::string, int>::const_iterator it = int_attrs->find(key);
  if (it != int_attrs->end()) {
    return it->second;
  }

  /* otherwise */
  if (has_string(key)) {
    return mwp_atoi(lookup(key));
  }

  return def;
}

int net_mobilewebprint::parsed_json_t::lookup_int(char const * key) const
{
  return lookup(key, 0);
}

int net_mobilewebprint::parsed_json_t::lookup_int(string const & key) const
{
  return lookup(key, 0);
}

bool net_mobilewebprint::parsed_json_t::lookup(char const * key, bool def) const
{
  std::map<std::string, bool>::const_iterator it = bool_attrs->find(key);
  if (it != bool_attrs->end()) {
    return it->second;
  }

  return def;
}

bool net_mobilewebprint::parsed_json_t::lookup(string const & key, bool def) const
{
  std::map<std::string, bool>::const_iterator it = bool_attrs->find(key);
  if (it != bool_attrs->end()) {
    return it->second;
  }

  /* otherwise */
  if (has_string(key)) {
    return lookup(key) == "true";
  }

  return def;
}

bool net_mobilewebprint::parsed_json_t::lookup_bool(char const * key) const
{
  return lookup(key, false);
}

bool net_mobilewebprint::parsed_json_t::lookup_bool(string const & key) const
{
  return lookup(key, false);
}


//std::string net_mobilewebprint::parsed_json_t::stringify() const
//{
//  return JSON_stringify(str_attrs, int_attrs, bool_attrs);
//}

//net_mobilewebprint::parsed_json_t const & net_mobilewebprint::parsed_json_t::dump(bool force) const
//{
//  if (!force) {
////    if (!get_flag("verbose"))               { return *this; }
////    if (get_option("v_log_level", 0) < 4)   { return *this; }
//  }
//
//  log_d(1, "", "-------------\n");
//  log_d(1, "", "strings(%d):\n", (int)str_attrs.size());
//  for (strmap::const_iterator it = str_attrs.begin(); it != str_attrs.end(); ++it) {
//    log_d(1, "", "  %s: %s\n", it->first.c_str(), it->second.c_str());
//  }
//
//  log_d(1, "", "ints(%d):\n", (int)int_attrs->size());
//  for (std::map<std::string, int>::const_iterator it = int_attrs->begin(); it != int_attrs->end(); ++it) {
//    log_d(1, "", "  %s: %d\n", it->first.c_str(), it->second);
//  }
//
//  log_d(1, "", "bools(%d):\n", (int)bool_attrs->size());
//  for (std::map<std::string, bool>::const_iterator it = bool_attrs->begin(); it != bool_attrs->end(); ++it) {
//    log_d(1, "", "  %s: %d\n", it->first.c_str(), (int)it->second);
//  }
//  log_d(1, "", "-------------\n");
//
//  return *this;
//}

static int fixup_json_key2(std::string &key)
{
  int       index = 0;
  string     key_ = net_mobilewebprint::_ltrim(key, '.');
  strlist   parts = net_mobilewebprint::_split(key_, ".");

  if (_is_num(parts.front())) {
    index = mwp_atoi(parts.front());
    parts.pop_front();
  }

  key = join(parts, ".");
  return index;
}

using net_mobilewebprint::parsed_json_t;

parsed_json_t * _lookupp(std::map<int, parsed_json_t*> & arr, int index)
{
  parsed_json_t * result = NULL;

  std::map<int, parsed_json_t*>::iterator it = arr.find(index);
  if (it == arr.end()) {
    result = new parsed_json_t();
    result->_init(true);
    arr.insert(make_pair(index, result));
  } else {
    result = it->second;
  }

  return result;
}

net_mobilewebprint::parsed_json_array_t::parsed_json_array_t()
{
}

net_mobilewebprint::parsed_json_array_t::~parsed_json_array_t()
{
  for (std::map<int, parsed_json_t*>::iterator it = arr.begin(); it != arr.end(); ++it) {
    delete it->second;
  }
}

//std::string net_mobilewebprint::parsed_json_array_t::stringify() const
//{
//  string result;
//  for (std::map<int, parsed_json_t*>::const_iterator it = arr.begin(); it != arr.end(); ++it) {
//    result += (*it).second->stringify();
//  }
//
//  return result;
//}

bool net_mobilewebprint::parsed_json_array_t::has(int n) const
{
  return arr.size() > n;
}

parsed_json_t const *  net_mobilewebprint::parsed_json_array_t::get(int n) const
{
  if (!has(n)) { return NULL; }

  /* otherwise */
  return (*arr.find(n)).second;
}

//void net_mobilewebprint::parsed_json_array_t::log_v_(int level, char const * mod, char const * pre) const
//{
//  log_v(level, mod, "%s", pre);
//
//  for (int i = 0; i < arr.size(); ++i) {
//    log_v(level, mod, "%s", get(i)->stringify().c_str());
//  }
//}

int net_mobilewebprint::parsed_json_array_t::insert(string const & key_, string const & value)
{
  string key = key_;
  int index = fixup_json_key2(key);
  //log_d(1, "", "4444444444444444444444444444444json_array_t::insert([%d] %s, %s)", index, key.c_str(), value.c_str());
  _lookupp(arr, index)->insert(key, value);

  return 0;
}

int net_mobilewebprint::parsed_json_array_t::insert(string const & key_, char const * value)
{
  string key = key_;
  int index = fixup_json_key2(key);
  _lookupp(arr, index)->insert(key, value);

  return 0;
}

int net_mobilewebprint::parsed_json_array_t::insert(string const & key_, int value)
{
  string key = key_;
  int index = fixup_json_key2(key);
  _lookupp(arr, index)->insert(key, value);

  return 0;
}

int net_mobilewebprint::parsed_json_array_t::insert(string const & key_, bool value)
{
  string key = key_;
  int index = fixup_json_key2(key);
  _lookupp(arr, index)->insert(key, value);

  return 0;
}

int net_mobilewebprint::parsed_json_array_t::insert(char const * key_, string const & value)
{
  string key = key_;
  int index = fixup_json_key2(key);
  _lookupp(arr, index)->insert(key, value);

  return 0;
}

int net_mobilewebprint::parsed_json_array_t::insert(char const * key_, char const * value)
{
  string key = key_;
  int index = fixup_json_key2(key);
  _lookupp(arr, index)->insert(key, value);

  return 0;
}

int net_mobilewebprint::parsed_json_array_t::insert(char const * key_, int value)
{
  string key = key_;
  int index = fixup_json_key2(key);
  _lookupp(arr, index)->insert(key, value);

  return 0;
}

int net_mobilewebprint::parsed_json_array_t::insert(char const * key_, bool value)
{
  string key = key_;
  int index = fixup_json_key2(key);
  _lookupp(arr, index)->insert(key, value);

  return 0;
}


static const char * skip_past(char ch, char const *& p_, char const * end /* in */)
{
  char const * p = _skip_ws(p_, end);
  if (p < end && *p == ch) {
    return p_ = p + 1;
  }

  return NULL;
}

static const char * JSON_parse_string(char const *& p_, char const *& end /* in/out */)
{
  char const * p = p_, *start = p_;
  char const * mem_end = end;

  if (_skip_ws(p, mem_end) != mem_end && *p != 0) {
    // Needs to be double-quoted
    if (*p != '"') { return NULL; }

    /* otherwise -- Look for an un-escaped ending double-quote */
    p += 1;
    start = p;
    for (;p < mem_end && *p; ++p) {
      if (*p == '\\') {
        p += 1;
        if (p >= mem_end || *p == 0) { return NULL; }

        /* otherwise */
        continue;
      } else if (*p == '"') {
        end = p;
        p_ = end + 1;
        return start;
      }
    }
  }

  return NULL;
}

template <typename T>
static bool JSON_parse_sub_unit(T & out, char const *& p, char const *& end_ /* in/out */, string unit_key)
{
  char const *     mem_end = end_;
  char const *         end = end_;
  char const *       start = NULL;
  string      sub_unit_key = unit_key + ".";

  string           key;

  // Look for the start of a sub-unit
  if (_skip_ws(p, mem_end) != mem_end && *p != 0) {
    if (*p == '{') {
      p += 1;

      for (;;) {
        if (p >= mem_end || *p == 0) { return false; }
        end = mem_end;
        if ((start = JSON_parse_string(p, end)) != NULL) {
          key = string(start, end);
          if (skip_past(':', p, mem_end)) {
            if (p >= mem_end || *p == 0) { return false; }
            if (JSON_parse_sub_unit(out, p, mem_end, sub_unit_key + key)) {
              _skip_ws(p, mem_end);
              if (p >= mem_end || *p == 0) { return false; }

              char ch = *p++;
              if (ch == ',') {
                continue;
              } else if (ch == '}') {
                break;
              }

              return false;
            }
          }
        }
      }

      return true;
    } else if (*p == '"') {
      // A string value
      if ((start = JSON_parse_string(p, end)) != NULL) {
        out.insert(unit_key, string(start, end));
        return true;
      }
    } else if (*p == '[') {
      p += 1;

      for (int i = 0; ; ++i) {
        if (p >= mem_end || *p == 0) { return false; }
        if (JSON_parse_sub_unit(out, p, mem_end, sub_unit_key + mwp_itoa(i))) {
          _skip_ws(p, mem_end);
          if (p >= mem_end || *p == 0) { return false; }

          char ch = *p++;
          if (ch == ',') {
            continue;
          } else if (ch == ']') {
            break;
          }

          return false;
        }
      }

      return true;
    } else {
      // The only thing left that we support is a number and true and false
      start = p;
      if (p + 4 < mem_end && *p == 't' && *(p+1) == 'r' && *(p+2) == 'u' && *(p+3) == 'e') {
        end = (p += 4);
        if (start == end) { return false; }
        out.insert(unit_key, true);

      } else if (p + 5 < mem_end && *p == 'f' && *(p+1) == 'a' && *(p+2) == 'l' && *(p+3) == 's' && *(p+4) == 'e') {
        end = (p += 5);
        if (start == end) { return false; }
        out.insert(unit_key, false);

      } else {
        for (; p < mem_end && ((*p >= '0' && *p <= '9') || *p == '.'); ++p) {
        }
        end = p;
        if (start == end) { return false; }
        out.insert(unit_key, mwp_atoi(string(start, end)));

      }

      return true;
    }
  }

  return false;
}

bool net_mobilewebprint::JSON_parse(parsed_json_t & out, string const & json_str)
{
  out._init(true);
  char const* start = json_str.c_str(), *end = start + json_str.length();
  if (JSON_parse_sub_unit(out, start, end, "")) {
    return true;
  }

  return false;
}

bool net_mobilewebprint::JSON_parse_array(parsed_json_array_t & out, string const & json_str)
{
  char const* begin = json_str.c_str(), *end = begin + json_str.length();
  if (JSON_parse_sub_unit(out, begin, end, "")) {
    return true;
  }

  return false;
}


