
#ifndef __MWP_UTILS_HPP__
#define __MWP_UTILS_HPP__

#include "mwp_types.hpp"

#include <map>
#include <set>
#include <deque>
#include <string>
#include <algorithm>

namespace net_mobilewebprint {

  using std::map;
  using std::set;
  using std::deque;
  using std::string;

  using net_mobilewebprint::strmap;
  using net_mobilewebprint::strlist;

  //-----------------------------------------------------------------------------
  //
  //  String manipulation
  //
  //-----------------------------------------------------------------------------
  extern char const *  end_of(char const * sz);

  extern char const *  ltrim(char const * sz);                // Returns pointer to first non-ws char
  extern char const *  ltrim(char const * sz, char ch);       // Returns pointer to first non-ws char
  extern int           tok_len(char const * sz);              // Length of string sans ending ws
  extern int           tok_len(char const * sz, char ch);

  extern string        trim(string const & str);
  extern string        trim(string const & str, char ch);
  extern string        rtrim(string const & str);
  extern string        rtrim(string const & str, char ch);
  extern string        ltrim(string const & str);
  extern string        ltrim(string const & str, char ch);
  extern int           tok_len(string const & str);
  extern int           tok_len(string const & str, char ch);

  extern void          make_lower(string & str);              // In place
  extern string        _lower(string const & str);            // Returns new lower-case'd string
  extern void          make_upper(string & str);              // In place
  extern string        _upper(string const & str);            // Returns new upper-case'd string

  extern string        replace_chars(char const * str, char const * find, const char * repl);
  extern string        replace_chars(string const & str, char const * find, const char * repl);
  extern string        replace_chars(char const * str_begin, char const * str_end, char const * find, const char * repl);
  extern string        replace(string const & str, char const *find, string const & replace);

  extern char *        rtrim(char * sz);                      // Just moves the zero-terminating char -- somewhat dangerous

  extern string        dashify_key(string const & key);

  extern string &      _accumulate(string & str, string const & part, char const * sep);
  extern string &      _accumulate(string & str, char const * part, char const * sep);

  extern char const *  skip_ws(char const *p);
  extern char *        skip_ws(char *p);

  extern char const *  skip_ws(char const *&p, char const * end);
  extern char *        skip_ws(char *&p, char * end);

  extern string        skip_char(string const & str, char ch);
  extern char const *  skip_char(char const *p, char ch);
  extern char *        skip_char(char *p, char ch);

  extern char const *  skip_past_double_newline(char const *sz, char const * end);

  extern bool          _starts_with(string const & str, char const * sz_start);
  extern bool          _starts_with(char const * sz, char const * sz_start);

  extern bool          eq(char const * sz1, char const * sz2);
  extern bool          eq(string const & s1, char const * sz2);
  extern bool          is_ip_addr(char const * str);
  extern bool          is_num(char const * str);
  extern bool          is_num(string const & str);

  extern char const *  or_blank(uint8 const * p);

  extern string        random_string(size_t length);
  extern bool          _normalize_keys(string & parent, string & key);

  //-----------------------------------------------------------------------------
  //
  //  String list building and manipulation
  //
  //-----------------------------------------------------------------------------
  strlist split(string const & str, char sep = ',');
  strlist split(char const * sz, char sep = ',');
  strlist split(char const * sz, char const * sep, char const * end = NULL);
  strlist split(char const * sz, char const * sep, int max_num_splits, char const * end = NULL);
  strlist split_on_parens(string const & str, char left = '(', char right = ')');

  extern string parse_on(string & str, char const * sep, char const * end = NULL);

  extern strlist  compact(strlist const & that);

  // String vectors
  extern strvlist splitv(string const & str, char sep = ',');
  extern strvlist splitv(char const * sz, char sep = ',');
  extern strvlist splitv(char const * sz, char const * sep, char const * end = NULL);

  extern int      splitv(strvlist & result, string const & str, char sep = ',');
  extern int      splitv(strvlist & result, char const * sz, char sep = ',');

  extern strvlist compact(strvlist const & that);

  // String sets
  extern strset splits(string const & str, char sep = ',');
  extern strset splits(char const * sz, char sep = ',');
  extern strset splits(char const * sz, char const * sep, char const * end = NULL);
  extern strset setify(strvlist const & that);

