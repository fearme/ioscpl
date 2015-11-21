
#import <UIKit/UIKit.h>
#include "hp_mwp.h"

int mwp_interop_bootstrap(void * app_data, char const * message, int id, int32 transaction_id, uint8 const * p1, mwp_params const * params)
{
  printf("-------------------------------------------- here i am in the interop\n");
//  int zero = 1;
//  zero -= 1;
//  int j = 10 / zero;

  // TODO: Generate clientID
  return 0;
}

//bool registered_bootstrap = hp_mwp_register_bootstrap("ios_interop", NULL, mwp_interop_bootstrap);
//bool registered_handler = hp_mwp_register_handler("ios_interop", NULL, mwp_interop_bootstrap);

