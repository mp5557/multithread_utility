#include <iostream>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <async_caller.h>

int main() {
  auto caller =
      MakeAsyncCaller([](std::unique_ptr<int> ptr, size_t time_stamp) {
        fmt::print("receive: ptr = {}, time_stamp = {}\n", *ptr, time_stamp);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "finished\n";
      });

  size_t time_stamp = 0;
  int x;
  while (std::cin >> x) {
    if (x < 0)
      break;
    caller->Call(std::make_unique<int>(x), time_stamp);
    fmt::print("after call: ptr = {}, time_stamp = {}\n", x, time_stamp);
    time_stamp++;
  }
}
