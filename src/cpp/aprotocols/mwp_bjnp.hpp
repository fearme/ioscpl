
#ifndef __MWP_BJNP_HPP__
#define __MWP_BJNP_HPP__

#include "hp_mwp.h"
#include "mwp_mq.hpp"
#include "mwp_mini_curl.hpp"

namespace net_mobilewebprint {

  struct controller_base_t;
  struct bjnp_connection_t;

  using namespace net_mobilewebprint::mq_enum;

  struct bjnp_t : public mq_select_handler_t, mq_handler_t
  {
    typedef std::map<uint32, bjnp_connection_t *>  connections_t;

    controller_base_t &    controller;
    mq_t &                 mq;
    connections_t          connections;

    map<uint32, uint32>    connection_ids;
    map<uint32, uint32>    http_connection_ids;

    bjnp_t(controller_base_t &);

    bjnp_connection_t *                   discover(uint32 & id, string const & ip, uint16 port = 8611);
    bjnp_connection_t *            send_to_printer(uint32   id, string const & ip, uint16 port = 8611);
    bool                       is_connection_alive(uint32 connection_id);

    virtual int                         pre_select(mq_pre_select_data_t * pre_select_data);
    virtual e_handle_result           _mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra);

    virtual e_handle_result                 handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);

    bjnp_connection_t *                _connection(uint32 & id, string const & ip, uint16 port = 8611);
    bool                      connection_is_closed(connections_t::iterator & it);

    uint32                       connection_id_for(uint32 http_connection_id);
    uint32                       http_connection_id_for(bjnp_connection_t const * connection);

    virtual string  mod_name();
  };

  // Bookkeeping for each data chunk we send
  struct bjnp_chunk_t {

    bool        have_result;
    bool        known_failed;
    uint16      seq_num;
    uint16      that_seq_num;
    uint16      resend_seq_num;

    buffer_t *  data;
    uint16      type1;
    uint16      type2;

    bool        is_udp;

    uint32      time;

    bjnp_chunk_t(buffer_t * data, uint16 seq_num, uint16 type1, uint16 type2, bool is_udp);
    bjnp_chunk_t(bjnp_chunk_t const & that);
    ~bjnp_chunk_t();

    uint32 time_since();
  };

  struct bjnp_connection_t : public mq_select_handler_t, mq_handler_t
  {
    controller_base_t &  controller;
    mq_t &               mq;

    enum e_payload_type { xml_payload = 4, cmd_payload, bits_payload, job_status };

    network_node_t *     udp_printer;

    bool                 tcp_is_open;
    network_node_t *     printer;
    uint32               connection_id;
    uint16               bjnp_job_num;
    uint16               bjnp_seq_num;
    uint32               bjnp_payload_size;
    uint16               curr_seq_num;

    // Timeouts for watching the progression of the work
    handler_holder_by_id_t *    watchdog_holder;
    int                         watchdog_timeout;
    int                         watchdog_num;
    bool                        discover_is_in_flight;
    bool                        send_job_is_in_flight;
    uint32                      last_tcp_status;

    int                         packet_num;
    std::deque<bjnp_chunk_t *>  udp_packets;
    std::deque<bjnp_chunk_t *>  chunks;
    bjnp_chunk_t *              special_tcp_chunk;

    // Buffer that holds the arriving data
    buffer_t                    buildout_buffer;
    e_payload_type              payload_type;         // The payload type for whats in buildout_buffer

    bjnp_connection_t(controller_base_t &, network_node_t *, network_node_t *udp, uint32 id);
    virtual ~bjnp_connection_t();

    // The API for bjnp_connection_t
    void  discover();
    void  send_to_printer();

    // Functions to be an mwp handler
    virtual string                 mod_name();

    virtual int                      pre_select(mq_pre_select_data_t * pre_select_data);
    virtual e_handle_result        _mq_selected(string const & name, buffer_view_i const & payload, buffer_t * data, mq_select_handler_extra_t & extra);

    virtual e_handle_result              handle(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);
            e_handle_result _on_bjnp_udp_packet(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);
            e_handle_result    _on_http_payload(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);
            e_handle_result       _on_txn_close(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);

            e_handle_result      _bjnp_watchdog(string const & name, buffer_view_i const & payload, buffer_t * data, mq_handler_extra_t & extra);

    // Management of the close action
    bool                 is_done() const;
    bool                 closed;
    bool                 printer_ackd_end_job;
    bool                 txn_closed;

    // Management of packets to send
    //
    // Note: "packet_by_seq" is the collection that holds the data -- "chunks" and "udp_packets" just hold weak pointers.
    //       This is true even with the "special_tcp_chunk".
    //
    std::map<uint16, bjnp_chunk_t *> packet_by_seq;
    bjnp_connection_t & push_back_udp(buffer_t * packet, uint16 type1, uint16 type2);
    bjnp_connection_t & push_back_tcp(buffer_view_i const & data, int begin_offset, int end_offset, uint16 type1, uint16 type2);
    bjnp_connection_t & result_found(buffer_view_i const & payload, bool is_udp);

    bjnp_chunk_t      * tcp_chunk(byte command, buffer_view_i const & data, int begin_offset, int end_offset, uint16 type1, uint16 type2);
    bjnp_connection_t & relabel_chunk(bjnp_chunk_t * chunk, uint16 seq_num);

    bjnp_chunk_t      * next_tcp_chunk(bool remove = false);

    // Helpers
    int             _parse_packet(buffer_view_i const & packet, uint16 & seq_num, uint16 & job_num, uint32 & payload_size, uint32 & num_recvd);
    bjnp_chunk_t *  resend_of(bjnp_chunk_t *);
  };

  namespace bjnp {
    buffer_t discover_pkt(uint16 & job_num, uint16 & seq_num);
    buffer_t get_id_pkt(uint16 & job_num, uint16 & seq_num);
  
    buffer_t start_job_pkt(string host, string user, string name, uint16 & job_num, uint16 & seq_num);
    buffer_t payload_size_pkt(uint16 & job_num, uint16 & seq_num);
    buffer_t status_pkt(uint16 & job_num, uint16 & seq_num);

    bool     parse_discover_response(buffer_view_i const & packet, string & mac_out, string & ip_out);
    bool     parse_identity_response(buffer_view_i const & packet, string & device_str_out);
    bool     parse_print_details_response(buffer_view_i const & packet, uint16 & bjnp_short_job_id, uint32 & bjnp_job_num);
    bool     parse_payload_size_response(buffer_view_i const & packet, uint32 & payload_size);

    buffer_range_view_t   payload_of(buffer_view_i const & that);

    buffer_t packet(byte command, uint16 & job_num, uint16 & seq_num, uint32 extra_byte_count = 0);

    uint16   get_seq_num(buffer_view_i const & packet);
    byte     get_command_id(buffer_view_i const & packet);
  };

};

#endif  // __MWP_BJNP_HPP__


