/**
 * implement a container like std::map
 */
#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

// only for std::less<T>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iterator>
#ifndef NDEBUG
#include <queue>   // only for debug use
#include <vector>  // only for debug use
#endif
#include "exceptions.hpp"
#include "utility.hpp"

namespace sjtu {
struct map_iterator_tag : std::bidirectional_iterator_tag {};
template <class Key, class T, class Compare = std::less<Key> >
class map {
 public:
  /**
   * the internal type of data.
   * it should have a default constructor, a copy constructor.
   * You can use sjtu::map as value_type by typedef.
   */
  typedef pair<const Key, T> value_type;

 private:
  static Compare comparer;
  /**
   * The NIL Node is recorded as a nullptr pointer.
   */
  struct RedBlackTreeNodeType {
    value_type val;
    RedBlackTreeNodeType *left, *right, *parent;
    enum RedBlackTreeColorType { RED, BLACK } color;
    RedBlackTreeNodeType() : left(nullptr), right(nullptr), parent(nullptr), color(RED) {}
    RedBlackTreeNodeType(const value_type &val, RedBlackTreeNodeType *left, RedBlackTreeNodeType *right,
                         RedBlackTreeNodeType *parent, RedBlackTreeColorType color)
        : val(val), left(left), right(right), parent(parent), color(color) {}
    inline RedBlackTreeNodeType *GetGrandParent() const noexcept {
      if (parent == nullptr)
#if __cplusplus >= 202002L
          [[unlikely]]
#endif
        return nullptr;
      return parent->parent;
    }
    inline RedBlackTreeNodeType *GetUncle() const noexcept {
      RedBlackTreeNodeType *grand_parent = GetGrandParent();
      if (grand_parent == nullptr)
#if __cplusplus >= 202002L
          [[unlikely]]
#endif
        return nullptr;
      if (parent == grand_parent->left)
        return grand_parent->right;
      else
        return grand_parent->left;
    }
    inline RedBlackTreeNodeType *GetSibling() const noexcept {
      if (parent == nullptr)
#if __cplusplus >= 202002L
          [[unlikely]]
#endif
        return nullptr;
      if (this == parent->left)
        return parent->right;
      else
        return parent->left;
    }
    inline RedBlackTreeNodeType *&GetSelfPath(RedBlackTreeNodeType *&tree_root) noexcept {
      if (parent == nullptr) return tree_root;
      if (this == parent->left)
        return parent->left;
      else
        return parent->right;
    }
    inline void SetChildrensParent() noexcept {
      if (left != nullptr) left->parent = this;
      if (right != nullptr) right->parent = this;
    }
    inline void RotateLeft(RedBlackTreeNodeType *&tree_root) noexcept {
      assert(this->right != nullptr);
      RedBlackTreeNodeType *parent_backup = parent;
      RedBlackTreeNodeType *&path = this->GetSelfPath(tree_root);
      RedBlackTreeNodeType *replacement = this->right;
      this->right = replacement->left;
      replacement->left = this;
      this->SetChildrensParent();
      replacement->SetChildrensParent();
      path = replacement;
      replacement->parent = parent_backup;
    }
    inline void RotateRight(RedBlackTreeNodeType *&tree_root) noexcept {
      assert(this->left != nullptr);
      RedBlackTreeNodeType *parent_backup = parent;
      RedBlackTreeNodeType *&path = this->GetSelfPath(tree_root);
      RedBlackTreeNodeType *replacement = this->left;
      this->left = replacement->right;
      replacement->right = this;
      this->SetChildrensParent();
      replacement->SetChildrensParent();
      path = replacement;
      replacement->parent = parent_backup;
    }
    void InsertFixUp(RedBlackTreeNodeType *&tree_root) {
      if (parent == nullptr) {
        // Case 1
        color = RedBlackTreeColorType::BLACK;
        return;
      }
      if (parent->color == RedBlackTreeColorType::BLACK) return;
      if (parent->parent == nullptr) {
        // Case 2 & 3
        parent->color = RedBlackTreeColorType::BLACK;
        return;
      }
      RedBlackTreeNodeType *uncle = GetUncle();
      RedBlackTreeNodeType *grand_parent = GetGrandParent();
      if (uncle != nullptr && uncle->color == RedBlackTreeColorType::RED) {
        // Case 4
        parent->color = RedBlackTreeColorType::BLACK;
        uncle->color = RedBlackTreeColorType::BLACK;
        grand_parent->color = RedBlackTreeColorType::RED;
        grand_parent->InsertFixUp(tree_root);
        return;
      }
      if (grand_parent->left == parent) {
        if (parent->right == this) {
          RedBlackTreeNodeType *old_parent = parent;
          parent->RotateLeft(tree_root);
          assert(old_parent->parent == this);
          old_parent->InsertFixUp(tree_root);
          return;
        }
        grand_parent->RotateRight(tree_root);
        assert(grand_parent->parent == parent);
        parent->color = RedBlackTreeColorType::BLACK;
        grand_parent->color = RedBlackTreeColorType::RED;
      } else {
        if (parent->left == this) {
          RedBlackTreeNodeType *old_parent = parent;
          parent->RotateRight(tree_root);
          assert(old_parent->parent == this);
          old_parent->InsertFixUp(tree_root);
          return;
        }
        grand_parent->RotateLeft(tree_root);
        assert(grand_parent->parent == parent);
        parent->color = RedBlackTreeColorType::BLACK;
        grand_parent->color = RedBlackTreeColorType::RED;
      }
    }
    /**
     * @brief Insert a new node into the tree.
     *
     * @details This function will insert a new node into the tree. If insert successfully, it will return true.
     *
     * @param tree_root The root of the tree.
     * @param val The value to be inserted.
     * @param allow_replacement Whether to allow replacement if the key already exists.
     *
     * @return Whether the insertion is successful and where the insertion is.
     *
     * @note Note that tree_root is a reference to the root of the tree. This function will modify the tree_root if
     * necessary.
     */
    std::pair<RedBlackTreeNodeType *, bool> Insert(RedBlackTreeNodeType *&tree_root, const value_type &val,
                                                   bool allow_replacement) {
      if (comparer(val.first, this->val.first)) {
        if (left == nullptr) {
          left = new RedBlackTreeNodeType(val, nullptr, nullptr, this, RedBlackTreeColorType::RED);
          left->parent = this;
          RedBlackTreeNodeType *addr = left;
          left->InsertFixUp(tree_root);
          return std::pair<RedBlackTreeNodeType *, bool>(addr, true);
        } else {
          return left->Insert(tree_root, val, allow_replacement);
        }
      } else if (comparer(this->val.first, val.first)) {
        if (right == nullptr) {
          right = new RedBlackTreeNodeType(val, nullptr, nullptr, this, RedBlackTreeColorType::RED);
          right->parent = this;
          RedBlackTreeNodeType *addr = right;
          right->InsertFixUp(tree_root);
          return std::pair<RedBlackTreeNodeType *, bool>(addr, true);
        } else {
          return right->Insert(tree_root, val, allow_replacement);
        }
      } else {
        if (allow_replacement) {
          this->val.second = val.second;
          return std::pair<RedBlackTreeNodeType *, bool>(this, false);
        }
      }
      return std::pair<RedBlackTreeNodeType *, bool>(this, false);
    }
    /**
     * @brief The definition of ReleaseAll.
     *
     * @details This fuction will be called when the whole map is destructed. It will release all the memory allocated.
     *
     * @note Note that the node itself must be released outside this function.
     */
    void ReleaseAll() {
      if (left) left->ReleaseAll();
      if (right) right->ReleaseAll();
      delete left;
      delete right;
    }
    RedBlackTreeNodeType *Find(const decltype(val.first) &key) {
      if (comparer(key, val.first)) {
        if (left == nullptr) return nullptr;
        return left->Find(key);
      } else if (comparer(val.first, key)) {
        if (right == nullptr) return nullptr;
        return right->Find(key);
      } else {
        return this;
      }
    }
    /**
     * @brief Swap the node with its successor.
     *
     * @details This function will swap the node with its successor.
     *
     * @note The color is not swapped.
     */
    inline static void SwapNodeWithItsSuccessor(RedBlackTreeNodeType *node, RedBlackTreeNodeType *successor,
                                                RedBlackTreeNodeType *&tree_root) {
      RedBlackTreeNodeType *left_of_node = node->left;
      RedBlackTreeNodeType *right_of_node = node->right;
      RedBlackTreeNodeType *parent_of_node = node->parent;
      RedBlackTreeColorType color_of_node = node->color;
      RedBlackTreeNodeType *&path_of_node = node->GetSelfPath(tree_root);
      RedBlackTreeNodeType *left_of_successor = successor->left;
      RedBlackTreeNodeType *right_of_successor = successor->right;
      RedBlackTreeNodeType *parent_of_successor = successor->parent;
      RedBlackTreeColorType color_of_successor = successor->color;
      RedBlackTreeNodeType *&path_of_successor = successor->GetSelfPath(tree_root);
      node->color = color_of_successor;
      successor->color = color_of_node;
      if (parent_of_successor == node) {
        successor->left = left_of_node;
        successor->right = node;
        successor->SetChildrensParent();
        successor->parent = parent_of_node;
        path_of_node = successor;
        node->left = left_of_successor;
        node->right = right_of_successor;
        node->SetChildrensParent();
      } else {
        successor->left = left_of_node;
        successor->right = right_of_node;
        successor->SetChildrensParent();
        successor->parent = parent_of_node;
        path_of_node = successor;
        node->left = left_of_successor;
        node->right = right_of_successor;
        node->SetChildrensParent();
        node->parent = parent_of_successor;
        path_of_successor = node;
      }
    }
    void DeleteFixUp(RedBlackTreeNodeType *&tree_root) {
      assert(this->color == RedBlackTreeColorType::BLACK);
      if (this == tree_root) return;
      RedBlackTreeNodeType *sibling = GetSibling();
      assert(sibling != nullptr);
      if (sibling->color == RedBlackTreeColorType::RED) {
        // Case 1
        parent->color = RedBlackTreeColorType::RED;
        sibling->color = RedBlackTreeColorType::BLACK;
        if (this == parent->left) {
          parent->RotateLeft(tree_root);
        } else {
          parent->RotateRight(tree_root);
        }
        this->DeleteFixUp(tree_root);
        return;
      }
      RedBlackTreeNodeType *close_nephew = nullptr;
      RedBlackTreeNodeType *distant_nephew = nullptr;
      if (this == parent->left) {
        close_nephew = sibling->left;
        distant_nephew = sibling->right;
      } else {
        close_nephew = sibling->right;
        distant_nephew = sibling->left;
      }
      if (sibling->color == RedBlackTreeColorType::BLACK && this->parent->color == RedBlackTreeColorType::RED &&
          (close_nephew == nullptr || close_nephew != nullptr && close_nephew->color == RedBlackTreeColorType::BLACK) &&
          (distant_nephew == nullptr ||
           distant_nephew != nullptr && distant_nephew->color == RedBlackTreeColorType::BLACK)) {
        // Case 2
        sibling->color = RedBlackTreeColorType::RED;
        this->parent->color = RedBlackTreeColorType::BLACK;
        return;
      }
      if (sibling->color == RedBlackTreeColorType::BLACK && this->parent->color == RedBlackTreeColorType::BLACK &&
          (close_nephew == nullptr || close_nephew != nullptr && close_nephew->color == RedBlackTreeColorType::BLACK) &&
          (distant_nephew == nullptr ||
           distant_nephew != nullptr && distant_nephew->color == RedBlackTreeColorType::BLACK)) {
        // Case 3
        sibling->color = RedBlackTreeColorType::RED;
        this->parent->DeleteFixUp(tree_root);
        return;
      }
      if (close_nephew != nullptr && close_nephew->color == RedBlackTreeColorType::RED) {
        // Case 4
        if (this == parent->left) {
          sibling->color = RedBlackTreeColorType::RED;
          close_nephew->color = RedBlackTreeColorType::BLACK;
          sibling->RotateRight(tree_root);
        } else {
          sibling->color = RedBlackTreeColorType::RED;
          close_nephew->color = RedBlackTreeColorType::BLACK;
          sibling->RotateLeft(tree_root);
        }
        this->DeleteFixUp(tree_root);
        return;
      }
      assert(distant_nephew != nullptr && distant_nephew->color == RedBlackTreeColorType::RED);
      // Then it must be Case 5
      if (this == parent->left) {
        std::swap(sibling->color, parent->color);
        distant_nephew->color = RedBlackTreeColorType::BLACK;
        parent->RotateLeft(tree_root);
      } else {
        std::swap(sibling->color, parent->color);
        distant_nephew->color = RedBlackTreeColorType::BLACK;
        parent->RotateRight(tree_root);
      }
    }
    static void DeleteNode(RedBlackTreeNodeType *pos, RedBlackTreeNodeType *&tree_root) {
      if (pos->parent == nullptr && pos->left == nullptr && pos->right == nullptr) {
        // Case 0: The only node in the tree.
        delete pos;
        tree_root = nullptr;
        return;
      }
      if (pos->left != nullptr && pos->right != nullptr) {
        // Case 1: The node has two children. Then we swap the node with its successor and just delete the successor.
        RedBlackTreeNodeType *successor = pos->right;
        while (successor->left != nullptr) successor = successor->left;
        SwapNodeWithItsSuccessor(pos, successor, tree_root);
        DeleteNode(pos, tree_root);
        return;
      }
      if (pos->left == nullptr && pos->right == nullptr) {
        // Case 2
        if (pos->color == RedBlackTreeColorType::BLACK) {
          pos->DeleteFixUp(tree_root);
        }
        pos->GetSelfPath(tree_root) = nullptr;
        delete pos;
        return;
      }
      // Case 3
      RedBlackTreeNodeType *replacement = (pos->left != nullptr ? pos->left : pos->right);
      assert(replacement != nullptr);
      assert(replacement->color == RedBlackTreeColorType::RED);
      pos->GetSelfPath(tree_root) = replacement;
      replacement->parent = pos->parent;
      replacement->color = RedBlackTreeColorType::BLACK;
      delete pos;
    }
    static void CopyFrom(RedBlackTreeNodeType *&target, const RedBlackTreeNodeType *source) {
      if (source == nullptr) return;
      target = new RedBlackTreeNodeType(source->val, nullptr, nullptr, nullptr, source->color);
      CopyFrom(target->left, source->left);
      CopyFrom(target->right, source->right);
      target->SetChildrensParent();
    }
  };
  size_t node_count;
  RedBlackTreeNodeType *tree_root;

