#pragma once

template <size_t... indices>
struct IndicesHolder {};

template <size_t requested_index, size_t... indices>
struct IndicesGenerator {
  using Type = typename IndicesGenerator<requested_index - 1,
                                         requested_index - 1, indices...>::Type;
};

template <size_t... indices>
struct IndicesGenerator<0, indices...> {
  using Type = IndicesHolder<indices...>;
};

template <int N>
using IndicesGeneratorT = typename IndicesGenerator<N>::Type;

template <typename... Ts>
inline void Fn(Ts&&...) {}

#define Pr(x) Fn((x, 0)...)

template <typename Op, typename T, typename Tuple, size_t... I>
inline T ReduceImpl(Op op, T res, Tuple const &a, IndicesHolder<I...>) {
  Fn(res = op(res, std::get<I>(a))...);
  return res;
}

template <typename Op, typename T, typename Tuple>
inline T Reduce(Op op, T res, Tuple const &a) {
  return ReduceImpl(op, res, a, IndicesGeneratorT<std::tuple_size_v<Tuple>>());
}

template <typename T>
struct OpAdd {
  T operator()(T const &x, T const &y) const { return x + y; }
};

template <typename T>
struct OpAnd {
  T operator()(T x, T y) const { return x && y; }
};

template <typename T>
struct OpMin {
  T operator()(T const &x, T const &y) const { return x < y ? x : y; }
};

template <typename T>
struct OpMax {
  T operator()(T const &x, T const &y) const { return x < y ? y : x; }
};
