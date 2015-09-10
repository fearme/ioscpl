
#ifndef __MWP_JSON_HPP__
#define __MWP_JSON_HPP__

#include "mwp_mq.hpp"

#include <string>

namespace net_mobilewebprint {

  struct json_for_server_t
  {

    static int string_type;
    static int int_type;
    static int bool_type;
    static int real_type;

    struct json_elt_for_server_t
    {
      string  value;
      //string  key;
      int     type;

      json_elt_for_server_t(string const &);
      json_elt_for_server_t(char const *);
      json_elt_for_server_t(int);
      json_elt_for_server_t(bool);
      json_elt_for_server_t(float);
      json_elt_for_server_t(double);

      string stringify();
    };

    typedef std::map<string, json_elt_for_server_t *> elements_t;
    typedef std::map<string, json_for_server_t *>     sub_elements_t;

    elements_t       elements;
    sub_elements_t   sub_elements;

    json_for_server_t();
    json_for_server_t(json_for_server_t const & that);

    ~json_for_server_t();

    json_for_server_t & getObject(string const & key_);
    string stringify() const;


    template <typename T>
    json_for_server_t & set(string const & key_, T const & value) {
      string       parent_key;
      string       key        = key_;

      // Is this a composite key?
      if (_normalize_keys(parent_key, key)) {
        return _set(parent_key, key, value);
      }

      // Free any memory we may already have been using
      if (_has(elements, key)) {
        delete elements[key]; /**/ num_allocations -= 1;
      }

      elements[key] = new json_elt_for_server_t(value); /**/ num_allocations += 1;
      return *this;
    }

    template <typename T>
    json_for_server_t & set(std::map<string, T> dict) {
      for (typename std::map<string, T>::const_iterator it = dict.begin(); it != dict.end(); ++it) {
        set(it->first, it->second);
      }

      return *this;
    }

    template <typename T>
    json_for_server_t & _set(string const & parent_key_, string const & key_, T const & value) {
      string parent_key    = parent_key_;
      string key           = key_;

      if (_normalize_keys(parent_key, key)) {
        if (!_has(sub_elements, parent_key)) {
          sub_elements[parent_key] = new json_for_server_t(); /**/ num_allocations += 1;
        }

        return sub_elements[parent_key]->set(key, value);
      }

      /* otherwise -- This wasn't really a composite key */
      return set(key, value);
    }

    // Returns true if the input was a composite key (so parent and key are both filled)
    // Returns false if the input was a non-compoiste key (had no '.'); only key is filled; parent is ""
    bool _normalize_keys(string & parent, string & key);



    template <typename T>
    bool _has(std::map<string, T> const & dict, string const & key) {
      return dict.find(key) != dict.end();
    }

    // This is not a general-purpose class
    private:
      json_for_server_t & operator=(json_for_server_t const &);
  };


  struct parsed_json_t
  {

    strmap                    str_attrs;
    std::map<string, int> *   int_attrs;
    std::map<string, bool> *  bool_attrs;

    parsed_json_t();
    parsed_json_t(bool fromParsing);
    parsed_json_t(parsed_json_t const & that);
    ~parsed_json_t();

    parsed_json_t & operator=(parsed_json_t const & that);

    parsed_json_t &        _init(bool fromParsing);

    parsed_json_t &       insert(string const & key, string const & value);
    parsed_json_t &       insert(string const & key, char const * value);
    parsed_json_t &       insert(string const & key, int value);
    parsed_json_t &       insert(string const & key, bool value);

    parsed_json_t &       insert(char const * key, string const & value);
    parsed_json_t &       insert(char const * key, char const * value);
    parsed_json_t &       insert(char const * key, int value);
    parsed_json_t &       insert(char const * key, bool value);

    bool              has(char const * key) const;
    bool       has_string(char const * key) const;
    bool          has_int(char const * key) const;
    bool         has_bool(char const * key) const;

    bool              has(string const & key) const;
    bool       has_string(string const & key) const;
    bool          has_int(string const & key) const;
    bool         has_bool(string const & key) const;

    string const & lookup(char const * key) const;
    string const & lookup(string const & key) const;

    int            lookup(char const * key, int def) const;
    int            lookup(string const & key, int def) const;
    int        lookup_int(char const * key) const;
    int        lookup_int(string const & key) const;

    bool           lookup(char const * key, bool def) const;
    bool           lookup(string const & key, bool def) const;
    bool      lookup_bool(char const * key) const;
    bool      lookup_bool(string const & key) const;

//    string      stringify() const;

//    parsed_json_t const &   dump(bool force = false) const;

    /* private */
    static string the_null_string;
    parsed_json_t &        _copy(parsed_json_t const & that);
  };

  struct parsed_json_array_t
  {
    std::map<int, parsed_json_t*> arr;

    parsed_json_array_t();
    ~parsed_json_array_t();

    bool            has(int n) const;
    parsed_json_t const *  get(int n) const;

    int             insert(string const & key, string const & value);
    int             insert(string const & key, char const * value);
    int             insert(string const & key, int value);
    int             insert(string const & key, bool value);

    int             insert(char const * key, string const & value);
    int             insert(char const * key, char const * value);
    int             insert(char const * key, int value);
    int             insert(char const * key, bool value);

//    string          stringify() const;

//    void            log_v_(int level, char const * mod, char const * pre) const;
  };

  extern bool       JSON_parse(parsed_json_t & out, string const & json);
  extern bool       JSON_parse_array(parsed_json_array_t & out, string const & json);
};

#endif  // __MWP_JSON_HPP__

