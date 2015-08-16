
#include "mwp_socket.hpp"
#include "mwp_host.hpp"
#include "mwp_assert.hpp"
#include <string>

using namespace net_mobilewebprint;
using namespace host;

// TODO: Implement this
void mwp_socket_close(int){}

static int        __level(int option_name);
static socklen_t __length(int option_name);

//---------------------------------------------------------------------------------------------------
//------------------------------------- socket_t ----------------------------------------------------
//---------------------------------------------------------------------------------------------------
net_mobilewebprint::socket_t::socket_t(char const * name, uint16 port_)
  : fd(0), ip(name), port(port_)
{
  _init();
}

void net_mobilewebprint::socket_t::_init()
{
  memset(&remote, 0, sizeof(remote));

  remote.sin_family = AF_INET;
  remote.sin_port   = htons(port);

  // TODO: Lookup name if not an ip address, like the old feature/cpp code does

  inet_pton(AF_INET, ip.c_str(), &remote.sin_addr);
}

int net_mobilewebprint::socket_t::set_sockopt(int option_name, int value)
{
  mwp_assert(option_name != IP_MULTICAST_IF, "IP_MULTICAST_IF not supported yet");

  int         level       = __level(option_name);
  socklen_t   option_len  = __length(option_name);
  void *      pvalue      = &value;

  byte        byvalue     = (byte)value;
  uint16      shvalue     = (uint16)value;

  mwp_assert(option_len <= 4, "The wrong set_sockopt was called");    // Did you want the one that takes a void* value?

  switch (option_len) {
    case 1:     pvalue = &byvalue;    break;
    case 2:     pvalue = &shvalue;    break;
  }

  int result = setsockopt(fd, level, option_name, pvalue, option_len);

  mwp_assert(result >= 0, "set_sockopt fail");

  return result;
}

int net_mobilewebprint::socket_t::set_sockopt(int option_name, void * pvalue)
{
  mwp_assert(option_name != IP_MULTICAST_IF, "IP_MULTICAST_IF not supported yet");

  int         level       = __level(option_name);
  socklen_t   option_len  = __length(option_name);

  int result = setsockopt(fd, level, option_name, pvalue, option_len);

  mwp_assert(result >= 0, "set_sockopt fail");

  return result;
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- udp_socket_t ------------------------------------------------
//---------------------------------------------------------------------------------------------------
net_mobilewebprint::udp_socket_t::udp_socket_t(char const * name, uint16 port)
  : socket_t(name, port)
{
  struct sockaddr_in myaddr;
  memset(&myaddr, 0, sizeof(myaddr));

  myaddr.sin_family       = AF_INET;
  myaddr.sin_addr.s_addr  = htonl(INADDR_ANY);
  myaddr.sin_port         = htons(0);

  int bind_result = -1;
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    last_error = get_last_network_error();
  } else {
    if ((bind_result = bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr))) < 0) {
      mwp_socket_close(fd);
      last_error = get_last_network_error();
    }
  }

  mwp_assert(fd >= 0, "udp_t fail to open socket");
  mwp_assert(bind_result >= 0, "udp_t fail to bind");
}

int net_mobilewebprint::udp_socket_t::send_to(buffer_t & packet)
{
  socklen_t addrlen = sizeof(remote);
  int result = (int)sendto(fd, packet.bytes, packet.data_length, 0, (struct sockaddr*)&remote, addrlen);
  last_error = get_last_network_error();
  return result;
}

int net_mobilewebprint::udp_socket_t::recv_from(buffer_t & result_buffer, string & sender_ip, uint16 & sender_port, int size_hint)
{
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);

  memset(&addr, 0, sizeof(addr));

  int orig_size = (int)result_buffer.data_length;
  result_buffer.resize_by(size_hint);

  int     result    = 0;
  int     buf_size  = result_buffer.mem_length - orig_size;
  char *  buffer    = (char*)result_buffer.bytes + orig_size;

  result_buffer.data_length += (result = (int)recvfrom(fd, buffer, buf_size, 0, (struct sockaddr*)&addr, &addrlen));
  last_error = get_last_network_error();

  char ip_[INET_ADDRSTRLEN + 1];

  sender_ip = string(inet_ntop(AF_INET, &addr.sin_addr, ip_, INET_ADDRSTRLEN));
  port = ntohs(addr.sin_port);

  return result;
}

//---------------------------------------------------------------------------------------------------
//------------------------------------- helpers -----------------------------------------------------
//---------------------------------------------------------------------------------------------------
static int __level(int option_name)
{
  int level = 0;

  switch (option_name) {
    case SO_BROADCAST:        level      = SOL_SOCKET;       break;
    case SO_KEEPALIVE:        level      = SOL_SOCKET;       break;

    case IP_ADD_MEMBERSHIP:   level      = IPPROTO_IP;       break;
    case IP_TTL:              level      = IPPROTO_IP;       break;
    case IP_MULTICAST_TTL:    level      = IPPROTO_IP;       break;

    default:
      mwp_assert(false, "option not supported in set_sockopt");
      break;

    //case IP_MULTICAST_IF:     level      = IPPROTO_IP;       break;
  }

  return level;
}

static socklen_t __length(int option_name)
{
  socklen_t option_len = 0;

  switch (option_name) {

    case IP_MULTICAST_TTL:    option_len = sizeof(u_char);   break;

    case SO_BROADCAST:        option_len = sizeof(int);      break;
    case SO_KEEPALIVE:        option_len = sizeof(int);      break;
    case IP_TTL:              option_len = sizeof(int);      break;

    case IP_ADD_MEMBERSHIP:   option_len = sizeof(ip_mreq);  break;

    default:
      mwp_assert(false, "option not supported in set_sockopt");
      break;

    //case IP_MULTICAST_IF:     level      = IPPROTO_IP;       break;
  }

  return option_len;
}


