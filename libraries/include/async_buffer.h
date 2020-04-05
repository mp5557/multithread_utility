#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

#include "ring_buffer.h"

template <typename T>
class AsyncBuffer {
 public:
  using Type = T;
  using Buffer = RingBuffer<T>;

  enum ErrorCode { kSuccess, kTimeout };

  explicit AsyncBuffer(int max_size = 3) : buffer_(max_size) {}

  ErrorCode PushForce(T item) {
    std::unique_lock<std::mutex> lock(mutex_);
    buffer_.Push(std::move(item));
    lock.unlock();
    cond_.notify_one();
    return kSuccess;
  }

  ErrorCode Push(T item, int timeout_ms) {
    std::unique_lock<std::mutex> lock(mutex_);
    bool flag = cond_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                               [this]() { return !Full(); });

    if (flag) {
      buffer_.Push(std::move(item));
      lock.unlock();
      cond_.notify_one();
    }

    return flag ? kSuccess : kTimeout;
  }

  ErrorCode Pop(int timeout_ms, T &item) {
    std::unique_lock<std::mutex> lock(mutex_);
    bool flag = cond_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                               [this]() { return !Empty(); });
    if (flag) {
      buffer_.Pop(item);

      lock.unlock();
      cond_.notify_one();
      return kSuccess;
    }
    return kTimeout;
  }

  bool Full() const { return buffer_.Full(); }
  bool Empty() const { return buffer_.Empty(); }

 private:
  std::condition_variable cond_;
  std::mutex mutex_;
  Buffer buffer_;
};
