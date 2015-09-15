
#include "mwp_udp.hpp"

#define TRASH_SIZE  2048

net_mobilewebprint::udp_t::udp_t()
  : fd(0)
{
  memset(&myaddr, 0, sizeof(myaddr));

  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(0);

}



















static void zero(fd_set & A, fd_set & B, fd_set & C);
static void set_(int & max, fd_set & A, int fdA, fd_set * B = NULL, int fdB = 0, fd_set * C = NULL, int fdC = 0);

net_mobilewebprint::udp2_t::udp2_t(int blast_width, int blast_height)
	: fd_max(-1), m_blast_width(blast_width), m_blast_height(blast_height), trash(NULL)
{

  memset(&myaddr, 0, sizeof(myaddr));

  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(0);

  short_timeout.tv_sec = 1;
  short_timeout.tv_usec = 0;

  long_timeout.tv_sec = 4;
  long_timeout.tv_usec = 0;

  zero_timeout.tv_sec = 0;
  zero_timeout.tv_usec = 0;

  trash = new byte[TRASH_SIZE];

	for (int i = 0; i < m_blast_width; ++i) {
    _add_socket();
  }

}

bool net_mobilewebprint::udp2_t::blast(buffer_t & result, network_node_t & network_node, buffer_t & packet) {
  return net_mobilewebprint::udp2_t::blast_(result, network_node, packet, long_timeout, short_timeout);
}

bool net_mobilewebprint::udp2_t::semi_blast(buffer_t & result, network_node_t & network_node, buffer_t & packet) {
  return net_mobilewebprint::udp2_t::blast_(result, network_node, packet, long_timeout, long_timeout);
}

bool net_mobilewebprint::udp2_t::send(buffer_t & result, network_node_t & network_node, buffer_t & packet) {
  return net_mobilewebprint::udp2_t::blast_(result, network_node, packet, long_timeout, long_timeout, 1, 1);
}

bool net_mobilewebprint::udp2_t::blast_(
										buffer_t & result, 
										network_node_t & network_node, 
										buffer_t & packet, 
										struct timeval & timeoutA,
										struct timeval & timeoutB,
										int max_width,
										int max_height)
{
	int									blast_width = max_width  == -1 ? m_blast_width  : min(m_blast_width, max_width);
	int								 blast_height = max_height == -1 ? m_blast_height : min(m_blast_height, max_height);
  socklen_t								addrlen = sizeof(network_node.addr);
  int													max = -1;
	struct timeval *       timeout	= &timeoutA;

  // First, clear reads
  _clear_reads(network_node);

  // Then, blast the packet
  fd_set readable, writable, exceptional;
	int num_packets_to_send = blast_height * blast_width;
  for (int num_packets_sent = 0; num_packets_sent < num_packets_to_send;) {
		for (int i = 0; i < blast_width; ++i) {
      int fd = fds[i];

      zero(readable, writable, exceptional);

      max = -1;
      set_(max, writable, fd, &exceptional, fd);
			if (select(max + 1, NULL, &writable, &exceptional, &timeoutB) != 0) {
        if (FD_ISSET(fd, &writable)) {
          int num_sent = (int)sendto(fd, (char*)packet.bytes, packet.data_length, 0, (struct sockaddr*)&network_node.addr, addrlen);
          log_d("Sent: %d, %d", num_sent, i);
          num_packets_sent += 1;

          // We sent a packet, see if we can read
          zero(readable, writable, exceptional);

          max = -1;
					for (int j = 0; j < blast_width; ++j) {
            set_(max, readable, fds[j], &exceptional, fds[j]);
          }

          if (select(max + 1, &readable, NULL, &exceptional, timeout) != 0) {
            // Got a response
						for (int j = 0; j < blast_width; ++j) {
              if (FD_ISSET(fds[j], &readable)) {
                result.data_length = recvfrom(fds[j], (char*)result.bytes, result.mem_length, 0, (struct sockaddr*)&network_node.addr, &addrlen);
                log_d("Recd: %d from %d", (int)result.data_length, j);
                goto FOUND_RESPONSE;
              }
            }
          }

					if (timeout == &timeoutA) {
						timeout = &timeoutB;
          }

        }
      }
    }
  }
  return false;

FOUND_RESPONSE:
  _clear_reads(network_node);
  return true;
}

void net_mobilewebprint::udp2_t::_clear_reads(network_node_t & network_node) {
  int num_recd = 0;
  socklen_t addrlen = sizeof(network_node.addr);

  // Clear any reads
  fd_set readable, writable, exceptional;

  do {
    zero(readable, writable, exceptional);

    int max = -1;
		for (int j = 0; j < m_blast_width; ++j) {
      set_(max, readable, fds[j], &exceptional, fds[j]);
    }

    if (select(max + 1, &readable, NULL, &exceptional, &zero_timeout) == 0) {
      break;
    }

		for (int j = 0; j < m_blast_width; ++j) {
      if (FD_ISSET(fds[j], &readable)) {
        num_recd = (int)recvfrom(fds[j], (char*)trash, TRASH_SIZE, 0, (struct sockaddr*)&network_node.addr, &addrlen);
        log_d("Recd: %d from %d", num_recd, j);
      }
    }
  } while (true);
}

/**
*  Add another UDP socket
*/
void net_mobilewebprint::udp2_t::_add_socket() {
  int udp_fd = 0;
  if ((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
    if (bind(udp_fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) >= 0) {
      fds.push_back(udp_fd);
      if (udp_fd > fd_max) {
        fd_max = udp_fd;
      }
    }
  }
}

void zero(fd_set & A, fd_set & B, fd_set & C) {
  FD_ZERO(&A);
  FD_ZERO(&B);
  FD_ZERO(&C);
}

void set_(int & max, fd_set & A, int fdA, fd_set * B, int fdB, fd_set * C, int fdC) {
  FD_SET(fdA, &A);
  if (fdA > max) { max = fdA; }

  if (B) {
    if (fdB > max) { max = fdB; }
    FD_SET(fdB, B);
  }

  if (C) {
    if (fdC > max) { max = fdC; }
    FD_SET(fdC, C);
  }
}

