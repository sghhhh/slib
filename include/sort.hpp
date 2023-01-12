/*
 * @Author: s
 * @Date: 2023-01-08 01:08:02
 * @LastEditTime: 2023-01-12 02:23:34
 * @LastEditors: s
 * @Description:
 * @FilePath: /slib/include/sort.hpp
 */
#include <atomic>
#include <functional>
#include <future>
#include <list>
#include <memory>
#include <vector>

#include "LockFree/stack.hpp"
namespace slib {
template <typename T, typename cmp>
struct sorter {
  struct chunk_to_sort {
    std::list<T> data;
    std::promise<std::list<T>> _promise;
  };
  slib::lockfree::stack<chunk_to_sort> chunks;
  std::vector<std::thread> threads;
  size_t const max_thread_count;
  std::atomic<bool> end_of_data;

  sorter()
      : max_thread_count(/*std::thread::hardware_concurrency() -*/ 1),
        end_of_data(false) {}

  ~sorter() {
    end_of_data = true;
    for (auto &_thread : threads) {
      _thread.join();
    }
  }

  void try_sort_chunk() {
    // chunk_to_sort chunk;
    auto chunk = chunks.pop();
    if (chunk) {
      sort_chunk(chunk);
    }
  }

  std::list<T> do_sort(std::list<T> &chunk_data) {
    if (chunk_data.empty()) {
      return chunk_data;
    }  // empty list just return it
    std::list<T> result;
    result.splice(result.begin(), chunk_data, chunk_data.begin());
    T const &partition_val = *result.begin();
    typename std::list<T>::iterator divide_point =
        std::partition(chunk_data.begin(), chunk_data.end(),
                       [&](T const &val) { return cmp()(val, partition_val); });
    chunk_to_sort new_lower_chunk;
    new_lower_chunk.data.splice(new_lower_chunk.data.end(), chunk_data,
                                chunk_data.begin(), divide_point);
    std::future<std::list<T>> new_lower = new_lower_chunk._promise.get_future();
    chunks.push(std::move(new_lower_chunk));
    if (threads.size() < max_thread_count) {
      threads.push_back(std::thread(&sorter<T, cmp>::sort_thread, this));
    }
    std::list<T> new_higher(do_sort(chunk_data));
    result.splice(result.end(), new_higher);
    while (new_lower.wait_for(std::chrono::seconds(0)) !=
           std::future_status::ready) {
      try_sort_chunk();
    }
    result.splice(result.begin(), new_lower.get());
    return result;
  }
  void sort_chunk(std::shared_ptr<chunk_to_sort> chunk) {
    chunk->_promise.set_value(do_sort(chunk->data));
  }
  void sort_thread() {
    while (!end_of_data) {
      try_sort_chunk();
      std::this_thread::yield();
    }
  }
};

template <typename T, typename cmp = std::less<T>>
std::list<T> parallel_quick_sort(std::list<T> &input) {
  if (input.empty()) {
    return input;
  }
  sorter<T, cmp> s;
  return s.do_sort(input);
}
}  // namespace slib