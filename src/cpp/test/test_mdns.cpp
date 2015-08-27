
#include "catch.hpp"
#include "mwp_mdns.hpp"
#include "mwp_assert.hpp"

#include <pcap.h>

using namespace net_mobilewebprint;

extern const unsigned char mdns_pkt1[1334];
extern const unsigned char mdns_pkt2[75];

TEST_CASE("mdns_t can a bunch of packets", "[mdns]")
{
  reset_assert_count();

  byte            by = 0;
  uint16          sh = 0;
  uint32          n  = 0;

  char            errbuf[PCAP_ERRBUF_SIZE];
  u_char const *  packet = NULL;
  pcap_t *        handle = NULL;
  pcap_pkthdr     header;

  SECTION("read pcap file") {
    handle = pcap_open_offline("../../src/cpp/test/data/mdns2.pcap", errbuf);
    REQUIRE(handle != NULL);
    if (handle != NULL) {
      while ((packet = pcap_next(handle, &header)) != NULL) {

        //printf("Packet. data_len: %d, on_wire_len %d\n", header.caplen, header.len);

        // Make a buffer for the data
        buffer_t         buffer(packet, header.caplen);
        buffer_reader_t  reader(buffer);

        // ---------- Ethernet ----------
        reader.read_byte(); reader.read_byte(); reader.read_byte(); reader.read_byte(); reader.read_byte(); reader.read_byte();
        reader.read_byte(); reader.read_byte(); reader.read_byte(); reader.read_byte(); reader.read_byte(); reader.read_byte();
        //printf("dest_mac: %02x:%02x:%02x:%02x:%02x:%02x  ", reader.read_byte(), reader.read_byte(), reader.read_byte(), reader.read_byte(), reader.read_byte(), reader.read_byte());
        //printf("src_mac: %02x:%02x:%02x:%02x:%02x:%02x\n", reader.read_byte(), reader.read_byte(), reader.read_byte(), reader.read_byte(), reader.read_byte(), reader.read_byte());

        sh = reader.read_uint16();                // Type

        // ---------- IP ----------
        by = reader.read_byte();                  // From the IP header

        if (sh == 0x0800) {

          // IPv4
          REQUIRE(by >> 4 == 4);
          int ip_header_len = (by & 0x0f) * 4;
          REQUIRE(ip_header_len >= 20);
          REQUIRE(ip_header_len < 28);

          by = reader.read_byte();          // Type of Service (TOS)
          sh = reader.read_uint16();        // Total packet length
          sh = reader.read_uint16();        // Identification
          sh = reader.read_uint16();        // Flags and Fragmentation offset
          by = reader.read_byte();          // TTL
          by = reader.read_byte();          // Protocol (i.e. UDP or TCP)

          byte protocol = by;

          reader.seek_to(14 + ip_header_len);   // Seek to the UDP header

          reader.read_uint16();
          reader.read_uint16();

          //REQUIRE(reader.read_uint16() == 5353);
          //REQUIRE(reader.read_uint16() == 5353);

          reader.seek(4);

          // Currently, we are just trying to ensure we are stable and don't crash on the packet data
          mdns_header_t header(reader);

        } else if (sh == 0x86dd) {
          REQUIRE(by >> 4 == 6);
        } else {
          REQUIRE(false);
        }

      }
    } else {
      fprintf(stderr, "Cannot open mdns2 data file %s\n", errbuf);
    }

  }

  REQUIRE( num_asserts() == 0 );
}

TEST_CASE("mdns_t can read stoopid mdns strings", "[mdns]")
{
  reset_assert_count();

  REQUIRE( num_asserts() == 0 );
  buffer_t buffer(mdns_pkt1 + 0x2a, sizeof(mdns_pkt1) - 0x2a);

  SECTION("mdns_parsed_packet_t should parse simple mdns-style strings") {

    buffer_reader_t reader(buffer);

    reader.seek(0x0c);
    string str = mdns_parsed_packet_t::read_stoopid_mdns_string(reader);
    REQUIRE(str == "_printer._tcp.local");
  }

  SECTION("mdns_parsed_packet_t should parse complex mdns-style strings") {

    buffer_reader_t reader(buffer);

    reader.seek_to(0x55 - 0x2a);
    string str = mdns_parsed_packet_t::read_stoopid_mdns_string(reader);
    REQUIRE(str == "MarkOfficejet Pro 8500 A910 [32BD6D]._printer._tcp.local");
  }

  REQUIRE( num_asserts() == 0 );
}

TEST_CASE("utils work on packets")
{
  reset_assert_count();

  REQUIRE( num_asserts() == 0 );
  buffer_t buffer(mdns_pkt1, sizeof(mdns_pkt1));
  buffer_reader_t reader(buffer);

  SECTION("utils can read IP address as string") {
    reader.seek_to(0x0148);
    REQUIRE(mwp_ntop(reader.p) == "15.80.127.138");
  }
}

