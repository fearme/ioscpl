
#include "mwp_types.hpp"
#include "mwp_controller.hpp"

using namespace net_mobilewebprint;

#define APP_TYPE sap_params

void net_mobilewebprint::log_d(char const * msg, log_param_t x)
{
  net_mobilewebprint::log_d(msg, "MobileWebPrint", x);
}

void net_mobilewebprint::log_d(char const * msg, char const * tag, log_param_t)
{
  if (get_flag("quiet")) { return; }

  send_to_app("log_d", -1, 0, (byte const *)msg, (APP_TYPE const *)NULL);
}

void net_mobilewebprint::log_w(char const * msg, log_param_t)
{
  if (get_flag("no_warn")) { return; }

  send_to_app("log_w", -1, 0, (byte const *)msg, (APP_TYPE const *)NULL);
}

void net_mobilewebprint::log_v(char const * msg, log_param_t x)
{
  net_mobilewebprint::log_v(msg, "MobileWebPrint", x);
}

void net_mobilewebprint::log_v(char const * msg, char const * tag, log_param_t)
{
  if (!get_flag("verbose")) { return; }

  send_to_app("log_v", -1, 0, (byte const *)msg, (APP_TYPE const *)NULL);
}

void net_mobilewebprint::log_e(char const * msg, log_param_t)
{
  send_to_app("log_e", -1, 0, (byte const *)msg, (APP_TYPE const *)NULL);
}

