
#ifndef __MWP_BUFFER_HPP__
#define __MWP_BUFFER_HPP__

#include "mwp_types.hpp"
#include <cstddef>

namespace net_mobilewebprint {

  using std::size_t;
  using net_mobilewebprint::byte;
  using net_mobilewebprint::uint16;

  struct buffer_ct
  {
    buffer_ct(byte   by);
    buffer_ct(uint16 sh);

    int size();

    size_t mem_size;
  };

};

#endif // __MWP_BUFFER_HPP__

