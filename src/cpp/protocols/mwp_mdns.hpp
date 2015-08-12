
#ifndef __MWP_MDNS_HPP__
#define __MWP_MDNS_HPP__

#include <mwp_controller.hpp>
#include <mwp_mq.hpp>

#include <string>

namespace net_mobilewebprint {

  using std::string;

  namespace mdns {
    enum record_types {
      a         =   1,  /* 0x0001 */
      cname     =   5,  /*  */
      soa       =   6,  /*  */
      ptr       =  12,  /* 0x000c */
      mx        =  15,  /* 0x000f */
      txt       =  16,  /* 0x0010 */
      aaaa      =  28,  /* 0x001c */
      srv       =  33,  /* 0x0021 */
      cert      =  37,  /*  */
      ipseckey  =  45,  /*  */

      last      = 0xffff
    };
  };

  struct mdns_parsed_packet_t
  {
    buffer_t *    buffer;
    mdns_parsed_packet_t(byte const *, size_t lenght);
    ~mdns_parsed_packet_t();

    static string read_stoopid_mdns_string(buffer_reader_t &);
  };

  struct mdns_header_t
  {
    string              record_name;
    uint16              type;
    uint16              flags;
    uint32              ttl;
    uint16              data_length;
    buffer_reader_t     data;

    mdns_header_t(buffer_reader_t & reader);

    buffer_reader_t data_reader() const;
  };

  struct mdns_record_base_t
  {
    string              record_name;
    uint16              type;
    uint16              flags;
    uint32              ttl;

    mdns_record_base_t(mdns_header_t const &);
  };

  struct mdns_a_record_t : public mdns_record_base_t
  {
    string ip;

    mdns_a_record_t(mdns_header_t const &);
  };

  struct mdns_srv_record_t : public mdns_record_base_t
  {
    uint16              priority;
    uint16              weight;
    uint16              port;
    string              target;

    mdns_srv_record_t(mdns_header_t const &);
  };

  struct mdns_txt_record_t : public mdns_record_base_t
  {
    strmap dict;

    mdns_txt_record_t(mdns_header_t const &);
  };

//  struct mdns_t : public mq_handler_t
//  {
//    controller_t & controller;
//    mq_t         & mq;
//
//    mdns_t(controller_t &);
//  };

};

#endif  // __MWP_MDNS_HPP__

