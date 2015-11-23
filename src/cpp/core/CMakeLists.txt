
# TODO: This should be removed, right?
#set(MARIO_PLATFORM ANDROID)

include(CheckIncludeFiles)

check_include_files("netinet/if_ether.h" HAVE_IF_ETHER_H)
check_include_files("net/if_arp.h"       HAVE_IF_ARP_H)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mwp_core_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/mwp_core_config.h)

# Build the MWP interface file
set(MWP_INTERFACE_NAME mwp)
set(MWP_INTERFACE_NAME_UC MWP)

configure_file(
  "${PROJECT_SOURCE_DIR}/core/hp_mwp.h.in"
  "${PROJECT_BINARY_DIR}/core/hp_${MWP_INTERFACE_NAME}.h"
  )
include_directories("${PROJECT_BINARY_DIR}/core")

# Build the SAP interface file
set(MWP_INTERFACE_NAME sap)
set(MWP_INTERFACE_NAME_UC SAP)

configure_file(
  "${PROJECT_SOURCE_DIR}/core/hp_mwp.h.in"
  "${PROJECT_BINARY_DIR}/core/hp_${MWP_INTERFACE_NAME}.h"
  )

# Make libmwpcore
add_library(mwpcore
                       hp_mwp.cpp
                     mwp_upstream.cpp
                     mwp_stats.cpp
                     mwp_mini_curl.cpp
                     mwp_controller.cpp
                     mwp_core_api.cpp
                     mwp_mq.cpp
                     mwp_printer.cpp
                     mwp_secure_asset_printing_api.cpp
                     mwp_types.cpp
                     mwp_utils.cpp)