TEST_CASE("mdns_t can read mdns headers", "[mdns]")
{
  reset_assert_count();

  REQUIRE( num_asserts() == 0 );
  buffer_t buffer(mdns_pkt1 + 0x2a, sizeof(mdns_pkt1) - 0x2a);
  string str;

  printf("----------------------------\n");
  SECTION("mdns_header_t can read header") {

    buffer_reader_t reader(buffer);

    reader.seek(12);  // Skip past MDNS header

    // -------------------------------------------------------------------------------- 1
    {
      mdns_header_t header(reader);
      REQUIRE(header.record_name == "_printer._tcp.local");

      REQUIRE(header.type        == mdns::ptr);
      REQUIRE(header.flags       == 0x0001);
      REQUIRE(header.ttl         == 0x00001194);
      REQUIRE(header.data_length == 0x0027);

      buffer_reader_t reader2 = header.data_reader();
      str = mdns_parsed_packet_t::read_stoopid_mdns_string(reader2);
      REQUIRE(str == "MarkOfficejet Pro 8500 A910 [32BD6D]._printer._tcp.local");
    }

    // -------------------------------------------------------------------------------- 2
    {
      mdns_header_t header(reader);
      REQUIRE(header.record_name == "_pdl-datastream._tcp.local");

      REQUIRE(header.type        == mdns::ptr);
      REQUIRE(header.flags       == 0x0001);
      REQUIRE(header.ttl         == 0x00001194);
      REQUIRE(header.data_length == 0x0027);

      buffer_reader_t reader2 = header.data_reader();
      str = mdns_parsed_packet_t::read_stoopid_mdns_string(reader2);
      REQUIRE(str == "MarkOfficejet Pro 8500 A910 [32BD6D]._pdl-datastream._tcp.local");
    }

    // -------------------------------------------------------------------------------- 3
    {
      mdns_header_t header(reader);
      REQUIRE(header.record_name == "_ipp._tcp.local");

      REQUIRE(header.type        == mdns::ptr);
      REQUIRE(header.flags       == 0x0001);
      REQUIRE(header.ttl         == 0x00001194);
      REQUIRE(header.data_length == 0x0027);

      buffer_reader_t reader2 = header.data_reader();
      str = mdns_parsed_packet_t::read_stoopid_mdns_string(reader2);
      REQUIRE(str == "MarkOfficejet Pro 8500 A910 [32BD6D]._ipp._tcp.local");
    }

    // -------------------------------------------------------------------------------- 4
    {
      mdns_header_t header(reader);
      REQUIRE(header.record_name == "_scanner._tcp.local");

      REQUIRE(header.type        == mdns::ptr);
      REQUIRE(header.flags       == 0x0001);
      REQUIRE(header.ttl         == 0x00001194);
      REQUIRE(header.data_length == 0x0027);

      buffer_reader_t reader2 = header.data_reader();
      str = mdns_parsed_packet_t::read_stoopid_mdns_string(reader2);
      REQUIRE(str == "MarkOfficejet Pro 8500 A910 [32BD6D]._scanner._tcp.local");
    }

    // -------------------------------------------------------------------------------- 5
    {
      mdns_header_t header(reader);
      REQUIRE(header.record_name == "HP32BD6D.local");

      REQUIRE(header.type        == mdns::a);
      REQUIRE(header.flags       == 0x8001);
      REQUIRE(header.ttl         == 0x00000078);
      REQUIRE(header.data_length == 0x0004);

      mdns_a_record_t a(header);

      REQUIRE(a.ip == "15.80.127.138");
    }

    // -------------------------------------------------------------------------------- 6
    {
      mdns_header_t header(reader);
      REQUIRE(header.record_name == "HP32BD6D.local");

      REQUIRE(header.type        == mdns::aaaa);
      REQUIRE(header.flags       == 0x8001);
      REQUIRE(header.ttl         == 0x00000078);
      REQUIRE(header.data_length == 0x0010);

    }

    // -------------------------------------------------------------------------------- 7
    {
      mdns_header_t header(reader);
      REQUIRE(header.record_name == "MarkOfficejet Pro 8500 A910 [32BD6D]._printer._tcp.local");

      REQUIRE(header.type        == mdns::srv);
      REQUIRE(header.flags       == 0x8001);
      REQUIRE(header.ttl         == 0x00000078);
      REQUIRE(header.data_length == 0x0008);

      mdns_srv_record_t srv(header);

      REQUIRE(srv.priority       == 0);
      REQUIRE(srv.weight         == 0);
      REQUIRE(srv.port           == 515);

      str = srv.target;
      REQUIRE(str == "HP32BD6D.local");
    }

    // -------------------------------------------------------------------------------- 8
    {
      mdns_header_t header(reader);
      REQUIRE(header.record_name == "MarkOfficejet Pro 8500 A910 [32BD6D]._printer._tcp.local");

      REQUIRE(header.type        == mdns::txt);
      REQUIRE(header.flags       == 0x8001);
      REQUIRE(header.ttl         == 0x00001194);
      REQUIRE(header.data_length == 0x00d4);

      mdns_txt_record_t txt(header);

      REQUIRE(txt.dict.size() == 12);

      REQUIRE(txt.dict["txtvers"] == "1");
      REQUIRE(txt.dict["qtotal"] == "1");
      REQUIRE(txt.dict["rp"] == "RAW");
      REQUIRE(txt.dict["pdl"] == "application/vnd.hp-PCL,image/urf,image/jpeg");
      REQUIRE(txt.dict["ty"] == "Officejet Pro 8500 A910");
      REQUIRE(txt.dict["product"] == "(HP Officejet Pro 8500 A910)");
      REQUIRE(txt.dict["priority"] == "52");
      REQUIRE(txt.dict["adminurl"] == "http://HP32BD6D.local.");
      REQUIRE(txt.dict["note"] == "");
      REQUIRE(txt.dict["Color"] == "T");
      REQUIRE(txt.dict["Duplex"] == "T");
      REQUIRE(txt.dict["Scan"] == "T");
    }

    // -------------------------------------------------------------------------------- 9
    {
      mdns_header_t header(reader);
      REQUIRE(header.record_name == "MarkOfficejet Pro 8500 A910 [32BD6D]._pdl-datastream._tcp.local");

      REQUIRE(header.type        == mdns::srv);
      REQUIRE(header.flags       == 0x8001);
      REQUIRE(header.ttl         == 0x00000078);
      REQUIRE(header.data_length == 0x0008);

      mdns_srv_record_t srv(header);

      REQUIRE(srv.priority       == 0);
      REQUIRE(srv.weight         == 0);
      REQUIRE(srv.port           == 9100);

      str = srv.target;
      REQUIRE(str == "HP32BD6D.local");
    }

    // -------------------------------------------------------------------------------- 10
    {
      mdns_header_t header(reader);
      REQUIRE(header.record_name == "MarkOfficejet Pro 8500 A910 [32BD6D]._pdl-datastream._tcp.local");

      REQUIRE(header.type        == mdns::txt);
      REQUIRE(header.flags       == 0x8001);
      REQUIRE(header.ttl         == 0x00001194);
      REQUIRE(header.data_length == 0x00cd);

      mdns_txt_record_t txt(header);

      REQUIRE(txt.dict.size() == 11);

      REQUIRE(txt.dict["txtvers"] == "1");
      REQUIRE(txt.dict["qtotal"] == "1");
      REQUIRE(txt.dict["pdl"] == "application/vnd.hp-PCL,image/urf,image/jpeg");
      REQUIRE(txt.dict["ty"] == "Officejet Pro 8500 A910");
      REQUIRE(txt.dict["product"] == "(HP Officejet Pro 8500 A910)");
      REQUIRE(txt.dict["priority"] == "30");
      REQUIRE(txt.dict["adminurl"] == "http://HP32BD6D.local.");
      REQUIRE(txt.dict["note"] == "");
      REQUIRE(txt.dict["Color"] == "T");
      REQUIRE(txt.dict["Duplex"] == "T");
      REQUIRE(txt.dict["Scan"] == "T");
    }

    // -------------------------------------------------------------------------------- 11
    {
      mdns_header_t header(reader);
      REQUIRE(header.record_name == "MarkOfficejet Pro 8500 A910 [32BD6D]._ipp._tcp.local");

      REQUIRE(header.type        == mdns::srv);
      REQUIRE(header.flags       == 0x8001);
      REQUIRE(header.ttl         == 0x00000078);
      REQUIRE(header.data_length == 0x0008);

      mdns_srv_record_t srv(header);

      REQUIRE(srv.priority       == 0);
      REQUIRE(srv.weight         == 0);
      REQUIRE(srv.port           == 631);

      str = srv.target;
      REQUIRE(str == "HP32BD6D.local");
    }

    // -------------------------------------------------------------------------------- 12
    {
      mdns_header_t header(reader);
      REQUIRE(header.record_name == "MarkOfficejet Pro 8500 A910 [32BD6D]._ipp._tcp.local");

      REQUIRE(header.type        == mdns::txt);
      REQUIRE(header.flags       == 0x8001);
      REQUIRE(header.ttl         == 0x00001194);
      REQUIRE(header.data_length == 0x0123);

      mdns_txt_record_t txt(header);

      REQUIRE(txt.dict.size() == 13);

      REQUIRE(txt.dict["txtvers"] == "1");
      REQUIRE(txt.dict["qtotal"] == "1");
      REQUIRE(txt.dict["rp"] == "ipp/printer");
      REQUIRE(txt.dict["URF"] == "CP1,MT1-2-8-9-10-11,OB9,OFU0,PQ3-4-5,RS300-600,SRGB24,W8,DM3,IS1-2");
      REQUIRE(txt.dict["pdl"] == "application/vnd.hp-PCL,image/urf,image/jpeg");
      REQUIRE(txt.dict["ty"] == "Officejet Pro 8500 A910");
      REQUIRE(txt.dict["product"] == "(HP Officejet Pro 8500 A910)");
      REQUIRE(txt.dict["priority"] == "60");
      REQUIRE(txt.dict["adminurl"] == "http://HP32BD6D.local.");
      REQUIRE(txt.dict["note"] == "");
      REQUIRE(txt.dict["Color"] == "T");
      REQUIRE(txt.dict["Duplex"] == "T");
      REQUIRE(txt.dict["Scan"] == "T");
    }

    // -------------------------------------------------------------------------------- 13
    {
      mdns_header_t header(reader);
      REQUIRE(header.record_name == "MarkOfficejet Pro 8500 A910 [32BD6D]._scanner._tcp.local");

      REQUIRE(header.type        == mdns::srv);
      REQUIRE(header.flags       == 0x8001);
      REQUIRE(header.ttl         == 0x00000078);
      REQUIRE(header.data_length == 0x0008);

      mdns_srv_record_t srv(header);

      REQUIRE(srv.priority       == 0);
      REQUIRE(srv.weight         == 0);
      REQUIRE(srv.port           == 8080);

      str = srv.target;
      REQUIRE(str == "HP32BD6D.local");
    }

    // -------------------------------------------------------------------------------- 14
    {
      mdns_header_t header(reader);
      REQUIRE(header.record_name == "MarkOfficejet Pro 8500 A910 [32BD6D]._scanner._tcp.local");

      REQUIRE(header.type        == mdns::txt);
      REQUIRE(header.flags       == 0x8001);
      REQUIRE(header.ttl         == 0x00001194);
      REQUIRE(header.data_length == 0x008a);

      mdns_txt_record_t txt(header);

      REQUIRE(txt.dict.size() == 9);

      REQUIRE(txt.dict["txtvers"] == "1");
      REQUIRE(txt.dict["ty"] == "Officejet Pro 8500 A910");
      REQUIRE(txt.dict["mfg"] == "HP");
      REQUIRE(txt.dict["mdl"] == "Officejet Pro 8500 A910");
      REQUIRE(txt.dict["adminurl"] == "http://HP32BD6D.local.");
      REQUIRE(txt.dict["note"] == "");
      REQUIRE(txt.dict["button"] == "T");
      REQUIRE(txt.dict["flatbed"] == "T");
      REQUIRE(txt.dict["feeder"] == "T");
    }

//    // -------------------------------------------------------------------------------- 8
//    {
//      mdns_header_t header(reader);
//      printf("%s: 0x%04x 0x%04x 0x%08x 0x%04x  --  0x%04x\n", header.record_name.c_str(), header.type, header.flags, header.ttl, header.data_length, 0x2a + reader.tell());
//    }

  }

  REQUIRE( num_asserts() == 0 );

}

