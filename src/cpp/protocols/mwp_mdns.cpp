
#include "mwp_mdns.hpp"
#include "mwp_utils.hpp"

using namespace net_mobilewebprint;

using std::string;

//net_mobilewebprint::mdns_t::mdns_t(controller_t & controller_)
//  : controller(controller_), mq(controller_.mq)
//{
//}

string net_mobilewebprint::mdns_parsed_packet_t::read_stoopid_mdns_string(buffer_reader_t & reader)
{
  string result;

  while (!reader.at_end()) {
    byte first = reader.read_byte();
    //printf("0x%02x ", first);

    if (first == 0) {
      return result;
    }

    /* otherwise */
    if ((first & 0xc0) == 0xc0) {
      uint16 offset = (first & ~0xc0) << 8;
      offset += reader.read_byte();
      //printf("offset: 0x%04x \n", offset);

      buffer_reader_t reader2(reader);
      reader2.seek_to(offset);

      return _accumulate(result, read_stoopid_mdns_string(reader2), ".");
    } else {
      byte length = first;
      _accumulate(result, reader.read_string_nz(length), ".");
    }
  }

  return result;
}

net_mobilewebprint::mdns_header_t::mdns_header_t(buffer_reader_t & reader)
  : type(0), flags(0), ttl(0), data_length(0), data(reader)
{
  record_name = mdns_parsed_packet_t::read_stoopid_mdns_string(reader);

  type          = reader.read_uint16();
  flags         = reader.read_uint16();
  ttl           = reader.read_uint32();
  data_length   = reader.read_uint16();

  data          = reader;

  reader.seek(data_length);
}

buffer_reader_t net_mobilewebprint::mdns_header_t::data_reader() const
{
  return data;
}

mdns_record_base_t::mdns_record_base_t(mdns_header_t const & header)
  : record_name(header.record_name), type(header.type), flags(header.flags), ttl(header.ttl)
{
}

mdns_a_record_t::mdns_a_record_t(mdns_header_t const & header)
  : mdns_record_base_t(header)
{
  ip = header.data_reader().read_ip();
}

mdns_srv_record_t::mdns_srv_record_t(mdns_header_t const & header)
  : mdns_record_base_t(header)
{
  buffer_reader_t reader = header.data_reader();

  priority = reader.read_uint16();
  weight   = reader.read_uint16();
  port     = reader.read_uint16();

  target = mdns_parsed_packet_t::read_stoopid_mdns_string(reader);
}

mdns_txt_record_t::mdns_txt_record_t(mdns_header_t const & header)
  : mdns_record_base_t(header)
{
  buffer_reader_t reader = header.data_reader();
  byte const *     start = reader.p;
  byte const *       end = start + header.data_length;

  string kv;
  while (reader.p < end) {

    size_t length = reader.read_byte();
    kv = reader.read_string_nz(length);
    //printf("kv: %s\n", kv.c_str());
    _add_kv(dict, kv, "=");
  }
}


//  struct mdns_txt_record_t : public mdns_record_base_t
//  {
//    strmap dict;
//
//    mdns_srv_record_t(mdns_header_t const &);
//  };
