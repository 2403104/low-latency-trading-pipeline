#pragma once
#include <cstdint>

namespace hft {

  enum class MarketEventType : uint8_t {
    OrderAdd = 1,
    OrderModify = 2,
    OrderDelete = 3,
    Trade = 4,
    BookSnapshot = 5,
    SystemEvent = 6
  };

  enum class Side : uint8_t {
    Buy = 1, 
    Sell = 2, 
    None = 0
  };

  struct alignas(64) MarketEvent {
    uint64_t receive_ts;  // Local NIC hardware packet receive time (ns)
    uint64_t exchange_ts; // Exchane matching engine execution time (ns)
    uint32_t security_id; // Unique internal asset identifier
    uint16_t exchange_id; // Unique internal exchange identifer
    MarketEventType type;
    Side side;
    uint64_t padding0;    // making alignment visible and intentional rather than hidden.
  
    union {

      // Used for Type: OrderAdd, OrderModify, OrderDelete
      struct {
        uint64_t order_id;
        int64_t price;
        uint32_t quantity;
        uint32_t priority;
      } order;

      // Used for Type: Trade
      struct {
        uint64_t trade_id;
        int64_t price;
        uint32_t quantity;
        uint32_t flags; // Aggressive of Passive Indicator
      } trade;

      // Used for Type: SystemEvent
      struct {
        uint8_t status; // 1 = Halted, 2 = Resumed, 3 = Pre-Open
        uint8_t padding[23];
      } system;
    };
    
    // Cache line termination (8 Bytes)
    uint64_t reserved_padding;
  };

  // Compile-time assertion ensures no compiler leaks memory alignment boundaries
  static_assert(sizeof(MarketEvent) == 64, "MarketEvent must be exactly 64 bytes to fit perfectly on one cache line.");
};

