// mario.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "hp_mwp.h"
#include "mwp_utils.hpp"
#include <map>
#include <iostream>

#include "mwp_controller.hpp"

//#define TEST_PRINTER_IP "192.168.11.127"
#define TEST_PRINTER_IP "15.80.125.53"

using std::cout;
using std::endl;

using namespace net_mobilewebprint;

void dump_printers();
int mwp_app_callback(void * app_data, char const * msg, int ident, int32 transaction_id, uint8 const * p1, mwp_params const * params);

using std::map;
using std::string;

map<string, map<string, string> > printers;

extern bool g_silent;

int _tmain(int argc, _TCHAR const * argv[])
{

  hp_mwp_parse_cli(argc, (void const **)argv);
  hp_mwp_register_handler("app", NULL, mwp_app_callback);
  hp_mwp_set_flag("quiet", true);
//  hp_mwp_set_flag("log_packet_data", true);
  hp_mwp_set_flag("log_bjnp", true);

  // Set these as desired, or leave commented out for normal behavior
  //hp_mwp_set_flag("verbose", true);
  hp_mwp_set_flag("quiet", false);
  hp_mwp_set_flag("fast-fail", true);
  //hp_mwp_set_option("log_tags", "bjnp:mdns");

  hp_mwp_set_option("http_proxy_name", "proxy.atlanta.hp.com");
  hp_mwp_set_option("http_proxy_port", "8080");

  hp_mwp_set_flag("log_api", true);

  hp_mwp_start_ex(HP_MWP_START_SCANNING);

  hp_mwp_set_timeout("afterMwpStart", 500);

  string ip;
  ip = "10.7.5.26";     // Canon MG5520
//  ip = "10.7.5.35";     // Whitney

  // Run normally for 4 seconds
  map<string, map<string, string> >::const_iterator it;
  for (int i = 0; i < 4000; ++i) {
//      dump_printers();
      if ((it = printers.find(ip)) != printers.end()) {
      map<string, string> const & printer = it->second;
      if (printer.find("1284_device_id") != printer.end()) {
        break;
      }
    }

    interruptable_sleep(1000);
  }

  g_silent = true;

#if 0
  printf("Enter an IP: ");
  std::cin >> ip;
#endif

  g_silent = false;
  dump_printers();

  printf("Printing to %s in 5sec\n", ip.c_str());
  interruptable_sleep(5000);

  printf("Printing to %s\n", ip.c_str());
  hp_mwp_send_job("http://qa1.hpsavingscenter.com/filestore/files/11911/data", ip.c_str());

  while (!hp_mwp_mq_is_done()) {
    interruptable_sleep(1000);
  }

  return 0;
}

void dump_printers()
{
//  return;
  if (g_silent) { return; }

  map<string, map<string, string> >::const_iterator it_printer;
  map<string, string>::const_iterator it_attrs;

  printf("---------------------------------------------------\n");
  for (it_printer = printers.begin(); it_printer != printers.end(); ++it_printer) {
    string const & ip = it_printer->first;
    map<string, string> const & printer = it_printer->second;

    printf("%s\n", ip.c_str());
    for (it_attrs = printer.begin(); it_attrs != printer.end(); ++it_attrs) {
      string const & key   = it_attrs->first;
      string const & value = it_attrs->second;

      printf("    %20s: %s\n", key.c_str(), value.c_str());
    }
  }
  printf("---------------------------------------------------\n");
}

int mwp_app_callback(void * app_data, char const * message, int ident, int32 transaction_id, uint8 const * p1, mwp_params const * params)
{
  char const * ip = (char const *)p1;
  char const * p2 = params ? params->p2 ? (char const *)params->p2 : "" : "";
  char const * p3 = params ? params->p3 ? (char const *)params->p3 : "" : "";

  // A new (full) printer list
  if (strcmp(message, HP_MWP_BEGIN_NEW_PRINTER_LIST_MSG) == 0) {
    printers = map<string, map<string, string> >();

    // A single printer attribute changed
  } else if (strcmp(message, HP_MWP_PRINTER_ATTRIBUTE_MSG) == 0) {
    //printf("printer attribute!!: {%s}: {%s}:{%s}\n", ip, p2, p3);

    printers[ip][p2] = p3;

    // A single printer attribute was removed
  } else if (strcmp(message, HP_MWP_RM_PRINTER_ATTRIBUTE_MSG) == 0) {
    //printf("Remove attribute: {%s}: %s, \n", p1 ? (char const *)p1 : "", p2);

    printers[ip].erase(p2);

    // MWP is done enumerating printer attributes
  } else if (strcmp(message, HP_MWP_END_PRINTER_ENUM_MSG) == 0) {
    //printf("Done with the printer\n");
//    dump_printers();

  } else if (strcmp(message, "afterMwpStart") == 0) {
//    dump_printers();
    hp_mwp_set_timeout("afterMwpStart", 500);
  }
  return 1;
}

