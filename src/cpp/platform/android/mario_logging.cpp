
#include "mwp_types.hpp"
#include "mwp_controller.hpp"
#include <android/log.h>

using namespace net_mobilewebprint;

#define APP_TYPE mwp_params

void net_mobilewebprint::log_d(char const * msg, log_param_t x)
{
  net_mobilewebprint::log_d(msg, "MobileWebPrint", x);
}

void net_mobilewebprint::log_d(char const * msg, char const * tag, log_param_t)
{
  if (get_flag("quiet")) { return; }

  __android_log_write(ANDROID_LOG_DEBUG, tag, msg);
}

void net_mobilewebprint::log_w(char const * msg, log_param_t)
{
  if (get_flag("no_warn")) { return; }

  __android_log_write(ANDROID_LOG_WARN, "MobileWebPrint", msg);
}

void net_mobilewebprint::log_v(char const * msg, log_param_t x)
{
  net_mobilewebprint::log_v(msg, "MobileWebPrint", x);
}

void net_mobilewebprint::log_v(char const * msg, char const * tag, log_param_t)
{
  if (!get_flag("verbose")) { return; }

  __android_log_write(ANDROID_LOG_VERBOSE, tag, msg);
}

void net_mobilewebprint::log_e(char const * msg, log_param_t)
{
  __android_log_write(ANDROID_LOG_ERROR, "MobileWebPrint", msg);
}


