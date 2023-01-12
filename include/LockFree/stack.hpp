/*
 * @Author: s
 * @Date: 2023-01-11 18:39:59
 * @LastEditTime: 2023-01-12 02:11:03
 * @LastEditors: s
 * @Description:
 * @FilePath: /slib/include/LockFree/stack.hpp
 */

#ifndef STACK_HPP
#define STACK_HPP

#include <atomic>
#include <memory>

namespace slib {
namespace lockfree {

template <typename T>
class stack {
 private:
  using value_type = T;
  using reference = T &;

  struct node {
    std::shared_ptr<T> data;
    node *next;
    explicit node(T const &_data)
        : data(std::make_shared<T>(_data)), next(nullptr) {}
    constexpr node() : data(nullptr), next(nullptr) {}
  };

  std::atomic_uint32_t threads_in_pop;
  static void delete_nodes(node *nodes) {
    while (nodes) {
      node *next = nodes->next;
      delete nodes;
      nodes = nodes->next;
    }
  }

  void chain_pending_nodes(node *nodes) {
    node *last = nodes;
    while (node *const next = last->next) {
      last = next;
    }
    chain_pending_nodes(nodes, last);
  }
  void chain_pending_nodes(node *first, node *last) {
    // last->next = to_be_deleted;
    while (!to_be_deleted.compare_exchange_weak(last->next, first))
      ;
  }

  void chain_pending_node(node *_node) { chain_pending_nodes(_node, _node); }

  void try_reclaim(node *old_head) {
    if (threads_in_pop == 1) {
      node *nodes_to_delete = to_be_deleted.exchange(nullptr);
      if (!--threads_in_pop) {
        delete_nodes(nodes_to_delete);
      }  // 再次确认无pop线程即可删除旧节点
      else if (nodes_to_delete) {
        chain_pending_nodes(nodes_to_delete);
      }
      delete old_head;  // 无论如何都可以删除自己的old_head节点
    }                   // 若只有一个进程在pop
    else {
      chain_pending_node(old_head);  // 将old_head加入待删链
      --threads_in_pop;
    }
  }

  std::atomic<node *> head;
  std::atomic<node *> to_be_deleted;

 public:
  constexpr stack()
      : head(nullptr), to_be_deleted(nullptr), threads_in_pop(0) {}

  // todo top()
  reference top() {
    ++threads_in_pop;
    auto old_head = head.load();
    T *ret = new T{*old_head->data};
    --threads_in_pop;
    return *ret;
  }
  void push(T const &_data) {
    node *const new_node = new node(_data);
    // new_node->next = head.load();
    while (!head.compare_exchange_weak(new_node->next, new_node))
      ;
  }
  void push(T &&_data) {
    node *const new_node = new node();
    new_node->data = std::make_shared<T>(std::move(_data));
    while (!head.compare_exchange_weak(new_node->next, new_node))
      ;
  }

  std::shared_ptr<T> pop() {
    ++threads_in_pop;
    auto old_head = head.load();
    while (old_head && !head.compare_exchange_weak(old_head, old_head->next))
      ;
    std::shared_ptr<T> res = nullptr;
    if (old_head) {
      res.swap(old_head->data);
    }
    try_reclaim(old_head);
    return res;
  }

  bool pop(T &carry_data) {
    ++threads_in_pop;
    auto old_head = head.load();
    while (old_head && !head.compare_exchange_weak(old_head, old_head->next))
      ;

    if (old_head) {
      carry_data = std::move(*old_head->data);
    }
    return old_head != nullptr;
  }
};
}  // namespace lockfree
}  // namespace slib
#endif