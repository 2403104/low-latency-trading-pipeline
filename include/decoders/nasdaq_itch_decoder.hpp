#pragma once 
#include "core/event_format.hpp"
#include <cstring>
#include <cstdint>

namespace hft {
  class NasdaqItchDecoder {
  public:
    /**
     * @brief Decodes a raw NASDAQ ITCH 5.0 Add Order Message (Type 'A')
     * @param buffer Pointer to the raw packet bytes arriving from the network/file
     * @param length Length of the received packet buffer
     * @param receive_ts Core 2's local hardware/software capture timestamp
     * @param out_event The 64-byte unified MarketEvent struct to populate
     * @return true if successfully parsed, false otherwise
     */
     static inline bool decode(const uint8_t* buffer, size_t length, uint64_t receive_ts, MarketEvent& out_event) {
      if (length == 0 || !buffer) return false;
      
      out_event.receive_ts = receive_ts;
      out_event.exchange_id = 1;
      
      const char msg_type = static_cast<char>(buffer[0]);
      switch (msg_type) {
        case 'A' : {
          if(length < 36) return false;
          out_event.type = MarketEventType::OrderAdd;

          out_event.security_id = __builtin_bswap16(read_unaligned<uint16_t>(buffer + 1));
          out_event.exchange_ts = parse_48bit_ts(buffer + 5);
          out_event.order.order_id = __builtin_bswap64(read_unaligned<uint64_t>(buffer + 11));
          out_event.side = (buffer[19] == 'B') ? Side::Buy : Side::Sell;
          out_event.order.quantity = __builtin_bswap32(read_unaligned<uint32_t>(buffer + 20));
          out_event.order.price = static_cast<int64_t>(__builtin_bswap32(read_unaligned<uint32_t>(buffer + 32)));
          return true;
        }
        default: 
          return false;
      }
     }
    private:
      template <typename T> 
      static inline T read_unaligned(const uint8_t* ptr) {
        T val;
        std::memcpy(&val, ptr, sizeof(T));
        return val;
      }
      static inline uint64_t parse_48bit_ts(const uint8_t* buffer) {
        uint64_t nanos = 0;
        nanos |= static_cast<uint64_t>(buffer[0]) << 40;
        nanos |= static_cast<uint64_t>(buffer[1]) << 32;
        nanos |= static_cast<uint64_t>(buffer[2]) << 24;
        nanos |= static_cast<uint64_t>(buffer[3]) << 16;
        nanos |= static_cast<uint64_t>(buffer[4]) << 8;
        nanos |= static_cast<uint64_t>(buffer[5]);
        return nanos;
      }
  };
}