TEST_CASE("mdns_t can make pdl request packet", "[mdns]")
{
  reset_assert_count();

  buffer_t pdl_query_request;

  SECTION("mdns_t makes pdl") {
    mdns_t::_mk_query_request_packet(pdl_query_request, mdns::ptr, "_pdl-datastream");
    buffer_reader_t reader(pdl_query_request);

    uint16 sh = reader.read_uint16();                 /* transaction ID */
    REQUIRE((sh = reader.read_uint16()) == 0);        /* flags */

    REQUIRE((sh = reader.read_uint16()) == 1);        /* number of questions */
    REQUIRE((sh = reader.read_uint16()) == 0);        /* number of answers */
    REQUIRE((sh = reader.read_uint16()) == 0);        /* number of authority records */
    REQUIRE((sh = reader.read_uint16()) == 0);        /* number of additional records */

    byte by = reader.read_byte();
    REQUIRE(reader.read_string_nz(by) == "_pdl-datastream");

    by = reader.read_byte();
    REQUIRE(reader.read_string_nz(by) == "_tcp");

    by = reader.read_byte();
    REQUIRE(reader.read_string_nz(by) == "local");

    REQUIRE((by = reader.read_byte()) == 0);
    REQUIRE((sh = reader.read_uint16()) == mdns::ptr);
    REQUIRE((sh = reader.read_uint16()) == 0x8001);

    REQUIRE( num_asserts() == 0 );
  }

  REQUIRE( num_asserts() == 0 );
}

