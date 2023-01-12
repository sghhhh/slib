/*
 * @Author: s
 * @Date: 2022-12-03 15:54:53
 * @LastEditTime: 2022-12-03 16:45:38
 * @LastEditors: s
 * @Description:
 * @FilePath: /slib/include/threadpool.hpp
 */
#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include <any>
#include <concepts>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

#include "config.hpp"

namespace slib {
template <typename T>
concept Function = std::is_function_v<T>;

class threadpool {
 public:
  explicit threadpool(size_t);

  template <typename Function, typename... Args>
  threadpool(size_t, Function&&, Args&&...);

  threadpool(threadpool const&) = delete;
  threadpool(threadpool&&);
  threadpool& operator=(threadpool const&) = delete;
  threadpool& operator=(threadpool&&);

  void stopAll();

  template <typename Function, typename... Args>
  auto enqueue(Function&&, Args&&...);

  ~threadpool();

 private:
  std::vector<std::thread> workers;
  std::deque<std::function<std::any()>> tasks;
  std::mutex queue_mutex;
  std::condition_variable condition;
  bool stop;
};

inline threadpool::threadpool(size_t t_nums) : stop(false) {
  for (size_t i = 0; i < t_nums; ++i) {
    workers.emplace_back([this] {
      for (;;) {
        std::function<std::any()> task;
        {
          std::unique_lock<std::mutex> lock(this->queue_mutex);
          this->condition.wait(
              lock, [this] { return this->stop || !this->tasks.empty(); });
          if (this->stop && this->tasks.empty()) return;
          task = std::move(this->tasks.front());
          this->tasks.pop_front();
        }
        task();
      }
    });
  }
}

template <typename Function, typename... Args>
threadpool::threadpool(size_t t_nums, Function&& fun, Args&&... args) {
  for (size_t i = 0; i < t_nums; ++i) {
    workers.emplace_back([this] {
      for (;;) {
        std::function<std::any()> task;
        {
          std::unique_lock<std::mutex> lock(this->queue_mutex);
          this->condition.wait(
              lock, [this] { return this->stop || !this->tasks.empty(); });
          if (this->stop && this->tasks.empty()) return;
          task = std::move(this->tasks.front());
          this->tasks.pop_front();
        }
        task();
      }
    });
  }
  enqueue(std::forward<Function>(fun), std::forward<Args>(args)...);
}

inline threadpool::threadpool(threadpool&& tp) {
  workers = std::move(tp.workers);
  tasks = std::move(tp.tasks);
  stop = tp.stop;
}

inline threadpool& threadpool::operator=(threadpool&& tp) {
  workers = std::move(tp.workers);
  tasks = std::move(tp.tasks);
  stop = tp.stop;
  return *this;
}

inline void threadpool::stopAll() {
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    stop = true;
  }
  condition.notify_all();
  for (auto& worker : workers) worker.join();
}

template <typename Function, typename... Args>
auto threadpool::enqueue(Function&& fun, Args&&... args) {
  using return_type = std::invoke_result_t<Function, Args...>;
  auto task = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<Function>(fun), std::forward<Args>(args)...));
  std::future<return_type> res = task->get_future();
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    if (stop) throw std::runtime_error("enqueue on stopped threadpool");
    tasks.emplace_back([task] { return (*task)(); });
  }
  condition.notify_one();
  return res;
}
};  // namespace slib

#endif