 public:
  /**
   * see BidirectionalIterator at CppReference for help.
   *
   * if there is anything wrong throw invalid_iterator.
   *     like it = map.begin(); --it;
   *       or it = map.end(); ++end();
   */
  class const_iterator;
  class iterator;
  friend iterator;
  friend const_iterator;
  class iterator {
   private:
    RedBlackTreeNodeType *raw_pointer;  // when iterator points to end(), raw_pointer=nullptr
    map *domain;

   public:
    // Add some type traits
    typedef sjtu::map_iterator_tag iterator_category;
    typedef pair<const Key, T> value_type;
    typedef std::ptrdiff_t difference_type;
    typedef value_type *pointer;
    typedef value_type &reference;

    friend const_iterator;
    friend map;
    iterator() : raw_pointer(nullptr), domain(nullptr) {}
    iterator(const iterator &other) : raw_pointer(other.raw_pointer), domain(other.domain) {}
    iterator(RedBlackTreeNodeType *raw_pointer, map *domain) : raw_pointer(raw_pointer), domain(domain) {}
    iterator &operator++() {
      if (raw_pointer == nullptr) throw invalid_iterator();
      if (raw_pointer->right != nullptr) {
        raw_pointer = raw_pointer->right;
        while (raw_pointer->left != nullptr) raw_pointer = raw_pointer->left;
      } else {
        RedBlackTreeNodeType *backup = raw_pointer;
        while (raw_pointer->parent != nullptr && raw_pointer->parent->right == raw_pointer)
          raw_pointer = raw_pointer->parent;
        if (raw_pointer->parent == nullptr) {
          raw_pointer = nullptr;
          return *this;
        }
        raw_pointer = raw_pointer->parent;
      }
      return *this;
    }
    iterator operator++(int) {
      iterator tmp = *this;
      ++*this;
      return tmp;
    }
    iterator &operator--() {
      if (raw_pointer == nullptr) {
        if (domain == nullptr) throw invalid_iterator();
        if (domain->tree_root == nullptr) throw invalid_iterator();
        raw_pointer = domain->tree_root;
        while (raw_pointer->right != nullptr) raw_pointer = raw_pointer->right;
        return *this;
      }
      if (raw_pointer->left != nullptr) {
        raw_pointer = raw_pointer->left;
        while (raw_pointer->right != nullptr) raw_pointer = raw_pointer->right;
      } else {
        RedBlackTreeNodeType *backup = raw_pointer;
        while (raw_pointer->parent != nullptr && raw_pointer->parent->left == raw_pointer)
          raw_pointer = raw_pointer->parent;
        if (raw_pointer->parent == nullptr) {
          throw invalid_iterator();
          raw_pointer = nullptr;
          return *this;
        }
        raw_pointer = raw_pointer->parent;
      }
      return *this;
    }
    iterator operator--(int) {
      iterator tmp = *this;
      --*this;
      return tmp;
    }
    /**
     * a operator to check whether two iterators are same (pointing to the same memory).
     */
    value_type &operator*() const {
      if (raw_pointer == nullptr) throw invalid_iterator();
      return raw_pointer->val;
    }
    bool operator==(const iterator &rhs) const { return domain == rhs.domain && raw_pointer == rhs.raw_pointer; }
    bool operator==(const const_iterator &rhs) const { return domain == rhs.domain && raw_pointer == rhs.raw_pointer; }
    /**
     * some other operator for iterator.
     */
    bool operator!=(const iterator &rhs) const { return domain != rhs.domain || raw_pointer != rhs.raw_pointer; }
    bool operator!=(const const_iterator &rhs) const { return domain != rhs.domain || raw_pointer != rhs.raw_pointer; }

