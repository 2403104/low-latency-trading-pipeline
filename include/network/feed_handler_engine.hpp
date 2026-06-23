#pragma once
#include "core/event_format.hpp"
#include "core/ring_buffer.hpp"
#include <chrono>
#include <atomic>
#include <bitset>
#include <iostream>
#include <iomanip> 

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

namespace hft {
  template <typename DataSource, typename ProtocolDecoder, size_t BufferCapacity>
  class FeedHandlerEngine {
  public:
    FeedHandlerEngine(RingBuffer<MarketEvent, BufferCapacity> &rb, DataSource& source) 
      : ring_buffer_(rb)
      , source_(source)
      , is_running_(false) {}

  void start() {
        is_running_.store(true, std::memory_order_release);

        std::cout << "[ENGINE] Ingestion loop successfully running on HW Core ID: " << rte_lcore_id() << std::endl;

        MarketEvent internal_event;
        uint64_t total_raw_frames = 0;

        while(is_running_.load(std::memory_order_relaxed)) {
          const uint8_t* pkt_ptr = nullptr;
          size_t bytes_read = source_.next_packet_ptr(pkt_ptr);
          
          if(bytes_read == 0) {
            continue; 
          }
          
          total_raw_frames++;
          uint64_t receive_ts = rte_rdtsc(); 
          
          bool success = ProtocolDecoder::decode(pkt_ptr, bytes_read, receive_ts, internal_event);

          if(success) {
            while(!ring_buffer_.push(internal_event)) {
              #if defined(__x86_64__) || defined(_M_X64)
              _mm_pause();
              #endif
            }
          }
          source_.release_packet();
        }
      }
    
      void stop() {
        is_running_.store(false, std::memory_order_release);
      }

  private:
    RingBuffer<MarketEvent, BufferCapacity> &ring_buffer_;
    DataSource& source_;
    std::atomic<bool> is_running_;
  };
};