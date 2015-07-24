
#include "mwp_buffer.hpp"

net_mobilewebprint::buffer_ct::buffer_ct(byte by)
  : mem_size(sizeof(by))
{
}

net_mobilewebprint::buffer_ct::buffer_ct(uint16 sh)
  : mem_size(sizeof(sh))
{
}

int net_mobilewebprint::buffer_ct::size()
{
  return mem_size;
}


