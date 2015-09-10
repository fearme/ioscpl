
// vim: filetype=ragel:

// A file to parse URIs
//

#include "mwp_utils.hpp"

using namespace net_mobilewebprint;
using std::string;

void show_token(char const * ts, char const * te, char const * msg);

%%{

  machine uri_parser;

  protocol = ( "http" | "https" );
  user     = [^:]+ ":" [^@]+;
  host     = [^:/]+;
  port     = ( ":" [1-9] [0-9]* )?;
  path     = "/" [^?#]*;

  main := |*

    protocol {
      show_token(ts, te, "protocol");
      protocol = string(ts, te);
    };

    "://" {
      //show_token(ts, te, "colon-slash-slash");
    };

    ( user "@" )? {
      show_token(ts, te, "user");
      user = string(ts, te);
    };

    host {
      show_token(ts, te, "host");
      host = string(ts, te);
    };

    port {
      show_token(ts + 1, te, "port");
      port = string(ts + 1, te);
    };

    path {
      show_token(ts, te, "path");
      path = string(ts, te);
    };

  *|;

}%%

%% write data;

bool net_mobilewebprint::parse_uri(char const * uri, string & protocol, string & user, string & host, string & port, string & path, strmap & query, string & fragment)
{
  string key, value;

  int          cs  = 0;
  int          act = 0;
  char const * p   = uri;
  char const * pe  = p + strlen(uri);
  char const * eof = pe;
  char const * ts = NULL;
  char const * te = NULL;

  %% write init;
  %% write exec;

  //printf("Parse %d finish %d, first_final %d\n", cs, %%{write error;}%%, %%{write first_final;}%%);
  if (cs == %%{write error;}%%) {
    printf("Parse error\n");
    return false;
  }
  if (cs < %%{write first_final;}%%) {
    printf("Not in final state\n");
    return false;
  }

  return protocol.length() > 0 && host.length() > 0 && path.length() > 0;
}

void show_token(char const * ts, char const * te, char const * msg)
{
  string token(ts, te);
  printf("%s: |%s|\n", msg, token.c_str());
}

