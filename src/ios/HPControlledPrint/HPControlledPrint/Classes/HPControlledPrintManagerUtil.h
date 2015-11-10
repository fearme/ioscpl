//
//  HPControlledPrintManagerUtil.h
//  HPControlledPrint
//
//  Created by Fredy on 11/5/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "HPServices.h"

#include "../Includes/mwp_secure_asset_printing_api.hpp"

@interface HPControlledPrintManagerUtil : NSObject

+ (HPServices *)parseDiscoveryData:(NSDictionary *)data secureAssetPrint:(net_mobilewebprint::secure_asset_printing_api_t *)sap caymanRootUrl:(NSString *)caymanRootUrl;

@end
