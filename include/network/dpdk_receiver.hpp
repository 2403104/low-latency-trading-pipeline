#pragma once

#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <cstring>
#include <rte_ethdev.h>
#include <rte_mbuf.h>

namespace hft {

class DpdkSource {
public:
  DpdkSource(uint16_t port, uint16_t queue, rte_mempool* pool)
      : port_(port), queue_(queue), current_mbuf_(nullptr) {

    if (!rte_eth_dev_is_valid_port(port_))
      throw std::runtime_error("Port ID is not valid.");

    struct rte_eth_conf port_conf;
    std::memset(&port_conf, 0, sizeof(port_conf));
    port_conf.rxmode.mq_mode = RTE_ETH_MQ_RX_NONE; // mq_mode Tells NIC how to distribute incoming packets across queues.

    int ret = rte_eth_dev_configure(port_, 1, 1, &port_conf); // port_id, cnt of rx_queu, cnt of tx_queue
    if (ret < 0)
      throw std::runtime_error("Failed to configure DPDK device port, error: " + std::to_string(ret));

    ret = rte_eth_rx_queue_setup(port_, queue_, 1024, rte_eth_dev_socket_id(port_), NULL, pool);
    if (ret < 0)
      throw std::runtime_error("Failed to setup RX queue, error: " + std::to_string(ret));

    ret = rte_eth_tx_queue_setup(port_, 0, 1024, rte_eth_dev_socket_id(port_), NULL);
    if (ret < 0)
      throw std::runtime_error("Failed to setup TX queue, error: " + std::to_string(ret));

    ret = rte_eth_dev_start(port_);
    if (ret < 0)
      throw std::runtime_error("Failed to start DPDK device, error: " + std::to_string(ret));

    rte_eth_promiscuous_enable(port_); // Packet arrives with ANY dst MAC NIC accepts it regardless
  }

  size_t next_packet_ptr(const uint8_t*& pkt_ptr) {
    uint16_t n = rte_eth_rx_burst(port_, queue_, &current_mbuf_, 1); // poll to NIC with port_, queue_, (take maximum 1 packet if available)

    if (n == 0) {
      current_mbuf_ = nullptr;
      ++empty_polls_;
      return 0;
    }

    empty_polls_ = 0;

    size_t full_frame_len = rte_pktmbuf_data_len(current_mbuf_);
    packets_received_++;

    if (full_frame_len < 42) {
      small_packet_drops_++;
      rte_pktmbuf_free(current_mbuf_);
      current_mbuf_ = nullptr;
      return 0;
    }

    const uint8_t* raw = rte_pktmbuf_mtod(current_mbuf_, const uint8_t*);
    pkt_ptr = raw + 42;
    return full_frame_len - 42;
  }

  void release_packet() {
    if (current_mbuf_) {
      rte_pktmbuf_free(current_mbuf_);
      current_mbuf_ = nullptr;
    }
  }

  void print_port_stats() const {
    struct rte_eth_stats stats;
    rte_eth_stats_get(port_, &stats);
  }

private:
  uint16_t port_;
  uint16_t queue_;
  rte_mbuf* current_mbuf_;

  uint64_t packets_received_ = 0;
  uint64_t small_packet_drops_ = 0;
  uint64_t empty_polls_ = 0;
};

}