TEST_CASE("mdns_t can read mdns headers2", "[mdns]")
{
  reset_assert_count();

  REQUIRE( num_asserts() == 0 );
  buffer_t buffer(mdns_pkt2, sizeof(mdns_pkt2));
  string str;

  SECTION("mdns_header_t can read header") {

    buffer_reader_t reader(buffer);

    reader.seek(12);  // Skip past MDNS header

    // -------------------------------------------------------------------------------- 1
    {
      mdns_header_t header(reader, /*is_question=*/true);
      REQUIRE(header.record_name == "_canon-bjnp1._tcp.local");

      REQUIRE(header.type        == mdns::ptr);
      REQUIRE(header.flags       == 0x0001);
      //REQUIRE(header.ttl         == 0x00001194);
      //REQUIRE(header.data_length == 0x0027);

      buffer_reader_t reader2 = header.data_reader();
      str = mdns_parsed_packet_t::read_stoopid_mdns_string(reader2);
      REQUIRE(str == "_canon-bjnp1._tcp.local");
    }

    // -------------------------------------------------------------------------------- 2
    {
      mdns_header_t header(reader);
      REQUIRE(header.record_name == "_canon-bjnp1._tcp.local");

      REQUIRE(header.type        == mdns::ptr);
      REQUIRE(header.flags       == 0x0001);
      REQUIRE(header.ttl         == 0x0000000a);
      REQUIRE(header.data_length == 0x0016);

      buffer_reader_t reader2 = header.data_reader();
      str = mdns_parsed_packet_t::read_stoopid_mdns_string(reader2);
      REQUIRE(str == "Canon MG5500 series._canon-bjnp1._tcp.local");
    }
  }

  REQUIRE( num_asserts() == 0 );

}

const unsigned char mdns_pkt2[75] = {
/*0000000*/ 0x00, 0x05, 0x84, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x5f, 0x63, 0x61,
/*0000010*/ 0x6e, 0x6f, 0x6e, 0x2d, 0x62, 0x6a, 0x6e, 0x70, 0x31, 0x04, 0x5f, 0x74, 0x63, 0x70, 0x05, 0x6c,
/*0000020*/ 0x6f, 0x63, 0x61, 0x6c, 0x00, 0x00, 0x0c, 0x00, 0x01, 0xc0, 0x0c, 0x00, 0x0c, 0x00, 0x01, 0x00,
/*0000030*/ 0x00, 0x00, 0x0a, 0x00, 0x16, 0x13, 0x43, 0x61, 0x6e, 0x6f, 0x6e, 0x20, 0x4d, 0x47, 0x35, 0x35,
/*0000040*/ 0x30, 0x30, 0x20, 0x73, 0x65, 0x72, 0x69, 0x65, 0x73, 0xc0, 0x0c
/*000004b*/
};