    /**
     * for the support of it->first.
     * See
     * <http://kelvinh.github.io/blog/2013/11/20/overloading-of-member-access-operator-dash-greater-than-symbol-in-cpp/>
     * for help.
     */
    value_type *operator->() const noexcept { return &raw_pointer->val; }
  };
  class const_iterator {
   private:
    RedBlackTreeNodeType *raw_pointer;  // when iterator points to end(), raw_pointer=nullptr
    const map *domain;

   public:
    // Add some type traits
    typedef sjtu::map_iterator_tag iterator_category;
    typedef pair<const Key, T> value_type;
    typedef std::ptrdiff_t difference_type;
    typedef value_type *pointer;
    typedef value_type &reference;

    friend iterator;
    friend map;
    const_iterator() : raw_pointer(nullptr), domain(nullptr) {}
    const_iterator(const const_iterator &other) : raw_pointer(other.raw_pointer), domain(other.domain) {}
    const_iterator(const iterator &other) : raw_pointer(other.raw_pointer), domain(other.domain) {}
    const_iterator(RedBlackTreeNodeType *raw_pointer, const map *domain) : raw_pointer(raw_pointer), domain(domain) {}
    const_iterator &operator++() {
      if (raw_pointer == nullptr) throw invalid_iterator();
      if (raw_pointer->right != nullptr) {
        raw_pointer = raw_pointer->right;
        while (raw_pointer->left != nullptr) raw_pointer = raw_pointer->left;
      } else {
        RedBlackTreeNodeType *backup = raw_pointer;
        while (raw_pointer->parent != nullptr && raw_pointer->parent->right == raw_pointer)
          raw_pointer = raw_pointer->parent;
        if (raw_pointer->parent == nullptr) {
          raw_pointer = nullptr;
          return *this;
        }
        raw_pointer = raw_pointer->parent;
      }
      return *this;
    }
    const_iterator operator++(int) {
      const_iterator tmp = *this;
      ++*this;
      return tmp;
    }
    const_iterator &operator--() {
      if (raw_pointer == nullptr) {
        if (domain == nullptr) throw invalid_iterator();
        if (domain->tree_root == nullptr) throw invalid_iterator();
        raw_pointer = domain->tree_root;
        while (raw_pointer->right != nullptr) raw_pointer = raw_pointer->right;
        return *this;
      }
      if (raw_pointer->left != nullptr) {
        raw_pointer = raw_pointer->left;
        while (raw_pointer->right != nullptr) raw_pointer = raw_pointer->right;
      } else {
        RedBlackTreeNodeType *backup = raw_pointer;
        while (raw_pointer->parent != nullptr && raw_pointer->parent->left == raw_pointer)
          raw_pointer = raw_pointer->parent;
        if (raw_pointer->parent == nullptr) {
          throw invalid_iterator();
          raw_pointer = nullptr;
          return *this;
        }
        raw_pointer = raw_pointer->parent;
      }
      return *this;
    }
    const_iterator operator--(int) {
      const_iterator tmp = *this;
      --*this;
      return tmp;
    }
    /**
     * a operator to check whether two iterators are same (pointing to the same memory).
     */
    const value_type &operator*() const {
      if (raw_pointer == nullptr) throw invalid_iterator();
      return raw_pointer->val;
    }
    bool operator==(const iterator &rhs) const { return domain == rhs.domain && raw_pointer == rhs.raw_pointer; }
    bool operator==(const const_iterator &rhs) const { return domain == rhs.domain && raw_pointer == rhs.raw_pointer; }
    /**
     * some other operator for iterator.
     */
    bool operator!=(const iterator &rhs) const { return domain != rhs.domain || raw_pointer != rhs.raw_pointer; }
    bool operator!=(const const_iterator &rhs) const { return domain != rhs.domain || raw_pointer != rhs.raw_pointer; }
    value_type *operator->() const noexcept { return &raw_pointer->val; }
  };
  map() : node_count(0), tree_root(nullptr) {}
  map(const map &other) {
    node_count = other.node_count;
    tree_root = nullptr;
    RedBlackTreeNodeType::CopyFrom(tree_root, other.tree_root);
  }
  map(map &&other) {
    node_count = other.node_count;
    tree_root = other.tree_root;
    other.node_count = 0;
    other.tree_root = nullptr;
  }
  /**
   * assignment operator
   */
  map &operator=(const map &other) {
    if (this == &other) return *this;
    if (tree_root) tree_root->ReleaseAll();
    delete tree_root;
    node_count = other.node_count;
    RedBlackTreeNodeType::CopyFrom(tree_root, other.tree_root);
    return *this;
  }
  map &operator=(map &&other) {
    if (this == &other) return *this;
    if (tree_root) tree_root->ReleaseAll();
    delete tree_root;
    node_count = other.node_count;
    tree_root = other.tree_root;
    other.node_count = 0;
    other.tree_root = nullptr;
    return *this;
  }
  ~map() {
    if (tree_root) tree_root->ReleaseAll();
    delete tree_root;
  }
  /**
   * access specified element with bounds checking
   * Returns a reference to the mapped value of the element with key equivalent to key.
   * If no such element exists, an exception of type `index_out_of_bound'
   */
  T &at(const Key &key) {
    if (tree_root == nullptr) throw index_out_of_bound();
    RedBlackTreeNodeType *result = tree_root->Find(key);
    if (result == nullptr) throw index_out_of_bound();
    return result->val.second;
  }
  const T &at(const Key &key) const {
    if (tree_root == nullptr) throw index_out_of_bound();
    RedBlackTreeNodeType *result = tree_root->Find(key);
    if (result == nullptr) throw index_out_of_bound();
    return result->val.second;
  }
  /**
   * access specified element
   * Returns a reference to the value that is mapped to a key equivalent to key,
   *   performing an insertion if such key does not already exist.
   */
  T &operator[](const Key &key) {
    if (node_count == 0) {
      tree_root =
          new RedBlackTreeNodeType(value_type(key, T()), nullptr, nullptr, nullptr, RedBlackTreeNodeType::BLACK);
      ++node_count;
      return tree_root->val.second;
    }
    auto result = tree_root->Insert(tree_root, value_type(key, T()), false);
    if (result.second) ++node_count;
    return result.first->val.second;
  }
  /**
   * behave like at() throw index_out_of_bound if such key does not exist.
   */
  const T &operator[](const Key &key) const { return at(key); }
  /**
   * return a iterator to the beginning
   */
  iterator begin() {
    if (tree_root == nullptr) return iterator(nullptr, this);
    RedBlackTreeNodeType *tmp = tree_root;
    while (tmp->left != nullptr) tmp = tmp->left;
    return iterator(tmp, this);
  }
  const_iterator begin() const { return cbegin(); }
  const_iterator cbegin() const {
    if (tree_root == nullptr) return const_iterator(nullptr, this);
    RedBlackTreeNodeType *tmp = tree_root;
    while (tmp->left != nullptr) tmp = tmp->left;
    return const_iterator(tmp, this);
  }
  /**
   * return a iterator to the end
   * in fact, it returns past-the-end.
   */
  iterator end() { return iterator(nullptr, this); }
  const_iterator end() const { return cend(); }
  const_iterator cend() const { return const_iterator(nullptr, this); }
  /**
   * checks whether the container is empty
   * return true if empty, otherwise false.
   */
  bool empty() const { return node_count == 0; }
  /**
   * returns the number of elements.
   */
  size_t size() const { return node_count; }
  /**
   * clears the contents
   */
  void clear() {
    if (tree_root) tree_root->ReleaseAll();
    delete tree_root;
    tree_root = nullptr;
    node_count = 0;
  }
  /**
   * insert an element.
   * return a pair, the first of the pair is
   *   the iterator to the new element (or the element that prevented the insertion),
   *   the second one is true if insert successfully, or false.
   */
  pair<iterator, bool> insert(const value_type &value) {
    if (tree_root == nullptr) {
      tree_root = new RedBlackTreeNodeType(value, nullptr, nullptr, nullptr, RedBlackTreeNodeType::BLACK);
      node_count = 1;
      return pair<iterator, bool>(iterator(tree_root, this), true);
    }
    auto result = tree_root->Insert(tree_root, value, false);
    if (result.second) ++node_count;
    return pair<iterator, bool>(iterator(result.first, this), result.second);
  }
  /**
   * erase the element at pos.
   *
   * throw if pos pointed to a bad element (pos == this->end() || pos points an element out of this)
   */
  void erase(iterator pos) {
    if (pos.domain != this || pos.raw_pointer == nullptr) throw invalid_iterator();
    RedBlackTreeNodeType::DeleteNode(pos.raw_pointer, tree_root);
    --node_count;
  }
  void erase(const Key &key) {
    RedBlackTreeNodeType *result = tree_root->Find(key);
    if (result == nullptr) return;
    RedBlackTreeNodeType::DeleteNode(result, tree_root);
    --node_count;
  }
  /**
   * Returns the number of elements with key
   *   that compares equivalent to the specified argument,
   *   which is either 1 or 0
   *     since this container does not allow duplicates.
   * The default method of check the equivalence is !(a < b || b > a)
   */
  size_t count(const Key &key) const {
    if (tree_root == nullptr) return 0;
    return tree_root->Find(key) == nullptr ? 0 : 1;
  }
  /**
   * Finds an element with key equivalent to key.
   * key value of the element to search for.
   * Iterator to an element with key equivalent to key.
   *   If no such element is found, past-the-end (see end()) iterator is returned.
   */
  iterator find(const Key &key) {
    if (tree_root == nullptr) return end();
    return iterator(tree_root->Find(key), this);
  }
  const_iterator find(const Key &key) const {
    if (tree_root == nullptr) return cend();
    return const_iterator(tree_root->Find(key), this);
  }
#ifndef NDEBUG
  bool RedBlackTreeStructureCheck() {
    if (tree_root == nullptr) return node_count == 0;
    if (node_count == 0) return false;
    std::queue<RedBlackTreeNodeType *> Q;
    std::vector<RedBlackTreeNodeType *> NIL_leafs;
    size_t actual_node_count = 0;
    Q.push(tree_root);
    while (!Q.empty()) {
      RedBlackTreeNodeType *current = Q.front();
      Q.pop();
      if (current->color == RedBlackTreeNodeType::RED) {
        if (current->left != nullptr && current->left->color == RedBlackTreeNodeType::RED) return false;
        if (current->right != nullptr && current->right->color == RedBlackTreeNodeType::RED) return false;
      }
      if (current->left != nullptr)
        Q.push(current->left);
      else
        NIL_leafs.push_back(current);
      if (current->right != nullptr)
        Q.push(current->right);
      else
        NIL_leafs.push_back(current);
      ++actual_node_count;
    }
    if (actual_node_count != node_count) return false;
    if (tree_root->color != RedBlackTreeNodeType::BLACK) return false;
    if (tree_root->parent != nullptr) return false;
    assert(NIL_leafs.size() >= 2);
    size_t correct_black_nodes = 1;
    RedBlackTreeNodeType *ptr = NIL_leafs[0];
    while (ptr) {
      if (ptr->color == RedBlackTreeNodeType::BLACK) ++correct_black_nodes;
      ptr = ptr->parent;
    }
    for (auto ptr : NIL_leafs) {
      size_t black_nodes = 1;
      while (ptr) {
        if (ptr->color == RedBlackTreeNodeType::BLACK) ++black_nodes;
        ptr = ptr->parent;
      }
      if (black_nodes != correct_black_nodes) return false;
    }
    // Now check whether it is a binary search tree. Use a lambda expression to get inorder tree walk.
    std::vector<Key> key_array;
    std::function<void(RedBlackTreeNodeType *)> inorder_walk = [&](RedBlackTreeNodeType *node) {
      if (node->left != nullptr) inorder_walk(node->left);
      key_array.push_back(node->val.first);
      if (node->right != nullptr) inorder_walk(node->right);
    };
    inorder_walk(tree_root);
    for (size_t i = 1; i < key_array.size(); ++i) {
      if (!comparer(key_array[i - 1], key_array[i])) return false;
    }
    return true;
  }
#endif
};
// Define the static member comparer.
template <class Key, class T, class Compare>
Compare map<Key, T, Compare>::comparer = Compare();

}  // namespace sjtu

#endif