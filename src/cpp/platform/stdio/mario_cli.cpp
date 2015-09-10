
#include "mwp_controller.hpp"

#include <cstdlib>

using namespace net_mobilewebprint;

string & get_env_option(string & options, char const *name);

int main(int argc, char const * argv[])
{
  string options;

  controller_t controller;

  controller.set_argv(argc, argv);

  get_env_option(options, "HTTP_PROXY");
  printf("Hello, world %s\n", options.c_str());

  controller.set_options(options);

  controller.start();
}

void translate_key(string & out, string const & in)
{
  for (int i = 0; i < in.length(); ++i) {
    char ch = in[i];
    if (ch == '_') {
      out += "-";
    } else if (ch >= 'A' && ch <= 'Z') {
      out += ch - ('A' - 'a');
    }
  }
}

string & get_env_option(string & options, char const *name)
{
  char const * envVar = NULL;

  string key("--");

  translate_key(key, name);

  if ((envVar = getenv(name)) != NULL) {
    if (options.length() > 0) { options += " "; }

    options = options + key + "=" + envVar;
  }

  return options;
}

