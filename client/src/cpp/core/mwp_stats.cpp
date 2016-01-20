
#include "mwp_stats.hpp"
#include "mwp_utils.hpp"

#include <string>

using std::string;

net_mobilewebprint::stats_t::stats_t()
{
}

net_mobilewebprint::stats_t::stats_t(string const & name, int value)
{
  int_attrs.insert(std::make_pair(name, value));
}

void net_mobilewebprint::stats_t::make_server_json(serialization_json_t & json) const
{
  json.set(attrs);
  json.set(int_attrs);
  json.set(bool_attrs);
}

std::string net_mobilewebprint::stats_t::lookup(string const & name, char const * def) const
{
  typename strmap::const_iterator it = attrs.find(name);
  if (it == attrs.end()) {
    return def;
  }

  /* otherwise */
  return it->second;
}

std::string net_mobilewebprint::stats_t::lookup(string const & name, string const & def) const
{
  typename strmap::const_iterator it = attrs.find(name);
  if (it == attrs.end()) {
    return def;
  }

  /* otherwise */
  return it->second;
}

int    net_mobilewebprint::stats_t::lookup(string const & name, int def) const
{
  typename intmap::const_iterator it = int_attrs.find(name);
  if (it == int_attrs.end()) {
    return def;
  }

  /* otherwise */
  return it->second;
}

bool   net_mobilewebprint::stats_t::lookup(string const & name, bool def) const
{
  typename boolmap::const_iterator it = bool_attrs.find(name);
  if (it == bool_attrs.end()) {
    return def;
  }

  /* otherwise */
  return it->second;
}


#if 1
string net_mobilewebprint::stats_t::debug_to_json() const
{
  strlist list;

  for (strmap::const_iterator it = attrs.begin(); it != attrs.end(); ++it) {
    //if (it != attrs.begin()) {
    //  result += ",\n";
    //}

    list.push_back(string("  \"") + it->first + "\" : \"" + it->second + "\"");
  }

  for (intmap::const_iterator it = int_attrs.begin(); it != int_attrs.end(); ++it) {
    //if (it != int_attrs.begin()) {
    //  result += ",\n";
    //}

    list.push_back(string("  \"") + it->first + "\" : " + mwp_itoa(it->second));
  }

  for (boolmap::const_iterator it = bool_attrs.begin(); it != bool_attrs.end(); ++it) {
    //if (it != bool_attrs.begin()) {
    //  result += ",\n";
    //}

    list.push_back(string("  \"") + it->first + "\" : " + (it->second ? "true" : "false"));
  }

  string result = "{\n";
  result += join(list, ",\n");
  result += "\n}";
  return result;
}
#endif

