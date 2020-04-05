#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

#include "ring_buffer.h"

template <typename T, typename TimeStamp = size_t>
class TimedBuffer {
 public:
  using Type = T;
  using Buffer = RingBuffer<std::pair<TimeStamp, T>>;

  enum ErrorCode { kSuccess, kTimeout };

  explicit TimedBuffer(int max_size = 3) : buffer_(max_size) {}

  ErrorCode PushForce(TimeStamp time_stamp, T item) {
    std::unique_lock<std::mutex> lock(mutex_);
    buffer_.Push(std::make_pair(time_stamp, std::move(item)));
    lock.unlock();
    cond_.notify_one();
    return kSuccess;
  }

  ErrorCode Pop(int timeout_ms, TimeStamp &time_stamp, T &item) {
    std::unique_lock<std::mutex> lock(mutex_);
    bool flag = cond_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                               [this]() { return !Empty(); });
    if (flag) {
      std::tie(time_stamp, item) = buffer_.Pop();

      lock.unlock();
      cond_.notify_one();
      return kSuccess;
    }
    return kTimeout;
  }

  typename Buffer::ConstIterator FindByTimestamp(TimeStamp time_stamp) const {
    return std::find_if(buffer_.begin(), buffer_.end(),
                        [time_stamp](Buffer::ValueType const &v) {
                          return v.first >= time_stamp;
                        });
  }

  ErrorCode PopByTimeStamp(int timeout_ms, TimeStamp time_stamp, T &item) {
    std::unique_lock<std::mutex> lock(mutex_);
    Buffer::ConstIterator iter;
    bool flag = cond_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                               [this, time_stamp, &iter]() {
                                 iter = FindByTimestamp(time_stamp);
                                 return iter;
                               });
    if (flag) {
      buffer_.Remove(iter);
      std::tie(time_stamp, item) = buffer_.Pop();

      lock.unlock();
      cond_.notify_one();
      return kSuccess;
    }
    return kTimeout;
  }

  ErrorCode GetByTimeStamp(int timeout_ms, TimeStamp time_stamp, T &item) {
    std::unique_lock<std::mutex> lock(mutex_);
    Buffer::ConstIterator iter;
    bool flag = cond_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                               [this, time_stamp, &iter]() {
                                 iter = FindByTimestamp(time_stamp);
                                 return iter;
                               });
    if (flag) {
      std::tie(time_stamp, item) = *iter;

      lock.unlock();
      cond_.notify_one();
      return kSuccess;
    }
    return kTimeout;
  }

 private:
  Buffer buffer_;
  std::condition_variable cond_;
  std::mutex mutex_;
};
