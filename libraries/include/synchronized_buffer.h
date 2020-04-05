#pragma once

#include <algorithm>
#include <array>
#include <condition_variable>
#include <thread>
#include <tuple>

#include "index_generator.h"
#include "ring_buffer.h"

template <typename Buffer, typename TimeStamp>
typename Buffer::ConstIterator FindByTimestamp(Buffer const &buffer,
                                               TimeStamp time_stamp) {
  return std::find_if(buffer.begin(), buffer.end(),
                      [time_stamp](Buffer::ValueType const &v) {
                        return v.first >= time_stamp;
                      });
}

template <int kSize>
struct SyncAllPolicy {
  template <typename T>
  bool operator()(T const &data) {
    return Reduce(OpAnd<bool>(), true, data);
  }
};

template <typename... Tuples>
class SynchronizedBuffer {
 public:
  using TimeStamp = size_t;
  using Types = std::tuple<Tuples...>;
  using Buffers = std::tuple<RingBuffer<std::pair<TimeStamp, Tuples>>...>;
  static constexpr int kSize = sizeof...(Tuples);

  template <int N>
  using EleType = std::tuple_element_t<N, Types>;

  enum ErrorCode { kSuccess, kTimeout };

  template <int N>
  ErrorCode PushForce(TimeStamp time_stamp, EleType<N> item) {
    std::unique_lock<std::mutex> lock(mutex_);
    std::get<N>(buffers_).Push(std::make_pair(time_stamp, std::move(item)));
    lock.unlock();
    cond_.notify_one();
    return kSuccess;
  }

  template <typename Policy, typename... Ts>
  ErrorCode PopByTimeStamp(Policy policy, TimeStamp time_stamp,
                           Ts &... tuples) {
    return PopByTimeStampImpl(IndicesGeneratorT<kSize>(), policy, time_stamp,
                              500, tuples...);
  }

  template <typename... Ts>
  ErrorCode PopAvailable(TimeStamp &time_stamp, Ts &... tuples) {
    return PopAvailableImpl(IndicesGeneratorT<kSize>(), 500, time_stamp,
                            tuples...);
  }

 private:
  std::array<bool, kSize> sync_flag_;
  std::condition_variable cond_;
  std::mutex mutex_;
  Buffers buffers_;

  template <size_t... I, typename... Ts>
  ErrorCode PopAvailableImpl(IndicesHolder<I...>, int timeout_ms,
                             TimeStamp &time_stamp, Ts &... tuples) {
    std::unique_lock<std::mutex> lock(mutex_);
    using IterList =
        decltype(std::make_tuple(std::get<I>(buffers_).cbegin()...));
    IterList iter_list;
    TimeStamp max_time_stamp = 0;
    bool flag =
        cond_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [&]() {
          bool empty_flag = true;
          Fn(empty_flag = empty_flag && std::get<I>(buffers_).Empty()...);
          if (empty_flag) return false;

          max_time_stamp = 0;
          Fn(max_time_stamp =
                 !std::get<I>(buffers_).Empty() &&
                         std::get<I>(buffers_).Front().first > max_time_stamp
                     ? std::get<I>(buffers_).Front().first
                     : max_time_stamp...);
          Fn(std::get<I>(iter_list) =
                 FindByTimestamp(std::get<I>(buffers_), max_time_stamp)...);
          Pr(std::get<I>(buffers_).Remove(std::get<I>(iter_list)));
          Fn(sync_flag_[I] =
                 std::get<I>(iter_list) &&
                 std::get<I>(iter_list)->first == max_time_stamp...);
          bool sync_flag = true;
          Fn(sync_flag = sync_flag && sync_flag_[I]...);
          return sync_flag;
        });

    if (flag) {
      Fn(tuples = std::move(std::get<I>(buffers_).Pop().second)...);
      time_stamp = max_time_stamp;

      lock.unlock();
      cond_.notify_one();
      return kSuccess;
    }
    return kTimeout;
  }

  template <typename SyncPolicy, size_t... I, typename... Ts>
  ErrorCode PopByTimeStampImpl(IndicesHolder<I...>, SyncPolicy policy,
                               TimeStamp time_stamp, int timeout_ms,
                               Ts &... tuples) {
    std::unique_lock<std::mutex> lock(mutex_);
    using IterList =
        decltype(std::make_tuple(std::get<I>(buffers_).cbegin()...));
    IterList iter_list;
    bool flag =
        cond_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [&]() {
          Fn(std::get<I>(iter_list) =
                 FindByTimestamp(std::get<I>(buffers_), time_stamp)...);
          Fn(sync_flag_[I] = std::get<I>(iter_list) &&
                             std::get<I>(iter_list)->first == time_stamp...);
          return policy(sync_flag_);
        });
    if (flag) {
      Pr(std::get<I>(buffers_).Remove(std::get<I>(iter_list)));
      Fn(tuples = std::move(std::get<I>(buffers_).Pop().second)...);

      lock.unlock();
      cond_.notify_one();
      return kSuccess;
    }
    return kTimeout;
  }
};
