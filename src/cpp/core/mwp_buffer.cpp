
#include "mwp_buffer.hpp"

int net_mobilewebprint::num_allocations               = 0;
int net_mobilewebprint::num_buffer_allocations        = 0;
int net_mobilewebprint::num_buf_bytes_allocations     = 0;
int net_mobilewebprint::num_buf_bytes                 = 0;

size_t net_mobilewebprint::write_hton(byte * mem, uint16 sh)
{
  *(uint16*)(mem) = htons(sh);
  return sizeof(sh);
}

size_t net_mobilewebprint::write_hton(byte * mem, uint32 n)
{
  *(uint32*)(mem) = htons(n);
  return sizeof(n);
}

