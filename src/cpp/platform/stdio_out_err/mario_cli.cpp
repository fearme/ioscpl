
#include "mwp_types.hpp"
#include "mwp_core_api.hpp"
#include "mwp_controller.hpp"
#include "mwp_utils.hpp"

#include <string>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

using std::string;

using namespace net_mobilewebprint;

bool g_verbose = false;

int callback(void * app_data, char const * message, int id, int32 transaction_id, uint8 const * p1, mwp_params const * params);

int main(int argc, char const * argv[])
{
  hp_mwp_register_handler("cli", NULL, callback);
  hp_mwp_parse_cli(argc, (void const **)argv);

  hp_mwp_set_option("http_proxy_name", "proxy.atlanta.hp.com");
  hp_mwp_set_option("http_proxy_port", "8080");

  hp_mwp_set_flag("log_api", true);

  printf("Logging: D:%d W:%d V:%d Vl:%s\n", (int)!get_flag("quiet"), (int)!get_flag("no_warn"), (int)get_flag("verbose"), get_option("v_log_level").c_str());

  hp_mwp_start_ex(HP_MWP_START_SCANNING | HP_MWP_BLOCK_START_FN);
}


using namespace net_mobilewebprint;

int callback(void * app_data, char const * message_, int id, int32 transaction_id, uint8 const * p1_, mwp_params const * params)
{
  char const * p1 = (p1_ != NULL ? (char const *)p1_ : "");
  char const * p2 = (params != NULL ? params->p2 != NULL ? (char const *)params->p2 : "" : "");
  char const * p3 = (params != NULL ? params->p3 != NULL ? (char const *)params->p3 : "" : "");
  char const * p4 = (params != NULL ? params->p4 != NULL ? (char const *)params->p4 : "" : "");
  char const * p5 = (params != NULL ? params->p5 != NULL ? (char const *)params->p5 : "" : "");
  int n1 = (params != NULL ? params->n1 : 0);
  int n2 = (params != NULL ? params->n2 : 0);
  int n3 = (params != NULL ? params->n3 : 0);
  int n4 = (params != NULL ? params->n4 : 0);
  int n5 = (params != NULL ? params->n5 : 0);

  string message(message_);
  if (message == "printer_attribute") {
    printf("%s: %s = %s\n", p1, p2, p3);
  } else if (message == "begin_printer_changes") {
  } else if (message == "end_printer_enum") {
  } else {
    printf("cb: %s id:%d, tid:%d p1:%s p2:%s p3:%s p4:%s p5:%s n1:%d n2:%d n3:%d n4:%d n5:%d params:%x\n", message_, id, (int)transaction_id, p1, p2, p3, p4, p5, n1, n2, n3, n4, n5, params);
  }
  return 1;
}

void net_mobilewebprint::log_d(char const * msg, log_param_t x)
{
  net_mobilewebprint::log_d(msg, "MobileWebPrint", x);
}

void net_mobilewebprint::log_d(char const * msg, char const * tag, log_param_t)
{
  if (get_flag("quiet")) { return; }

//  printf("D: %s\n", msg);
}

void net_mobilewebprint::log_w(char const * msg, log_param_t)
{
  if (!get_flag("no_warn")) { return; }

  printf("W: %s\n", msg);
}

void net_mobilewebprint::log_v(char const * msg, log_param_t x)
{
  net_mobilewebprint::log_v(msg, "MobileWebPrint", x);
}

void net_mobilewebprint::log_v(char const * msg, char const * tag, log_param_t)
{
  if (!get_flag("verbose")) { return; }

  printf("V: %s\n", msg);
  //net_mobilewebprint::log_d(msg);
}

void net_mobilewebprint::log_e(char const * msg, log_param_t)
{
  printf("E: %s\n", msg);
  //net_mobilewebprint::log_d(msg);
}
