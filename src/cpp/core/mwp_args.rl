
// vim: filetype=ragel:

// A file to parse command-line parameters
//
//   --do-it-all=55
//
//      params["do_it_all"] = "55"

#include "mwp_utils.hpp"

using namespace net_mobilewebprint;
using std::string;

void commit(string & key, string & value, strmap & dict, strset & flags);

%%{

  machine ma;

  action ddash    {              /*printf("ddash: %c\n", fc);*/ }
  action letters  { key += fc;   /*printf("lett: %c\n", fc);*/ }
  action eq       {              /*printf("eq:   %c\n", fc);*/ }
  action val      { value += fc; /*printf("val:  %c\n", fc);*/ }

  action commit   { commit(key, value, dict, flags); }

  param = (
    '--' @ddash [a-zA-Z0-9]+ $letters ( '-' [a-zA-Z90-9]+ )* $letters ( '=' @eq [^ ]+ $val )?
  ) %commit;

  main := param ( ' ' param )*;

}%%

%% write data;

bool net_mobilewebprint::parse_args(char const * args, strmap & dict, strset & flags)
{
  string key, value;

  int cs = 0;
  char const * p   = args;
  char const * pe  = p + strlen(args);
  char const * eof = pe;

  %% write init;
  %% write exec;

  int final_cs = %%{write error;}%%;
  if (cs == %%{write error;}%%) {
    return false;
    //printf("Parse error %d\n", final_cs);
  }
  if (cs < %%{write first_final;}%%) {
    return false;
    //printf("Not in final state\n");
  }

  return true;
}

string sanitize(string const & str)
{
  string result;

  for (int i = 0; i < str.length(); ++i) {
    char ch = str[i];
    if (ch == '-') {
      ch = '_';
    } else if (ch >= 'A' && ch <= 'Z') {
      ch -= ('A' - 'a');
    }

    result += ch;
  }

  return result;
}

void commit(string & key_, string & value, strmap & dict, strset & flags)
{
  string key = sanitize(key_);

  if (value.length() > 0) {
    dict.insert(make_pair(key, value));
    //printf("dict-entry: %s: %s\n", key.c_str(), value.c_str());
  } else {
    flags.insert(key);
    //printf("flag: %s\n", key.c_str());
  }

  key_ = value = "";
}
