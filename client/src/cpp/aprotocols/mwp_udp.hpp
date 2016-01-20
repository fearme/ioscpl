
#ifndef __MWP_UDP_HPP__
#define __MWP_UDP_HPP__

#include "mwp_types.hpp"
#include <vector>
#include <stdlib.h>

namespace net_mobilewebprint {

  using std::string;
  using std::vector;

	/**
	 *	Send UDP packets and get resulting packets back.
	 *
	 *  UDP is a non-guaranteed protocol.  You send a packet and *hope* the destination gets it;
	 *  you then *hope* to get the reply.  Ugh.  When the network gets congested (particularly on 
	 *  WiFi) UDP packets are dropped.  There are a couple of techniques to combat this.
	 *
	 *  "Blasting" a lot of identical and redundant packets is easy and effective, so udp_t provides
	 *  a blast() function.  However, you can only use it when the destination doesn't care about the
	 *  redundant packets.  For example, asking for a Canon printer's model will return the same data
	 *  each time, and won't break the internal state of the printer.  However, sending a "start job"
	 *  UDP packet isn't safe to do multiple times.
	 *
	 *  While blasting, udp_t is smart enough to give a little extra time for the very first packet,
	 *  as that one will be responded-to in the vast majority of cases.  Udp_t then sends many others 
	 *  very quickly if the first one takes too long.  Udp_t stops blasting when it gets a response.
	 *
	 *  "Semi-blasting" is a lot like blasting.  It takes a lot longer to send redundant packets, but is
	 *  still semi-agressive.  It is for when all the responses would be the same, but when the 
	 *  destination will still exert effort to compute responses or may leak.  For example, SNMP status.
	 *
	 *  "Send" (i.e. Slow) is for other the other cases.  Udp_t will be "sure" that the response isn't coming 
	 *  before re-sending.
	 *
	 */
  struct udp2_t {

    udp2_t(int blast_width = 3, int blast_height = 3);

		bool      blast(buffer_t & result, network_node_t & dest, buffer_t & packet);
		bool semi_blast(buffer_t & result, network_node_t & dest, buffer_t & packet);
		bool       send(buffer_t & result, network_node_t & dest, buffer_t & packet);

	protected:
		bool blast_(buffer_t & result, network_node_t & dest, buffer_t & packet, struct timeval & timeoutA, struct timeval & timeoutB, int max_width = -1, int max_height = -1);

    vector<int>         fds;
    int                 fd_max;
		int                 m_blast_width;
	  int									m_blast_height;
    struct timeval      short_timeout;
    struct timeval      long_timeout;
    struct timeval      zero_timeout;

    struct sockaddr_in  myaddr;

    byte *              trash;

    void _clear_reads(network_node_t & network_node);
    void _add_socket();

  };

  struct udp_t {
    int                 fd;
    struct sockaddr_in  myaddr;

    udp_t();
  };
};

#endif