/* Frame (1334 bytes) */
const unsigned char mdns_pkt1[1334] = {
0x01, 0x00, 0x5e, 0x00, 0x00, 0xfb, 0xd4, 0x85, /* ..^.....  00000000 */
0x64, 0x32, 0xbd, 0x6d, 0x08, 0x00, 0x45, 0x00, /* d2.m..E.  00000008 */
0x05, 0x28, 0x35, 0x09, 0x00, 0x00, 0xff, 0x11, /* .(5.....  00000010 */
0x11, 0xe6, 0x0f, 0x50, 0x7f, 0x8a, 0xe0, 0x00, /* ...P....  00000018 */
0x00, 0xfb, 0x14, 0xe9, 0x14, 0xe9, 0x05, 0x14, /* ........  00000020 */
0xf2, 0xc7,
            0x00, 0x00, 0x84, 0x00, 0x00, 0x00, /* ........  00000028 */ /* - 0x2a */                 // ID 0x0000, flags 0x8400, #qs 0x0000
0x00, 0x04, 0x00, 0x00, 0x00, 0x0a,                                                                   // #rrs 0x0004 #auth_rrs 0x0000 #addl_rrs 0x000a
                                    0x08, 0x5f, /* ......._  00000030 */ /* 0x36 - 0x2a = 0x0c */
0x70, 0x72, 0x69, 0x6e, 0x74, 0x65, 0x72, 0x04, /* printer.  00000038 */
0x5f, 0x74, 0x63, 0x70, 0x05, 0x6c, 0x6f, 0x63, /* _tcp.loc  00000040 */
0x61, 0x6c, 0x00,                               /* al.       00000048 */
                  0x00, 0x0c, 0x00, 0x01, 0x00, /*    .....           */
0x00, 0x11, 0x94, 0x00, 0x27,
                              0x24, 0x4d, 0x61, /* ....'$Ma  00000050 */ /* 0x55 - 0x2a = 0x2e */
0x72, 0x6b, 0x4f, 0x66, 0x66, 0x69, 0x63, 0x65, /* rkOffice  00000058 */
0x6a, 0x65, 0x74, 0x20, 0x50, 0x72, 0x6f, 0x20, /* jet Pro   00000060 */
0x38, 0x35, 0x30, 0x30, 0x20, 0x41, 0x39, 0x31, /* 8500 A91  00000068 */
0x30, 0x20, 0x5b, 0x33, 0x32, 0x42, 0x44, 0x36, /* 0 [32BD6  00000070 */
0x44, 0x5d,                                     /* D]        00000078 */
            0xc0, 0x0c,                         /*   ..               */
                        0x0f, 0x5f, 0x70, 0x64, /*     ._pd           */
0x6c, 0x2d, 0x64, 0x61, 0x74, 0x61, 0x73, 0x74, /* l-datast  00000080 */
0x72, 0x65, 0x61, 0x6d,                         /* ream      00000088 */ /* 0x8b - 0x2a = 0x61 */
                        0xc0, 0x15,             /*     ..             */
                                    0x00, 0x0c, /*       ..           */
0x00, 0x01, 0x00, 0x00, 0x11, 0x94, 0x00, 0x27, /* .......'  00000090 */

0x24, 0x4d, 0x61, 0x72, 0x6b, 0x4f, 0x66, 0x66, /* $MarkOff  00000098 */
0x69, 0x63, 0x65, 0x6a, 0x65, 0x74, 0x20, 0x50, /* icejet P  000000a0 */
0x72, 0x6f, 0x20, 0x38, 0x35, 0x30, 0x30, 0x20, /* ro 8500   000000a8 */
0x41, 0x39, 0x31, 0x30, 0x20, 0x5b, 0x33, 0x32, /* A910 [32  000000b0 */
0x42, 0x44, 0x36, 0x44, 0x5d,                   /* BD6D]     000000b8 */
                              0xc0, 0x52, 0x04, /*      .R.           */
0x5f, 0x69, 0x70, 0x70, 0xc0, 0x15, 0x00, 0x0c, /* _ipp....  000000c0 */
0x00, 0x01, 0x00, 0x00, 0x11, 0x94, 0x00, 0x27, /* .......'  000000c8 */
0x24, 0x4d, 0x61, 0x72, 0x6b, 0x4f, 0x66, 0x66, /* $MarkOff  000000d0 */
0x69, 0x63, 0x65, 0x6a, 0x65, 0x74, 0x20, 0x50, /* icejet P  000000d8 */
0x72, 0x6f, 0x20, 0x38, 0x35, 0x30, 0x30, 0x20, /* ro 8500   000000e0 */
0x41, 0x39, 0x31, 0x30, 0x20, 0x5b, 0x33, 0x32, /* A910 [32  000000e8 */
0x42, 0x44, 0x36, 0x44, 0x5d, 0xc0, 0x95, 0x08, /* BD6D]...  000000f0 */
0x5f, 0x73, 0x63, 0x61, 0x6e, 0x6e, 0x65, 0x72, /* _scanner  000000f8 */
0xc0, 0x15, 0x00, 0x0c, 0x00, 0x01, 0x00, 0x00, /* ........  00000100 */
0x11, 0x94, 0x00, 0x27, 0x24, 0x4d, 0x61, 0x72, /* ...'$Mar  00000108 */
0x6b, 0x4f, 0x66, 0x66, 0x69, 0x63, 0x65, 0x6a, /* kOfficej  00000110 */
0x65, 0x74, 0x20, 0x50, 0x72, 0x6f, 0x20, 0x38, /* et Pro 8  00000118 */
0x35, 0x30, 0x30, 0x20, 0x41, 0x39, 0x31, 0x30, /* 500 A910  00000120 */
0x20, 0x5b, 0x33, 0x32, 0x42, 0x44, 0x36, 0x44, /*  [32BD6D  00000128 */
0x5d, 0xc0, 0xcd, 0x08, 0x48, 0x50, 0x33, 0x32, /* ]...HP32  00000130 */
0x42, 0x44, 0x36, 0x44, 0xc0, 0x1a, 0x00, 0x01, /* BD6D....  00000138 */
0x80, 0x01, 0x00, 0x00, 0x00, 0x78, 0x00, 0x04, /* .....x..  00000140 */

0x0f, 0x50, 0x7f, 0x8a, 0xc1, 0x09, 0x00, 0x1c, /* .P......  00000148 */
0x80, 0x01, 0x00, 0x00, 0x00, 0x78, 0x00, 0x10, /* .....x..  00000150 */
0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ........  00000158 */
0xd6, 0x85, 0x64, 0xff, 0xfe, 0x32, 0xbd, 0x6d, /* ..d..2.m  00000160 */
0xc0, 0x2b, 0x00, 0x21, 0x80, 0x01, 0x00, 0x00, /* .+.!....  00000168 */
0x00, 0x78, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, /* .x......  00000170 */
0x02, 0x03, 0xc1, 0x09, 0xc0, 0x2b, 0x00, 0x10, /* .....+..  00000178 */
0x80, 0x01, 0x00, 0x00, 0x11, 0x94, 0x00, 0xd4, /* ........  00000180 */
0x09, 0x74, 0x78, 0x74, 0x76, 0x65, 0x72, 0x73, /* .txtvers  00000188 */
0x3d, 0x31, 0x08, 0x71, 0x74, 0x6f, 0x74, 0x61, /* =1.qtota  00000190 */
0x6c, 0x3d, 0x31, 0x06, 0x72, 0x70, 0x3d, 0x52, /* l=1.rp=R  00000198 */
0x41, 0x57, 0x2f, 0x70, 0x64, 0x6c, 0x3d, 0x61, /* AW/pdl=a  000001a0 */
0x70, 0x70, 0x6c, 0x69, 0x63, 0x61, 0x74, 0x69, /* pplicati  000001a8 */
0x6f, 0x6e, 0x2f, 0x76, 0x6e, 0x64, 0x2e, 0x68, /* on/vnd.h  000001b0 */
0x70, 0x2d, 0x50, 0x43, 0x4c, 0x2c, 0x69, 0x6d, /* p-PCL,im  000001b8 */
0x61, 0x67, 0x65, 0x2f, 0x75, 0x72, 0x66, 0x2c, /* age/urf,  000001c0 */
0x69, 0x6d, 0x61, 0x67, 0x65, 0x2f, 0x6a, 0x70, /* image/jp  000001c8 */
0x65, 0x67, 0x1a, 0x74, 0x79, 0x3d, 0x4f, 0x66, /* eg.ty=Of  000001d0 */
0x66, 0x69, 0x63, 0x65, 0x6a, 0x65, 0x74, 0x20, /* ficejet   000001d8 */
0x50, 0x72, 0x6f, 0x20, 0x38, 0x35, 0x30, 0x30, /* Pro 8500  000001e0 */
0x20, 0x41, 0x39, 0x31, 0x30, 0x24, 0x70, 0x72, /*  A910$pr  000001e8 */
0x6f, 0x64, 0x75, 0x63, 0x74, 0x3d, 0x28, 0x48, /* oduct=(H  000001f0 */
0x50, 0x20, 0x4f, 0x66, 0x66, 0x69, 0x63, 0x65, /* P Office  000001f8 */
0x6a, 0x65, 0x74, 0x20, 0x50, 0x72, 0x6f, 0x20, /* jet Pro   00000200 */
0x38, 0x35, 0x30, 0x30, 0x20, 0x41, 0x39, 0x31, /* 8500 A91  00000208 */
0x30, 0x29, 0x0b, 0x70, 0x72, 0x69, 0x6f, 0x72, /* 0).prior  00000210 */
0x69, 0x74, 0x79, 0x3d, 0x35, 0x32, 0x1f, 0x61, /* ity=52.a  00000218 */
0x64, 0x6d, 0x69, 0x6e, 0x75, 0x72, 0x6c, 0x3d, /* dminurl=  00000220 */
0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x48, /* http://H  00000228 */
0x50, 0x33, 0x32, 0x42, 0x44, 0x36, 0x44, 0x2e, /* P32BD6D.  00000230 */
0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x2e, 0x05, 0x6e, /* local..n  00000238 */
0x6f, 0x74, 0x65, 0x3d, 0x07, 0x43, 0x6f, 0x6c, /* ote=.Col  00000240 */
0x6f, 0x72, 0x3d, 0x54, 0x08, 0x44, 0x75, 0x70, /* or=T.Dup  00000248 */
0x6c, 0x65, 0x78, 0x3d, 0x54, 0x06, 0x53, 0x63, /* lex=T.Sc  00000250 */
0x61, 0x6e, 0x3d, 0x54, 0xc0, 0x6e, 0x00, 0x21, /* an=T.n.!  00000258 */
0x80, 0x01, 0x00, 0x00, 0x00, 0x78, 0x00, 0x08, /* .....x..  00000260 */
0x00, 0x00, 0x00, 0x00, 0x23, 0x8c, 0xc1, 0x09, /* ....#...  00000268 */
0xc0, 0x6e, 0x00, 0x10, 0x80, 0x01, 0x00, 0x00, /* .n......  00000270 */
0x11, 0x94, 0x00, 0xcd, 0x09, 0x74, 0x78, 0x74, /* .....txt  00000278 */
0x76, 0x65, 0x72, 0x73, 0x3d, 0x31, 0x08, 0x71, /* vers=1.q  00000280 */
0x74, 0x6f, 0x74, 0x61, 0x6c, 0x3d, 0x31, 0x2f, /* total=1/  00000288 */
0x70, 0x64, 0x6c, 0x3d, 0x61, 0x70, 0x70, 0x6c, /* pdl=appl  00000290 */
0x69, 0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x2f, /* ication/  00000298 */
0x76, 0x6e, 0x64, 0x2e, 0x68, 0x70, 0x2d, 0x50, /* vnd.hp-P  000002a0 */
0x43, 0x4c, 0x2c, 0x69, 0x6d, 0x61, 0x67, 0x65, /* CL,image  000002a8 */
0x2f, 0x75, 0x72, 0x66, 0x2c, 0x69, 0x6d, 0x61, /* /urf,ima  000002b0 */
0x67, 0x65, 0x2f, 0x6a, 0x70, 0x65, 0x67, 0x1a, /* ge/jpeg.  000002b8 */
0x74, 0x79, 0x3d, 0x4f, 0x66, 0x66, 0x69, 0x63, /* ty=Offic  000002c0 */
0x65, 0x6a, 0x65, 0x74, 0x20, 0x50, 0x72, 0x6f, /* ejet Pro  000002c8 */
0x20, 0x38, 0x35, 0x30, 0x30, 0x20, 0x41, 0x39, /*  8500 A9  000002d0 */
0x31, 0x30, 0x24, 0x70, 0x72, 0x6f, 0x64, 0x75, /* 10$produ  000002d8 */
0x63, 0x74, 0x3d, 0x28, 0x48, 0x50, 0x20, 0x4f, /* ct=(HP O  000002e0 */
0x66, 0x66, 0x69, 0x63, 0x65, 0x6a, 0x65, 0x74, /* fficejet  000002e8 */
0x20, 0x50, 0x72, 0x6f, 0x20, 0x38, 0x35, 0x30, /*  Pro 850  000002f0 */
0x30, 0x20, 0x41, 0x39, 0x31, 0x30, 0x29, 0x0b, /* 0 A910).  000002f8 */
0x70, 0x72, 0x69, 0x6f, 0x72, 0x69, 0x74, 0x79, /* priority           */
0x3d, 0x33, 0x30, 0x1f, 0x61, 0x64, 0x6d, 0x69, /* =30.admi  00000000 */
0x6e, 0x75, 0x72, 0x6c, 0x3d, 0x68, 0x74, 0x74, /* nurl=htt  00000000 */
0x70, 0x3a, 0x2f, 0x2f, 0x48, 0x50, 0x33, 0x32, /* p://HP32  00000000 */
0x42, 0x44, 0x36, 0x44, 0x2e, 0x6c, 0x6f, 0x63, /* BD6D.loc  00000000 */
0x61, 0x6c, 0x2e, 0x05, 0x6e, 0x6f, 0x74, 0x65, /* al..note  00000000 */
0x3d, 0x07, 0x43, 0x6f, 0x6c, 0x6f, 0x72, 0x3d, /* =.Color=  00000000 */
0x54, 0x08, 0x44, 0x75, 0x70, 0x6c, 0x65, 0x78, /* T.Duplex  00000000 */
0x3d, 0x54, 0x06, 0x53, 0x63, 0x61, 0x6e, 0x3d, /* =T.Scan=  00000000 */
0x54, 0xc0, 0xa6, 0x00, 0x21, 0x80, 0x01, 0x00, /* T...!...  00000000 */
0x00, 0x00, 0x78, 0x00, 0x08, 0x00, 0x00, 0x00, /* ..x.....  00000000 */
0x00, 0x02, 0x77, 0xc1, 0x09, 0xc0, 0xa6, 0x00, /* ..w.....  00000000 */
0x10, 0x80, 0x01, 0x00, 0x00, 0x11, 0x94, 0x01, /* ........  00000000 */
0x23, 0x09, 0x74, 0x78, 0x74, 0x76, 0x65, 0x72, /* #.txtver  00000000 */
0x73, 0x3d, 0x31, 0x08, 0x71, 0x74, 0x6f, 0x74, /* s=1.qtot  00000000 */
0x61, 0x6c, 0x3d, 0x31, 0x2f, 0x70, 0x64, 0x6c, /* al=1/pdl  00000000 */
0x3d, 0x61, 0x70, 0x70, 0x6c, 0x69, 0x63, 0x61, /* =applica  00000000 */
0x74, 0x69, 0x6f, 0x6e, 0x2f, 0x76, 0x6e, 0x64, /* tion/vnd  00000000 */
0x2e, 0x68, 0x70, 0x2d, 0x50, 0x43, 0x4c, 0x2c, /* .hp-PCL,  00000000 */
0x69, 0x6d, 0x61, 0x67, 0x65, 0x2f, 0x75, 0x72, /* image/ur  00000000 */
0x66, 0x2c, 0x69, 0x6d, 0x61, 0x67, 0x65, 0x2f, /* f,image/  00000000 */
0x6a, 0x70, 0x65, 0x67, 0x0e, 0x72, 0x70, 0x3d, /* jpeg.rp=  00000000 */
0x69, 0x70, 0x70, 0x2f, 0x70, 0x72, 0x69, 0x6e, /* ipp/prin  00000000 */
0x74, 0x65, 0x72, 0x46, 0x55, 0x52, 0x46, 0x3d, /* terFURF=  00000000 */
0x43, 0x50, 0x31, 0x2c, 0x4d, 0x54, 0x31, 0x2d, /* CP1,MT1-  00000000 */
0x32, 0x2d, 0x38, 0x2d, 0x39, 0x2d, 0x31, 0x30, /* 2-8-9-10  00000000 */
0x2d, 0x31, 0x31, 0x2c, 0x4f, 0x42, 0x39, 0x2c, /* -11,OB9,  00000000 */
0x4f, 0x46, 0x55, 0x30, 0x2c, 0x50, 0x51, 0x33, /* OFU0,PQ3  00000000 */
0x2d, 0x34, 0x2d, 0x35, 0x2c, 0x52, 0x53, 0x33, /* -4-5,RS3  00000000 */
0x30, 0x30, 0x2d, 0x36, 0x30, 0x30, 0x2c, 0x53, /* 00-600,S  00000000 */
0x52, 0x47, 0x42, 0x32, 0x34, 0x2c, 0x57, 0x38, /* RGB24,W8  00000000 */
0x2c, 0x44, 0x4d, 0x33, 0x2c, 0x49, 0x53, 0x31, /* ,DM3,IS1  00000000 */
0x2d, 0x32, 0x1a, 0x74, 0x79, 0x3d, 0x4f, 0x66, /* -2.ty=Of  00000000 */
0x66, 0x69, 0x63, 0x65, 0x6a, 0x65, 0x74, 0x20, /* ficejet   00000000 */
0x50, 0x72, 0x6f, 0x20, 0x38, 0x35, 0x30, 0x30, /* Pro 8500  00000000 */
0x20, 0x41, 0x39, 0x31, 0x30, 0x24, 0x70, 0x72, /*  A910$pr  00000000 */
0x6f, 0x64, 0x75, 0x63, 0x74, 0x3d, 0x28, 0x48, /* oduct=(H  00000000 */
0x50, 0x20, 0x4f, 0x66, 0x66, 0x69, 0x63, 0x65, /* P Office  00000000 */
0x6a, 0x65, 0x74, 0x20, 0x50, 0x72, 0x6f, 0x20, /* jet Pro   00000000 */
0x38, 0x35, 0x30, 0x30, 0x20, 0x41, 0x39, 0x31, /* 8500 A91  00000000 */
0x30, 0x29, 0x0b, 0x70, 0x72, 0x69, 0x6f, 0x72, /* 0).prior  00000000 */
0x69, 0x74, 0x79, 0x3d, 0x36, 0x30, 0x1f, 0x61, /* ity=60.a  00000000 */
0x64, 0x6d, 0x69, 0x6e, 0x75, 0x72, 0x6c, 0x3d, /* dminurl=  00000000 */
0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x48, /* http://H  00000000 */
0x50, 0x33, 0x32, 0x42, 0x44, 0x36, 0x44, 0x2e, /* P32BD6D.  00000000 */
0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x2e, 0x05, 0x6e, /* local..n  00000000 */
0x6f, 0x74, 0x65, 0x3d, 0x07, 0x43, 0x6f, 0x6c, /* ote=.Col  00000000 */
0x6f, 0x72, 0x3d, 0x54, 0x08, 0x44, 0x75, 0x70, /* or=T.Dup  00000000 */
0x6c, 0x65, 0x78, 0x3d, 0x54, 0x06, 0x53, 0x63, /* lex=T.Sc  00000000 */
0x61, 0x6e, 0x3d, 0x54, 0xc0, 0xe2, 0x00, 0x21, /* an=T...!  00000000 */
0x80, 0x01, 0x00, 0x00, 0x00, 0x78, 0x00, 0x08, /* .....x..  00000000 */
0x00, 0x00, 0x00, 0x00, 0x1f, 0x90, 0xc1, 0x09, /* ........  00000000 */
0xc0, 0xe2, 0x00, 0x10, 0x80, 0x01, 0x00, 0x00, /* ........  00000000 */
0x11, 0x94, 0x00, 0x8a, 0x09, 0x74, 0x78, 0x74, /* .....txt  00000000 */
0x76, 0x65, 0x72, 0x73, 0x3d, 0x31, 0x1a, 0x74, /* vers=1.t  00000000 */
0x79, 0x3d, 0x4f, 0x66, 0x66, 0x69, 0x63, 0x65, /* y=Office  00000000 */
0x6a, 0x65, 0x74, 0x20, 0x50, 0x72, 0x6f, 0x20, /* jet Pro   00000000 */
0x38, 0x35, 0x30, 0x30, 0x20, 0x41, 0x39, 0x31, /* 8500 A91  00000000 */
0x30, 0x06, 0x6d, 0x66, 0x67, 0x3d, 0x48, 0x50, /* 0.mfg=HP  00000000 */
0x1b, 0x6d, 0x64, 0x6c, 0x3d, 0x4f, 0x66, 0x66, /* .mdl=Off  00000000 */
0x69, 0x63, 0x65, 0x6a, 0x65, 0x74, 0x20, 0x50, /* icejet P  00000000 */
0x72, 0x6f, 0x20, 0x38, 0x35, 0x30, 0x30, 0x20, /* ro 8500   00000000 */
0x41, 0x39, 0x31, 0x30, 0x1f, 0x61, 0x64, 0x6d, /* A910.adm  00000000 */
0x69, 0x6e, 0x75, 0x72, 0x6c, 0x3d, 0x68, 0x74, /* inurl=ht  00000000 */
0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x48, 0x50, 0x33, /* tp://HP3  00000000 */
0x32, 0x42, 0x44, 0x36, 0x44, 0x2e, 0x6c, 0x6f, /* 2BD6D.lo  00000000 */
0x63, 0x61, 0x6c, 0x2e, 0x05, 0x6e, 0x6f, 0x74, /* cal..not  00000000 */
0x65, 0x3d, 0x08, 0x62, 0x75, 0x74, 0x74, 0x6f, /* e=.butto  00000000 */
0x6e, 0x3d, 0x54, 0x09, 0x66, 0x6c, 0x61, 0x74, /* n=T.flat  00000000 */
0x62, 0x65, 0x64, 0x3d, 0x54, 0x08, 0x66, 0x65, /* bed=T.fe  00000000 */
0x65, 0x64, 0x65, 0x72, 0x3d, 0x54              /* eder=T */
};


