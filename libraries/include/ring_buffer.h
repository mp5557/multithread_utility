#pragma once

#include <assert.h>
#include <iterator>
#include <type_traits>
#include <vector>

namespace detail {
template <typename RB, bool is_const>
class RingBufferIterator {
 public:
  // Iterator required types
  using value_type = typename RB::ValueType;
  using pointer =
      typename std::conditional<is_const, const value_type, value_type>::type *;
  using reference =
      typename std::conditional<is_const, const value_type, value_type>::type &;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::forward_iterator_tag;

  using Type = RingBufferIterator<RB, is_const>;

  using RingBufferPointer =
      typename std::conditional<is_const, RB const *, RB *>::type;

  RingBufferIterator() = default;
  RingBufferIterator(const RingBufferIterator &) = default;
  RingBufferIterator(RingBufferPointer buffer, int index)
      : buffer_(buffer), index_(index) {}

  template <typename = std::enable_if<is_const>>
  RingBufferIterator(const RingBufferIterator<RB, false> &iter)
      : buffer_(iter.buffer_), index_(iter.index_) {}

  pointer operator->() const { return &buffer_->data_[index_]; }
  reference operator*() const { return buffer_->data_[index_]; }
  Type &operator++() {
    index_ = buffer_->Next(index_);
    return *this;
  }
  Type operator++(int) {
    Type r(*this);
    ++*this;
    return r;
  }
  template <bool C>
  inline bool operator!=(RingBufferIterator<RB, C> const &rhs) const {
    return index_ != rhs.index_;
  }
  operator bool() const {
    return buffer_ && index_ != buffer_->head_;
  }

  friend RB;
  friend class RingBufferIterator<RB, !is_const>;

 private:
  RingBufferPointer buffer_ = nullptr;
  int index_;
};
}  // namespace detail

template <typename T>
class RingBuffer {
 public:
  using ValueType = T;
  using Type = RingBuffer<T>;
  using Iterator = detail::RingBufferIterator<Type, false>;
  using ConstIterator = detail::RingBufferIterator<Type, true>;

  friend class detail::RingBufferIterator<Type, false>;
  friend class detail::RingBufferIterator<Type, true>;

  explicit RingBuffer(int capacity = 5) : max_length_(capacity + 1) {}

  int Next(int index) const { return (index + 1) % max_length_; }
  int Prev(int index) const { return (index - 1 + max_length_) % max_length_; }

  Iterator begin() { return Iterator(this, tail_); }
  ConstIterator begin() const { return ConstIterator(this, tail_); }
  Iterator end() { return Iterator(this, head_); }
  ConstIterator end() const { return ConstIterator(this, head_); }
  ConstIterator cbegin() const { return ConstIterator(this, tail_); }
  ConstIterator cend() const { return ConstIterator(this, head_); }

  ValueType &Front() { return data_[tail_]; }
  ValueType const &Front() const { return data_[tail_]; }
  ValueType &Back() { return data_[Next(head_)]; }
  ValueType const &Back() const { return data_[Next(head_)]; }
  bool Full() const { return Next(head_) == tail_; }
  bool Empty() const { return head_ == tail_; }
  int Capacity() const { return max_length_ - 1; };
  int Size() const { (head_ + max_length_ - tail_) % max_length_; };
  void Remove(ConstIterator const &iter) { tail_ = iter.index_; }

  T Pop() {
    assert(!Empty());

    auto &item = data_[tail_];
    tail_ = Next(tail_);
    return std::move(item);
  }
  void Pop(T &item) { item = Pop(); }
  void Push(T item) {
    if (Full()) Pop();
    if (data_.size() < max_length_)
      data_.push_back(std::move(item));
    else
      data_[head_] = std::move(item);
    head_ = Next(head_);
  }

  bool SetCapacity(int capacity) {
    if (data_.size() < max_length_) {
      max_length_ = capacity + 1;
      return true;
    }
    return false;
  }

 private:
  std::vector<T> data_;
  int head_ = 0;
  int tail_ = 0;
  int max_length_ = 10;
};
