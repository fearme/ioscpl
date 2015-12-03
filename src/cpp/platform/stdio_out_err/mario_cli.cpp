
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

static string undefined(HP_MWP_UNDEFINED);

string  ip, url, option_name;
bool    is_print_mode       = false;
bool    is_scan_mode        = false;
bool    have_printed        = false;

bool g_verbose  = false;
bool g_quiet    = true;
bool g_started  = false;
bool g_outJson  = false;

int callback(void * app_data, char const * message, int id, int32 transaction_id, uint8 const * p1, mwp_params const * params);
int hf_callback(void * app_data, char const * name, int loop_num, uint32 tick_count, uint8 const * p1);

char buffer[2048];

int main(int argc, char const * argv[])
{

  hp_mwp_register_handler("cli", NULL, callback);
  hp_mwp_register_hf_handler("cli", NULL, hf_callback);

  hp_mwp_set_flag("app_verbosity", true);
  hp_mwp_parse_cli(argc, (void const **)argv);
  g_quiet = false;

  if (undefined == hp_mwp_get_option_def("stackName", cundefined, buffer, sizeof(buffer))) {
    hp_mwp_set_option("stackName", "qa");
  }

  // Are we outputting as JSON?
  if (_lower(hp_mwp_get_option_def("outfmt", cundefined, buffer, sizeof(buffer))) == "json") {
    g_outJson = true;
  }

  // Are we printing?
  ip  = hp_mwp_get_option_def("ip",  cundefined, buffer, sizeof(buffer));
  url = hp_mwp_get_option_def("url", cundefined, buffer, sizeof(buffer));
  if (ip != undefined && url != undefined) {
    is_print_mode = true;
  }

  is_scan_mode = hp_mwp_get_flag_def("scan", false);

  option_name = _lower(hp_mwp_get_option_def("option", cundefined, buffer, sizeof(buffer)));
  if (option_name == undefined ) {
    option_name = "";
  }

  hp_mwp_start_ex(HP_MWP_START_SCANNING | HP_MWP_BLOCK_START_FN);
}


using namespace net_mobilewebprint;

typedef std::map<string, strmap> strstrmap;

strstrmap printers;

uint32 current_tick = 0;
uint32 exit_time    = 0;

int hf_callback(void * app_data, char const * name, int loop_num, uint32 tick_count, uint8 const * p1)
{
  current_tick = tick_count;

  if (exit_time != 0) {
    if (tick_count > exit_time) {
      exit(0);
    }
  }

  if (option_name.length() > 0) {
    printf("%s", hp_mwp_get_option_def(option_name.c_str(), cundefined, buffer, sizeof(buffer)));
    exit(0);
  }

  if (!is_print_mode) { return 0; }

  /* otherwise */
  if (!have_printed && _has(printers, ip) && printers[ip]["is_supported"] == "1") {
    printf("------------- Print it!\n");
    fflush(stdout);
    hp_mwp_send_job(url.c_str(), ip.c_str());
    have_printed = true;
  }
}

string to_json(strmap const & x)
{
  strlist result;
  for (strmap::const_iterator it = x.begin(); it != x.end(); ++it) {
    string const & key   = it->first;
    string const & value = it->second;

    result.push_back(formats("\"%s\" : \"%s\"", key, value));
  }

  string out = "{ ";
  return out + join(result) + " }";
}

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

  string s1(p1);
  string message(message_);
  if (message == "printer_attribute") {

    string ip_(p1);

    // Remember the printer list
    printers[s1][p2] = p3;
    if (g_outJson) {
      string json = to_json(printers[s1]);
      printf("%s\n", json.c_str());
      fflush(stdout);
    } else {
      if (string(p2) == "is_supported" && p3[0] == '1') {
        printf("---------- supported: %17s   %s\n", p1, _lookup(printers[s1], "name", "no-name").c_str());
        fflush(stdout);

        if (is_scan_mode && ip == ip_) {
          exit_time = current_tick + 100;
        }

      } else if (string(p2) == "name") {
        printf("%17s   %s\n", p1, p3);
        fflush(stdout);
      }
    }

    if (string(p2) == "is_supported" && p3[0] == '1') {
      if (is_scan_mode && ip == ip_) {
        exit_time = current_tick + 100;
      }
    }

  } else if (message == "print_progress") {

    string status(p5);
    if (status == "PRINTER_SCAN_DONE") {
      if (is_scan_mode && ip.length() == 0) {
        exit_time = current_tick + 2000;
      }
    } else {

      if (g_outJson) {
        printf("{ \"message\" : \"%s\", \"progress\" : %lf, \"p2\" : \"%s\", \"p1\" : \"%s\", \"p3\" : \"%s\", \"p4\" : \"%s\", \"p5\" : \"%s\", \"n1\":%d, \"n2\" : %d }\n", message_, (float)n1 / (float)n2, p2, p1, p3, p4, p5, n1, n2);
      } else {
        printf("%s (%4.2lf) %-25s      %-10s p3:%-10s p4:%s p5:%s n1:%d n2:%d\n", message_, (float)n1 / (float)n2, p2, p1, p3, p4, p5, n1, n2);
      }
      fflush(stdout);
      if (s1 == "SUCCESS") {
        exit_time = current_tick + 100;
      }
    }

  } else if (message == "begin_printer_changes") {
  } else if (message == "end_printer_enum") {
  } else if (message == "recd_register_hf_handler") {
  } else if (message == "recd_register_handler") {

    g_started = true;

  } else {
//    if (get_flag_def("quiet", g_quiet)) { return 0; }

    /* otherwise */
    printf("cb: %s id:%d, tid:%d p1:%s p2:%s p3:%s p4:%s p5:%s n1:%d n2:%d n3:%d n4:%d n5:%d params:%p\n", message_, id, (int)transaction_id, p1, p2, p3, p4, p5, n1, n2, n3, n4, n5, params);
    fflush(stdout);
  }
  return 1;
}

void net_mobilewebprint::log_d(char const * msg, log_param_t x)
{
  if (!g_started) { return; }

  // Debug is like log-level 1
  if (hp_mwp_get_int_option_def("v_log_level", 0) <= 0) { return; }

  /* otherwise */
  net_mobilewebprint::log_d(msg, "MobileWebPrint", x);
}

void net_mobilewebprint::log_d(char const * msg, char const * tag, log_param_t)
{
  if (get_flag_def("quiet", g_quiet)) { return; }

  printf("D: %s\n", msg);
  fflush(stdout);
}

void net_mobilewebprint::log_w(char const * msg, log_param_t)
{
  if (!get_flag("no_warn")) { return; }

  printf("W: %s\n", msg);
  fflush(stdout);
}

void net_mobilewebprint::log_v(char const * msg, log_param_t x)
{
  printf("%s\n", msg);
  fflush(stdout);
  //net_mobilewebprint::log_v(msg, "MobileWebPrint", x);
}

void net_mobilewebprint::log_v(char const * msg, char const * tag, log_param_t)
{
  if (!get_flag("verbose"))                   { return; }

  printf("V: %s\n", msg);
  fflush(stdout);
  //net_mobilewebprint::log_d(msg);
}

void net_mobilewebprint::log_e(char const * msg, log_param_t)
{
  printf("E: %s\n", msg);
  fflush(stdout);
  //net_mobilewebprint::log_d(msg);
}