  extern string  join(strlist const & list, char const * sep = ", ");

  extern strlist & append_to(strlist & to, strlist const & additional_items);
  extern void           dump(strlist const & list);

  //-----------------------------------------------------------------------------
  //
  //  String map building and manipulation
  //
  //-----------------------------------------------------------------------------
  extern bool   _has(strmap const & dict, char const * key);

  extern int    split_kv(strmap & result, string const & str, char sep1, char sep2, strmap * result_lc_out = NULL);
  extern bool   split_kv(strmap_entry & result, string const & str, char sep, string * other_out = NULL);
  extern bool   split_kv(strmap_entry & result, string const & str, char const * sep);
  extern bool   _extend(strmap & self, strmap const & that);

  extern string join(strmap const & that, char sep1, char sep2);

  extern strmap &   add_kv(strmap & result, string const & key, string const & value);
  extern strmap &   add_kv(strmap & result, char const * key, string const & value);
  extern strmap &   add_kv(strmap & result, string const & key, int32 value);
  extern strmap &   add_kv(strmap & result, char const * key, int32 value);

  extern string &   value(strmap::iterator it);
  extern string const &   value(strmap::const_iterator it);

  extern string     _lookup(strmap const & dict, string const & key);
  extern string     _lookup(strmap const & dict, string const & key, char const * def);
  extern int        _lookup(strmap const & dict, string const & key, int def);

  extern strlist &  add_kvs(strlist & in_out, strmap const & dict);

  extern void       dump(strmap const & dict);

  extern boolmap    true_map(strlist const & list);

  extern string     JSON_stringify(strmap const & dict, std::map<string, int> const * numbers = NULL, std::map<string, bool> const * bools = NULL);
  extern string     JSON_debug_string(strmap const & dict, strlist const & keys, strlist const & key_order, std::map<string, int> const * numbers = NULL, std::map<string, bool> const * bools = NULL);
  extern bool       JSON_parse(json_t & out, string const & json);
  extern bool       JSON_parse_array(json_array_t & out, string const & json);

  template <typename T>
  bool _has(std::map<string, T> const & dict, string const & key) {
    return dict.find(key) != dict.end();
  }

  template <typename T>
  T* _item(std::map<string, T*> & dict, string const & key) {
    typename std::map<string, T*>::const_iterator it = dict.find(key);
    if (it == dict.end()) { return NULL; }

    /* otherwise */
    return it->second;
  }

  template <typename T>
  int parse_kv(T begin, T end, strmap & dict, char sep = '=', strmap * dict_lc_out = NULL, strlist * others_out = NULL)
  {
    map<string, string> dict_lc;
    if (dict_lc_out == NULL) {
      dict_lc_out = &dict_lc;
    }

    deque<string> others_;
    if (others_out == NULL) {
      others_out = &others_;
    }

    int result = 0;

    for (T it = begin; it != end; ++it) {
      string str = *it;

      string other;
      strmap_entry kv;

      if (split_kv(kv, str, sep, &other)) {
        dict.insert(kv);
        result += 1;

        string key_lc = kv.first;
        std::transform(key_lc.begin(), key_lc.end(), key_lc.begin(), ::tolower);
        dict_lc_out->insert(make_pair(key_lc, kv.second));
      } else {
        others_out->push_back(other);
      }
    }

    return result;
  }

  //-----------------------------------------------------------------------------
  //
  //  Misc
  //
  //-----------------------------------------------------------------------------
  void    srandom(uint32 newseed);
  uint32  random(void);

  extern void log_d(map<string, string> const &, string pre = "", string post = "");
  extern char const *  find(char const *p, char ch);
  extern char const *  find(char const *p, char const * sz);

  extern void           dump(std::map<string, int> const & dict);

  template <typename Ta, typename Tb>
  float no_div_zero(Ta a, Tb b) {
    if ((float)b == 0.0) { return -1.0; }   // Yes, I know this isn't mathematically correct.

    return (float)a / (float)b;
  }

  template <typename T>
  T * pull(std::deque<T *> & list) {
    if (list.empty()) { return NULL; }

    /* otherwise */
    T * result = list.front();
    list.pop_front();
    return result;
  }

