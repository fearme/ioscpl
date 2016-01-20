
#include "mwp_types.hpp"

extern void darwin_platform_bootstrap();

void net_mobilewebprint::platform_bootstrap()
{
  darwin_platform_bootstrap();
}

