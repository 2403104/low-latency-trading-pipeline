#include <iostream>
#include <atomic>
#include <memory>
#include <csignal>
#include <bitset>
#include <iomanip>

#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_launch.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

#include "include/network/feed_handler_engine.hpp"
#include "include/core/ring_buffer.hpp"
#include "include/network/dpdk_receiver.hpp"
#include "include/decoders/nasdaq_itch_decoder.hpp"

std::atomic<bool> keep_running{true};

void signal_handler(int signum) {
  if (signum == SIGINT || signum == SIGTERM) {
    std::cout << "\n[MAIN] Termination signal received. Shutting down..." << std::endl;
    keep_running.store(false);
  }
}

int launch_engine_on_core(void* arg) {
    constexpr size_t RING_BUFFER_CAPACITY = 131072;
    using EngineType = hft::FeedHandlerEngine<hft::DpdkSource, hft::NasdaqItchDecoder, RING_BUFFER_CAPACITY>;
    auto* engine_ptr = static_cast<EngineType*>(arg);
    engine_ptr->start();
    return 0;
}
int main(int argc, char* argv[]) {
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  int args_parsed = rte_eal_init(argc, argv); // Bootstraps the entire DPDK environment. It parses command-line arguments (like -l 0-1 to assign cores), reserves massive chunks of RAM (hugepages) to avoid cache misses, and takes control of the network hardware away from the Linux kernel.
  if (args_parsed < 0) {
    std::cerr << "Error with EAL initialization" << std::endl;
    return -1;
  }

  try {
    // Allocates a performance-optimized memory pool for fast, zero-copy packet processing.
    struct rte_mempool* mbuf_pool = rte_pktmbuf_pool_create(
      "MBUF_POOL", 
      16383, 
      256, 
      0, 
      RTE_MBUF_DEFAULT_BUF_SIZE, 
      rte_socket_id()
    );

    if (mbuf_pool == nullptr) {
      throw std::runtime_error("Cannot create mbuf pool");
    }

    uint16_t port_id = 0; // DPDK assigns port_id to each NIC card it owns to
    uint16_t queue_id = 0; // NIC has multiple RX rings sitting in memory. Queue ID is just which ring you're referring to.
    constexpr size_t RING_BUFFER_CAPACITY = 131072;

    auto ring_buffer = std::make_unique<hft::RingBuffer<hft::MarketEvent, RING_BUFFER_CAPACITY>>();
    hft::DpdkSource source(port_id, queue_id, mbuf_pool);
    hft::FeedHandlerEngine<hft::DpdkSource, hft::NasdaqItchDecoder, RING_BUFFER_CAPACITY> engine(*ring_buffer, source);

    unsigned int worker_core = rte_get_next_lcore(-1, 1, 0); // find the next availabe core
    if (worker_core == RTE_MAX_LCORE) {
        throw std::runtime_error("No dedicated worker core found. Ensure you pass '-l 0-1'");
    }

    std::cout << "[MAIN] Launching engine loop onto EAL Worker Core: " << worker_core << std::endl;

    rte_eal_remote_launch(launch_engine_on_core, &engine, worker_core);

    std::cout << "[RUNNING] Pipeline spinning. Monitoring ring buffer metrics..." << std::endl;

    hft::MarketEvent consumed_event;
    uint64_t total_add_orders = 0;

    std::cout << std::left  << std::setw(8)  << "SEQ" << std::setw(18) << "RECEIVE_TS (ns)" << std::setw(18) << "EXCHANGE_TS (ns)" << std::setw(8)  << "SEC_ID" << std::setw(9)  << "EXCH_ID" << std::setw(6)  << "SIDE" << std::setw(12) << "ORDER_ID" << std::setw(12) << "PRICE" << std::setw(10) << "QUANTITY" << std::setw(10) << "PRIORITY"  << "\n";

    std::cout << std::string(111, '-') << "\n";
    while (keep_running.load()) {
      if (ring_buffer->pop(consumed_event)) {
        total_add_orders++;
        if(total_add_orders % 1000000 == 0) {
          std::cout << "Packet Number: " << total_add_orders << std::endl; 
          std::cout << std::left << std::setw(8)  << total_add_orders << std::setw(18) << consumed_event.receive_ts << std::setw(18) << consumed_event.exchange_ts << std::setw(8)  << consumed_event.security_id << std::setw(9)  << consumed_event.exchange_id << std::setw(6)  << (consumed_event.side == hft::Side::Buy ? "BUY" : "SELL") << std::setw(12) << consumed_event.order.order_id << std::setw(12) << consumed_event.order.price << std::setw(10) << consumed_event.order.quantity << std::setw(10) << consumed_event.order.priority  << std::endl;
        }
      } else {
        #if defined(__x86_64__) || defined(_M_X64)
        _mm_pause(); // High-efficiency spinning
        #endif
      }
    }

    engine.stop();
      
    rte_eal_wait_lcore(worker_core); // Wait for core 1 to exit cleanly

  } catch (const std::exception& e) {
      std::cerr << "CRASH: " << e.what() << std::endl;
  }

  rte_eal_cleanup();
  return 0;
}