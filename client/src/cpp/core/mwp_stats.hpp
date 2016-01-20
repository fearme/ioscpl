
#ifndef __MWP_JOB_STATS_HPP__
#define __MWP_JOB_STATS_HPP__

#include "mwp_types.hpp"

namespace net_mobilewebprint {

  struct stats_t
  {
    strmap  attrs;
    intmap  int_attrs;
    boolmap bool_attrs;

    stats_t();
    stats_t(string const & name, int value);

    string lookup(string const & name, char const * def) const;
    string lookup(string const & name, string const & def) const;
    int    lookup(string const & name, int def) const;
    bool   lookup(string const & name, bool def) const;

    void make_server_json(serialization_json_t & json) const;
    string debug_to_json() const;
  };

};

#endif  // __MWP_JOB_STATS_HPP__
