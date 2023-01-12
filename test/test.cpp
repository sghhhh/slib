/*
 * @Author: s
 * @Date: 2022-12-03 16:48:46
 * @LastEditTime: 2023-01-12 02:04:51
 * @LastEditors: s
 * @Description:
 * @FilePath: /slib/test/test.cpp
 */
#include <fmt/core.h>

#include <algorithm.hpp>
#include <algorithm>
#include <functional>
#include <list>
#include <memory>
#include <stack>
#include <threadpool.hpp>

#include "LockFree/stack.hpp"
using namespace slib;

template <typename T, typename cmp = std::greater<T>>
bool test(T _v1, T _v2) {
  return cmp()(_v1, _v2);
}

int main(int argc, char* argv[]) {
  std::list<int> l{5,  61, 651, 65, 16,  541, 1, 56,
                   31, 56, 146, 5,  165, 65,  5, 5};
  fmt::print("before sort: ");
  for (auto& i : l) {
    fmt::print("{} ", i);
  }
  fmt::print("\n");
  parallel_quick_sort(l);
  //  std::shared_ptr<std::vector<int>> ptr =
  //      std::make_shared<std::vector<int>>(std::vector<int>{
  //          1,
  //          5,
  //          6,
  //          1,
  //          651,
  //          6,
  //          1,
  //          63,
  //      });
  //  auto itVec = std::move(*ptr);
  //  ptr = nullptr;
  //  for (auto it : itVec) {
  //    fmt::print("{},", it);
  //  }

  slib::lockfree::stack<int> s;
  s.push(2);
  auto top = s.top();
  fmt::print("{}", test(1, 2));
  for (auto& i : l) {
    fmt::print("{} ", i);
  }
  return 0;
}