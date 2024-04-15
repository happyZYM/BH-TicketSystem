#ifndef SJTU_PRIORITY_QUEUE_HPP
#define SJTU_PRIORITY_QUEUE_HPP

#include <cstddef>
#include <functional>
#include "exceptions.hpp"

namespace sjtu {

/**
 * a container like std::priority_queue which is a heap internal.
 * Use Skew heap to implement this priority_queue.
 */
template <typename T, class Compare = std::less<T>>
class priority_queue {
  Compare comp;
  struct Node {
    T value;
    Node *left, *right;
    Node(const T &v) : value(v), left(nullptr), right(nullptr) {}
  } * root;
  size_t node_count;
  void FreeAll(Node *node) {
    if (node == nullptr) return;
    FreeAll(node->left);
    FreeAll(node->right);
    delete node;
  }
  void CopyAll(Node *&dest, Node *src) {
    if (src == nullptr) return;
    dest = new Node(src->value);
    CopyAll(dest->left, src->left);
    CopyAll(dest->right, src->right);
  }

  Node *SkewMerge(Node *a, Node *b) {
    if (a == nullptr) return b;
    if (b == nullptr) return a;
    if (comp(a->value, b->value)) std::swap(a, b);
    a->right = SkewMerge(a->right, b);
    std::swap(a->left, a->right);
    return a;
  }

 public:
  priority_queue() : root(nullptr), node_count(0) {}
  priority_queue(const priority_queue &other) {
    root = nullptr;
    CopyAll(root, other.root);
    node_count = other.node_count;
  }
  /**
   * TODO deconstructor
   */
  ~priority_queue() {
    FreeAll(root);
    root = nullptr;
  }
  /**
   * TODO Assignment operator
   */
  priority_queue &operator=(const priority_queue &other) {
    if (this == &other) return *this;
    FreeAll(root);
    root = nullptr;
    CopyAll(root, other.root);
    node_count = other.node_count;
    return *this;
  }
  /**
   * get the top of the queue.
   * @return a reference of the top element.
   * throw container_is_empty if empty() returns true;
   */
  const T &top() const {
    if (empty()) throw container_is_empty();
    return root->value;
  }
  /**
   * TODO
   * push new element to the priority queue.
   */
  void push(const T &e) {
    Node *new_node = new Node(e);
    try {
      root = SkewMerge(root, new_node);
    } catch (...) {
      delete new_node;
      throw;
    }
    ++node_count;
  }
  /**
   * TODO
   * delete the top element.
   * throw container_is_empty if empty() returns true;
   */
  void pop() {
    if (empty()) throw container_is_empty();
    Node *old_root = root;
    root = SkewMerge(root->left, root->right);
    delete old_root;
    --node_count;
  }
  /**
   * return the number of the elements.
   */
  size_t size() const { return node_count; }
  /**
   * check if the container has at least an element.
   * @return true if it is empty, false if it has at least an element.
   */
  bool empty() const { return node_count == 0; }
  /**
   * merge two priority_queues with at most O(logn) complexity.
   * clear the other priority_queue.
   */
  void merge(priority_queue &other) {
    if (other.root == root) return;
    root = SkewMerge(root, other.root);
    node_count += other.node_count;
    other.root = nullptr;
    other.node_count = 0;
  }
};

}  // namespace sjtu

#endif
