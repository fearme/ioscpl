
#include "mwp_controller.hpp"
#include "mwp_host.hpp"

net_mobilewebprint::controller_t::controller_t()
  : mdns(*this),
    printers(*this)
{
}

void net_mobilewebprint::controller_t::start(bool should_block)
{
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

