#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <tuple>

#include "async_buffer.h"
#include "index_generator.h"

namespace detail {
template <typename Function>
struct FunctionTraits : public FunctionTraits<decltype(&Function::operator())> {
};

template <typename ClassType, typename ReturnType, typename... Args>
struct FunctionTraits<ReturnType (ClassType::*)(Args...) const> {
  using ArgTypes = std::tuple<typename std::decay<Args>::type...>;
};

template <typename F, typename Tuple, size_t... I>
void InvokeTupleImple(F f, Tuple&& args, IndicesHolder<I...>) {
  f(std::get<I>(std::forward<Tuple>(args))...);
}

template <typename F, typename Tuple>
void InvokeTuple(F f, Tuple&& args) {
  constexpr size_t N = std::tuple_size<Tuple>::value;
  InvokeTupleImple(f, std::forward<Tuple>(args), IndicesGeneratorT<N>());
}
}  // namespace detail

template <typename F, typename T>
class AsyncCaller {};

template <typename F, typename... Tuples>
class AsyncCaller<F, std::tuple<Tuples...>> {
  F f_;
  AsyncBuffer<std::tuple<Tuples...>> buffer_;
  std::thread thread_;
  std::atomic_bool exit_flag_;

  void Run() {
    std::tuple<Tuples...> item;
    while (!exit_flag_) {
      if (buffer_.Pop(50, item) == buffer_.kSuccess) {
        detail::InvokeTuple(f_, std::move(item));
      }
    }
  }

 public:
  AsyncCaller(F f) : f_(f) {
    std::atomic_init(&exit_flag_, false);
    thread_ = std::thread(&AsyncCaller::Run, this);
  }

  ~AsyncCaller() {
    exit_flag_ = true;
    if (thread_.joinable()) thread_.join();
  }

  template <typename... T>
  void Call(T&&... args) {
    buffer_.PushForce(std::make_tuple(std::forward<T>(args)...));
  }
};

template <typename F>
auto MakeAsyncCaller(F f) {
  using Args = detail::FunctionTraits<F>::ArgTypes;
  using Caller = AsyncCaller<decltype(f), Args>;
  return std::unique_ptr<Caller>(new Caller(f));
}

/*

#include <iostream>

#include "async_caller.h"

int main() {
  auto caller = MakeAsyncCaller(
      [](int const &x, int y) { std::cout << x + y << std::endl; });

  int x, y;
  while (std::cin >> x >> y) caller->Call(x, y);
}
*/
