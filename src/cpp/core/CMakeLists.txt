
include(CheckIncludeFiles)

check_include_files("netinet/if_ether.h" HAVE_IF_ETHER_H)
check_include_files("net/if_arp.h"       HAVE_IF_ARP_H)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mwp_core_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/mwp_core_config.h)

add_library(mwpcore
                mwp_controller.cpp
                mwp_mq.cpp
                mwp_buffer.cpp
                mwp_assert.cpp
                mwp_utils.cpp
                mwp_socket.cpp
                mwp_printer.cpp
                mwp_json.cpp
                )
