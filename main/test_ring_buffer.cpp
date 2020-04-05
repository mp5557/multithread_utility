#include <iostream>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <ring_buffer.h>

int main() {
   RingBuffer<int> ring_buffer(3);

   ring_buffer.Push(1);
   fmt::print("after push 1: {}\n", fmt::join(ring_buffer, ","));

   ring_buffer.Push(2);
   fmt::print("after push 2: {}\n", fmt::join(ring_buffer, ","));

   ring_buffer.Push(3);
   fmt::print("after push 3: {}\n", fmt::join(ring_buffer, ","));

   ring_buffer.Push(4);
   fmt::print("after push 4: {}\n", fmt::join(ring_buffer, ","));

   auto iter = std::find_if(ring_buffer.begin(), ring_buffer.end(),
                           [](int x) { return x > 2; });
   fmt::print("find iter {}\n", *iter);

   ring_buffer.Remove(iter);
   fmt::print("after remove: {}\n", fmt::join(ring_buffer, ","));
}
