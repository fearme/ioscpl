
#include "mwp_controller.hpp"
#include "mwp_host.hpp"

net_mobilewebprint::controller_t::controller_t()
  : mdns(*this)
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

