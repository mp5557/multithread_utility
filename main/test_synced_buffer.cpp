#include <iostream>
#include <tuple>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <synchronized_buffer.h>

int main() {
  SynchronizedBuffer<std::string, int> synced_buffer;

  std::thread t([&]() {
    synced_buffer.PushForce<0>(0, "1+1=");
    fmt::print("time 0 push '1+1='\n");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    synced_buffer.PushForce<0>(1, "2+1=");
    fmt::print("time 1 push '2+1='\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    synced_buffer.PushForce<1>(1, 3);
    fmt::print("time 1 push 3\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    synced_buffer.PushForce<0>(2, "2+3=");
    fmt::print("time 2 push '2+3='\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    synced_buffer.PushForce<1>(2, 5);
    fmt::print("time 2 push 5\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    synced_buffer.PushForce<0>(3, "4+3=");
    fmt::print("time 3 push '4+3='\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    synced_buffer.PushForce<1>(3, 7);
    fmt::print("time 3 push 7\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  });

  std::string str_get;
  int int_get = 0;
  size_t time_stamp = 0;
  while (synced_buffer.PopAvailable(time_stamp, str_get, int_get) != synced_buffer.kSuccess) {
    fmt::print("waiting\n");
  }
  fmt::print("received 1# time_stamp = {}, str_get = '{}', int_get = {}\n", time_stamp, str_get, int_get);

  while (synced_buffer.PopByTimeStamp(SyncAllPolicy<2>(), 3, str_get, int_get) != synced_buffer.kSuccess) {
    fmt::print("waiting\n");
  }
  fmt::print("received 2# str_get = '{}', int_get = {}\n", str_get, int_get);
  t.join();
}
