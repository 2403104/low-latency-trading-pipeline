#pragma once
#include <atomic>
#include <cstddef>
#include <cassert>

namespace hft {
  /**
  * @brief Lock-free Single-Producer Single-Consumer (SPSC) Ring Buffer.
  * @tparam T The type of the data structure
  * @tparam SIZE Must be a power of 2 for bitwise index wrapping.
  */
  template <typename T, size_t SIZE>
  class RingBuffer {
  public:

    static_assert((SIZE != 0) && ((SIZE & (SIZE - 1)) == 0), "RingBuffer size must be a power of 2.");

    RingBuffer() = default;
    ~RingBuffer() = default;

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    inline bool push(const T& data) {
      
      const size_t current_write = write_index_.load(std::memory_order_relaxed);
      const size_t next_write = (current_write + 1) & (SIZE - 1);
      
      if(next_write == read_index_.load(std::memory_order_acquire)) {
        return false;
      
      }
      
      buffer_[current_write] = data;
      write_index_.store(next_write, std::memory_order_release);
      
      return true;
    }

    inline bool pop(T& data) {
      const size_t current_read = read_index_.load(std::memory_order_relaxed);

      if(current_read == write_index_.load(std::memory_order_acquire)) {
        return false;
      }
      
      data = buffer_[current_read];
      
      const size_t next_read = (current_read + 1) & (SIZE - 1);
      read_index_.store(next_read, std::memory_order_release);
      
      return true;
    }

    bool empty() const {
      return read_index_.load(std::memory_order_acquire) == write_index_.load(std::memory_order_acquire);
    }

    size_t size() const {
      return (write_index_.load(std::memory_order_acquire) - read_index_.load(std::memory_order_acquire)) & (SIZE - 1);
    }

  private:
    alignas(64) std::atomic<size_t> write_index_{0};
    alignas(64) std::atomic<size_t> read_index_{0};
    alignas(64) T buffer_[SIZE];
  }; 

};