  template <typename T>
  T* value(typename map<string, T*>::const_iterator it)
  {
    return it->second;
  }

  template <typename K, typename V>
  V * _lookup(std::map<K, V*> & dict, K const & key)
  {
    typename std::map<K, V*>::iterator it = dict.find(key);
    if (it == dict.end()) {
      return NULL;
    }

    /* otherwise */
    return it->second;
  }

  template <typename V>
  V _lookup(std::map<std::string, V> const & dict, std::string const & key, V const & def)
  {
    typename std::map<std::string, V>::const_iterator it = dict.find(key);
    if (it == dict.end()) {
      return def;
    }

    /* otherwise */
    return it->second;
  }

  template <typename T>
  T _or(T a, T b)
  {
    if (a) { return a; }
    return b;
  }

#if 0
  template <typename V>
  V _lookup(std::map<std::string, V> const & dict, char const * key, V def)
  {
    typename std::map<std::string, V>::const_iterator it = dict.find(key);
    if (it == dict.end()) {
      return def;
    }

    /* otherwise */
    return it->second;
  }
#endif

  bool    _has_elapsed(uint32 & time, uint32 timeout, uint32 current_time);
  uint32  _time_since(uint32 time, uint32 start = 0);

  //-----------------------------------------------------------------------------
  //
  //  ARGS
  //
  //-----------------------------------------------------------------------------
  struct args_t
  {
    static string none;

    map<string, string> args;
    set<string>         flags;
    set<string>         antiFlags;

    args_t();

    template <typename T> args_t(int argc, T const *argv[])
      : no_more_args(false)
    {
      string arg, key, value;

      for (int i = 1; i < argc; ++i) {
        arg = platform_to_ascii_string(argv[i]);
        if (_parse(arg.c_str(), key, value)) {
          args.insert(make_pair(key, value));
        }
      }
    }

    args_t &       merge(args_t const & that);

    string const & operator[](char const * key);
    string const & get(char const * key, string const & def);
    bool           get_flag(char const * key);
    bool           get_flag(char const * key, bool def);

    args_t &       set_arg(char const * key, char const * value);
    args_t &       set_arg(char const * key, string const & value);
    args_t &       set_flag(char const *flag, bool value = true);
    args_t &       clear_flag(char const *flag);

  protected:
    bool no_more_args;
    bool _parse(char const * str, string & key, string & value);
    args_t & _insert(string key, string value);
    args_t & _insert_bool(string key, bool value = true);
  };

  // ----- Non-standard implementations -----
  int         mwp_atoi(char const * sz);
  int         mwp_atoi(string const & str);
  std::string mwp_itoa(int n);
  std::string mwp_itoa(int n, int length);
  std::string mwp_ftoa(float n);
  std::string mwp_dtoa(double n);

  //-----------------------------------------------------------------------------
  //
  //  Functional implementations
  //
  //-----------------------------------------------------------------------------
  template <typename T>
  bool _has(set<T> const & s, T const & item) {
    return s.find(item) != s.end();
  }

  template <typename T>
  int _union(set<T> const & a, set<T> const & b, set<T> & result) {

    if (&a != &result) {
      for (typename set<T>::const_iterator it = a.begin(); it != a.end(); ++it) {
        result.insert(*it);
      }
    }

    if (&b != &result) {
      for (typename set<T>::const_iterator it = b.begin(); it != b.end(); ++it) {
        result.insert(*it);
      }
    }

    return (int)result.size();
  }

  template <typename T>
  set<T> _union(set<T> const & a, set<T> const & b) {
    set<T> result;
    _union(a, b, result);
    return result;
  }

  template <typename T>
  int _intersection(set<T> const & a, set<T> const & b, set<T> & result) {

    result.clear();

    for (typename set<T>::const_iterator it = a.begin(); it != a.end(); ++it) {
      if (_has(b, *it)) {
        result.insert(*it);
      }
    }

    return (int)result.size();
  }

  template <typename T>
  set<T> _intersection(set<T> const & a, set<T> const & b) {
    set<T> result;
    _intersection(a, b, result);
    return result;
  }
};

#endif  // __MWP_UTILS_HPP__

