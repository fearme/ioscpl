
#include "mwp_controller.hpp"
#include "mwp_host.hpp"

net_mobilewebprint::controller_t::controller_t()
  : mdns(*this),
    printers(*this)
{
}

void net_mobilewebprint::controller_t::set_option(string const & key, string const & value)
{
  options.insert(make_pair(key, value));
}

void net_mobilewebprint::controller_t::set_options(string const & str)
{
  set_options(str.c_str());
}

void net_mobilewebprint::controller_t::set_options(char const * sz)
{
  parse_args(sz, options, flags);
}

void net_mobilewebprint::controller_t::set_argv(int argc, char const *argv[], int start)
{
  string args;

  for (int i = start; i < argc; ++i) {
    string arg = argv[i];
    if (arg == "--") {
      break;
    }

    /* otherwise */
    _accumulate(args, arg, " ");
  }

  set_options(args);
}


void net_mobilewebprint::controller_t::start(bool should_block)
{
  for (strmap::const_iterator it = options.begin(); it != options.end(); ++it) {
    string const & key = it->first;
    string const & val = it->second;

    printf("option(%s): |%s|\n", key.c_str(), val.c_str());
  }

  for (strset::const_iterator it = flags.begin(); it != flags.end(); ++it) {
    string const & flag = *it;

    printf("flag: |%s|\n", flag.c_str());
  }

  mq.run();

  // Start the scan
  mq.send("scan_for_printers");

  if (!should_block) {
    return;
  }

  /* otherwise -- wait for MQ to finish */
  for (int i = 0; i < 5 && !mq.is_done(); ++i) {
    printf("%d\n", i);
    host::interruptable_sleep(1000);
  }
}

void net_mobilewebprint::controller_t::get_attributes(printer_t & printer, strset keys)
{
  for (strset::const_iterator it = keys.begin(); it != keys.end(); ++it) {
    get_attributes(printer, *it);
  }
}

void net_mobilewebprint::controller_t::get_attributes(printer_t & printer, string const & key)
{
  if (key == "ip") {
    // TODO: if printer has mdns_name, send the right request to get the IP
  }
}

