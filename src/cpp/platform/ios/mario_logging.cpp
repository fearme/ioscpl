
#include "mwp_types.hpp"
#include "mwp_controller.hpp"

using namespace net_mobilewebprint;

#define APP_TYPE sap_params

int darwin_log(char type, char const * tag, char const * message);

void net_mobilewebprint::log_d(char const * msg, log_param_t x)
{
  net_mobilewebprint::log_d(msg, "MobileWebPrint", x);
}

void net_mobilewebprint::log_d(char const * msg, char const * tag, log_param_t)
{
  if (get_flag("quiet")) { return; }

  darwin_log('d', tag, msg);
}

void net_mobilewebprint::log_w(char const * msg, log_param_t)
{
  if (get_flag("no_warn")) { return; }

  darwin_log('w', "", msg);
}

void net_mobilewebprint::log_v(char const * msg, log_param_t x)
{
  net_mobilewebprint::log_v(msg, "MobileWebPrint", x);
}

void net_mobilewebprint::log_v(char const * msg, char const * tag, log_param_t)
{
  if (!get_flag("verbose")) { return; }

  darwin_log('v', tag, msg);
}

void net_mobilewebprint::log_e(char const * msg, log_param_t)
{
  darwin_log('e', "", msg);
}

