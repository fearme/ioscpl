
#ifndef __MWP_SOCKET_HPP__
#define __MWP_SOCKET_HPP__

#include "mwp_buffer.hpp"
#include "mwp_types.hpp"
#include <string>

#ifdef WIN32

#include <Winsock2.h>
#include <Ws2tcpip.>

#else

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#endif

namespace net_mobilewebprint {

  using std::string;

  struct socket_t
  {
    int                 fd;
    int                 last_error;
    struct sockaddr_in  remote;

    string              ip;
    uint16              port;

    socket_t(char const * name, uint16 port);

    int set_sockopt(int option_name, int    value);
    int set_sockopt(int option_name, void * value);

    void _init();
  };

  struct udp_socket_t : public socket_t
  {
    udp_socket_t(char const * name, uint16 port);

    int send_to(buffer_t & packet);
    int recv_from(buffer_t & result, string & sender_ip, uint16 & sender_port, int size_hint = 2 * 1024);
  };


};

#endif // __MWP_SOCKET_HPP__

