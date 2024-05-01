#ifndef SJTU_VECTOR_HPP
#define SJTU_VECTOR_HPP

#include "exceptions.hpp"

#include <climits>
#include <cstddef>
#include <exception>
#include <iterator>
#include <memory>

namespace sjtu {
/**
 * a data container like std::vector
 * store data in a successive memory and support random access.
 */
template <typename T>
class vector {
  static std::allocator<T> alloc;
  size_t allocated_length;
  size_t current_length;
  T *raw_beg, *raw_end;

 public:
  /**
   * you can see RandomAccessIterator at CppReference for help.
   */
  class const_iterator;
  class iterator {
    // The following code is written for the C++ type_traits library.
    // Type traits is a C++ feature for describing certain properties of a type.
    // For instance, for an iterator, iterator::value_type is the type that the
    // iterator points to.
    // STL algorithms and containers may use these type_traits (e.g. the following
    // typedef) to work properly. In particular, without the following code,
    // @code{std::sort(iter, iter1);} would not compile.
    // See these websites for more information:
    // https://en.cppreference.com/w/cpp/header/type_traits
    // About value_type: https://blog.csdn.net/u014299153/article/details/72419713
    // About iterator_category: https://en.cppreference.com/w/cpp/iterator
    friend class vector<T>;

   public:
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using iterator_category = std::random_access_iterator_tag;

   private:
    vector<T> *domain;
    T *raw_pointer;
    iterator(vector<T> *domain, T *raw_pointer) : domain(domain), raw_pointer(raw_pointer) {}

   public:
    /**
     * return a new iterator which pointer n-next elements
     * as well as operator-
     */
    iterator operator+(const int &n) const {
      iterator temp = *this;
      temp.raw_pointer += n;
      return temp;
    }
    iterator operator-(const int &n) const {
      iterator temp = *this;
      temp.raw_pointer -= n;
      return temp;
    }
    // return the distance between two iterators,
    // if these two iterators point to different vectors, throw invaild_iterator.
    int operator-(const iterator &rhs) const {
      if (domain != rhs.domain) [[unlikely]]
        throw invalid_iterator();
      return raw_pointer - rhs.raw_pointer;
    }
    iterator &operator+=(const int &n) {
      raw_pointer += n;
      return *this;
    }
    iterator &operator-=(const int &n) {
      raw_pointer -= n;
      return *this;
    }
    /**
     * TODO iter++
     */
    iterator operator++(int) {
      iterator temp = *this;
      raw_pointer++;
      return temp;
    }
    /**
     * TODO ++iter
     */
    iterator &operator++() {
      raw_pointer++;
      return *this;
    }
    /**
     * TODO iter--
     */
    iterator operator--(int) {
      iterator temp = *this;
      raw_pointer--;
      return temp;
    }
    /**
     * TODO --iter
     */
    iterator &operator--() {
      raw_pointer--;
      return *this;
    }
    /**
     * TODO *it
     */
    T &operator*() const { return *raw_pointer; }
    /**
     * a operator to check whether two iterators are same (pointing to the same memory address).
     */
    bool operator==(const iterator &rhs) const { return raw_pointer == rhs.raw_pointer; }
    bool operator==(const const_iterator &rhs) const { return raw_pointer == rhs.raw_pointer; }
    /**
     * some other operator for iterator.
     */
    bool operator!=(const iterator &rhs) const { return raw_pointer != rhs.raw_pointer; }
    bool operator!=(const const_iterator &rhs) const { return raw_pointer != rhs.raw_pointer; }
  };
  /**
   * TODO
   * has same function as iterator, just for a const object.
   */
  class const_iterator {
    friend class vector<T>;

   public:
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using iterator_category = std::random_access_iterator_tag;

   private:
    const vector<T> *domain;
    const T *raw_pointer;
    inline const_iterator(const vector<T> *domain, const T *raw_pointer) : domain(domain), raw_pointer(raw_pointer) {}

   public:
    /**
     * return a new iterator which pointer n-next elements
     * as well as operator-
     */
    const_iterator operator+(const int &n) const {
      const_iterator temp = *this;
      temp.raw_pointer += n;
      return temp;
    }
    const_iterator operator-(const int &n) const {
      const_iterator temp = *this;
      temp.raw_pointer -= n;
      return temp;
    }
    // return the distance between two iterators,
    // if these two iterators point to different vectors, throw invaild_iterator.
    int operator-(const const_iterator &rhs) const {
      if (domain != rhs.domain) [[unlikely]]
        throw invalid_iterator();
      return raw_pointer - rhs.raw_pointer;
    }
    const_iterator &operator+=(const int &n) {
      raw_pointer += n;
      return *this;
    }
    const_iterator &operator-=(const int &n) {
      raw_pointer -= n;
      return *this;
    }
    /**
     * TODO iter++
     */
    const_iterator operator++(int) {
      const_iterator temp = *this;
      raw_pointer++;
      return temp;
    }
    /**
     * TODO ++iter
     */
    const_iterator &operator++() {
      raw_pointer++;
      return *this;
    }
    /**
     * TODO iter--
     */
    const_iterator operator--(int) {
      const_iterator temp = *this;
      raw_pointer--;
      return temp;
    }
    /**
     * TODO --iter
     */
    const_iterator &operator--() {
      raw_pointer--;
      return *this;
    }
    /**
     * TODO *it
     */
    const T &operator*() const { return *raw_pointer; }
    /**
     * a operator to check whether two iterators are same (pointing to the same memory address).
     */
    bool operator==(const iterator &rhs) const { return raw_pointer == rhs.raw_pointer; }
    bool operator==(const const_iterator &rhs) const { return raw_pointer == rhs.raw_pointer; }
    /**
     * some other operator for iterator.
     */
    bool operator!=(const iterator &rhs) const { return raw_pointer != rhs.raw_pointer; }
    bool operator!=(const const_iterator &rhs) const { return raw_pointer != rhs.raw_pointer; }
  };
  /**
   * TODO Constructs
   * At least two: default constructor, copy constructor
   */
  vector() {
    raw_beg = alloc.allocate(1);
    raw_end = raw_beg;
    allocated_length = 1;
    current_length = 0;
  }
  vector(const vector &other) {
    raw_beg = alloc.allocate(other.allocated_length);
    raw_end = raw_beg + other.current_length;
    allocated_length = other.allocated_length;
    current_length = other.current_length;
    for (size_t i = 0; i < current_length; ++i) {
      std::allocator_traits<decltype(alloc)>::construct(alloc, raw_beg + i, other.raw_beg[i]);
    }
  }
  vector(vector &&other) noexcept {
    raw_beg = other.raw_beg;
    raw_end = other.raw_end;
    allocated_length = other.allocated_length;
    current_length = other.current_length;
    other.raw_beg = nullptr;
    other.raw_end = nullptr;
    other.allocated_length = 0;
    other.current_length = 0;
  }
  ~vector() {
    if (raw_beg != nullptr) {
      for (size_t i = 0; i < current_length; ++i) {
        std::allocator_traits<decltype(alloc)>::destroy(alloc, raw_beg + i);
      }
      alloc.deallocate(raw_beg, allocated_length);
    }
  }
  /**
   * TODO Assignment operator
   */
  vector &operator=(const vector &other) {
    if (this == &other) return *this;
    if (raw_beg != nullptr) {
      for (size_t i = 0; i < current_length; ++i) {
        std::allocator_traits<decltype(alloc)>::destroy(alloc, raw_beg + i);
      }
      alloc.deallocate(raw_beg, allocated_length);
    }
    raw_beg = alloc.allocate(other.allocated_length);
    raw_end = raw_beg + other.current_length;
    allocated_length = other.allocated_length;
    current_length = other.current_length;
    for (size_t i = 0; i < current_length; ++i) {
      std::allocator_traits<decltype(alloc)>::construct(alloc, raw_beg + i, other.raw_beg[i]);
    }
    return *this;
  }
  vector &operator=(vector &&other) noexcept {
    if (this == &other) return *this;
    if (raw_beg != nullptr) {
      for (size_t i = 0; i < current_length; ++i) {
        std::allocator_traits<decltype(alloc)>::destroy(alloc, raw_beg + i);
      }
      alloc.deallocate(raw_beg, allocated_length);
    }
    raw_beg = other.raw_beg;
    raw_end = other.raw_end;
    allocated_length = other.allocated_length;
    current_length = other.current_length;
    other.raw_beg = nullptr;
    other.raw_end = nullptr;
    other.allocated_length = 0;
    other.current_length = 0;
    return *this;
  }
  /**
   * assigns specified element with bounds checking
   * throw index_out_of_bound if pos is not in [0, size)
   */
  T &at(const size_t &pos) {
    if (pos < 0 || pos >= current_length) [[unlikely]]
      throw index_out_of_bound();
    return raw_beg[pos];
  }
  const T &at(const size_t &pos) const {
    if (pos < 0 || pos >= current_length) [[unlikely]]
      throw index_out_of_bound();
    return raw_beg[pos];
  }
  /**
   * assigns specified element with bounds checking
   * throw index_out_of_bound if pos is not in [0, size)
   * !!! Pay attentions
   *   In STL this operator does not check the boundary but I want you to do.
   */
  T &operator[](const size_t &pos) {
    if (pos < 0 || pos >= current_length) [[unlikely]]
      throw index_out_of_bound();
    return raw_beg[pos];
  }
  const T &operator[](const size_t &pos) const {
    if (pos < 0 || pos >= current_length) [[unlikely]]
      throw index_out_of_bound();
    return raw_beg[pos];
  }
  /**
   * access the first element.
   * throw container_is_empty if size == 0
   */
  const T &front() const {
    if (current_length == 0) [[unlikely]]
      throw container_is_empty();
    return raw_beg[0];
  }
  /**
   * access the last element.
   * throw container_is_empty if size == 0
   */
  T &back() const {
    if (current_length == 0) [[unlikely]]
      throw container_is_empty();
    return raw_end[-1];
  }
  /**
   * returns an iterator to the beginning.
   */
  iterator begin() { return iterator(this, raw_beg); }
  const_iterator begin() const { return const_iterator(this, raw_beg); }
  const_iterator cbegin() const { return const_iterator(this, raw_beg); }
  /**
   * returns an iterator to the end.
   */
  iterator end() { return iterator(this, raw_end); }
  const_iterator end() const { return const_iterator(this, raw_end); }
  const_iterator cend() const { return const_iterator(this, raw_end); }
  /**
   * checks whether the container is empty
   */
  bool empty() const { return current_length == 0; }
  /**
   * returns the number of elements
   */
  size_t size() const { return current_length; }
  /**
   * clears the contents
   */
  void clear() {
    if (raw_beg != nullptr) {
      for (size_t i = 0; i < current_length; ++i) {
        std::allocator_traits<decltype(alloc)>::destroy(alloc, raw_beg + i);
      }
      alloc.deallocate(raw_beg, allocated_length);
    }
    raw_beg = alloc.allocate(1);
    raw_end = raw_beg;
    allocated_length = 1;
    current_length = 0;
  }
  /**
   * inserts value before pos
   * returns an iterator pointing to the inserted value.
   */
  iterator insert(iterator pos, const T &value) {
    if (pos.raw_pointer < raw_beg || pos.raw_pointer > raw_end) throw invalid_iterator();
    if (current_length == allocated_length) {
      size_t new_allocated_length = allocated_length * 2;
      T *new_raw_beg = alloc.allocate(new_allocated_length);
      pos.raw_pointer = new_raw_beg + (pos.raw_pointer - raw_beg);
      for (size_t i = 0; i < current_length; ++i) {
        std::allocator_traits<decltype(alloc)>::construct(alloc, new_raw_beg + i, std::move(raw_beg[i]));
        std::allocator_traits<decltype(alloc)>::destroy(alloc, raw_beg + i);
      }
      alloc.deallocate(raw_beg, allocated_length);
      raw_beg = new_raw_beg;
      raw_end = raw_beg + current_length;
      allocated_length = new_allocated_length;
    }
    for (T *i = raw_end; i != pos.raw_pointer; --i) {
      std::allocator_traits<decltype(alloc)>::construct(alloc, i, std::move(*(i - 1)));
      std::allocator_traits<decltype(alloc)>::destroy(alloc, i - 1);
    }
    std::allocator_traits<decltype(alloc)>::construct(alloc, pos.raw_pointer, value);
    raw_end++;
    current_length++;
    return pos;
  }
  /**
   * inserts value at index ind.
   * after inserting, this->at(ind) == value
   * returns an iterator pointing to the inserted value.
   * throw index_out_of_bound if ind > size (in this situation ind can be size because after inserting the size will
   * increase 1.)
   */
  iterator insert(const size_t &ind, const T &value) {
    if (ind < 0 || ind > current_length) throw index_out_of_bound();
    if (current_length == allocated_length) {
      size_t new_allocated_length = allocated_length * 2;
      T *new_raw_beg = alloc.allocate(new_allocated_length);
      for (size_t i = 0; i < current_length; ++i) {
        alloc.construct(new_raw_beg + i, std::move(raw_beg[i]));
        alloc.destroy(raw_beg + i);
      }
      alloc.deallocate(raw_beg, allocated_length);
      raw_beg = new_raw_beg;
      raw_end = raw_beg + current_length;
      allocated_length = new_allocated_length;
    }
    for (T *i = raw_end; i != raw_beg + ind; --i) {
      alloc.construct(i, std::move(*(i - 1)));
      alloc.destroy(i - 1);
    }
    alloc.construct(raw_beg + ind, value);
    raw_end++;
    current_length++;
    return iterator(this, raw_beg + ind);
  }
  /**
   * removes the element at pos.
   * return an iterator pointing to the following element.
   * If the iterator pos refers the last element, the end() iterator is returned.
   */
  iterator erase(iterator pos) {
    if (pos.raw_pointer < raw_beg || pos.raw_pointer >= raw_end) throw invalid_iterator();
    for (T *i = pos.raw_pointer; i != raw_end - 1; ++i) {
      std::allocator_traits<decltype(alloc)>::construct(alloc, i, std::move(*(i + 1)));
      std::allocator_traits<decltype(alloc)>::destroy(alloc, i + 1);
    }
    raw_end--;
    current_length--;
    if (current_length != 0 && current_length <= allocated_length / 4) {
      size_t new_allocated_length = allocated_length / 2;
      T *new_raw_beg = alloc.allocate(new_allocated_length);
      for (size_t i = 0; i < current_length; ++i) {
        std::allocator_traits<decltype(alloc)>::construct(alloc, new_raw_beg + i, std::move(raw_beg[i]));
        std::allocator_traits<decltype(alloc)>::destroy(alloc, raw_beg + i);
      }
      alloc.deallocate(raw_beg, allocated_length);
      raw_beg = new_raw_beg;
      raw_end = raw_beg + current_length;
      allocated_length = new_allocated_length;
    }
    return pos;
  }
  /**
   * removes the element with index ind.
   * return an iterator pointing to the following element.
   * throw index_out_of_bound if ind >= size
   */
  iterator erase(const size_t &ind) {
    if (ind < 0 || ind >= current_length) throw index_out_of_bound();
    for (T *i = raw_beg + ind; i != raw_end - 1; ++i) {
      alloc.construct(i, std::move(*(i + 1)));
      alloc.destroy(i + 1);
    }
    raw_end--;
    current_length--;
    if (current_length != 0 && current_length <= allocated_length / 4) {
      size_t new_allocated_length = allocated_length / 2;
      T *new_raw_beg = alloc.allocate(new_allocated_length);
      for (size_t i = 0; i < current_length; ++i) {
        alloc.construct(new_raw_beg + i, std::move(raw_beg[i]));
        alloc.destroy(raw_beg + i);
      }
      alloc.deallocate(raw_beg, allocated_length);
      raw_beg = new_raw_beg;
      raw_end = raw_beg + current_length;
      allocated_length = new_allocated_length;
    }
    return iterator(this, raw_beg + ind);
  }
  /**
   * adds an element to the end.
   */
  void push_back(T value) {
    if (current_length == allocated_length) [[unlikely]] {
      size_t new_allocated_length = allocated_length * 2;
      T *new_raw_beg = alloc.allocate(new_allocated_length);
      for (size_t i = 0; i < current_length; ++i) {
        std::allocator_traits<decltype(alloc)>::construct(alloc, new_raw_beg + i, std::move(raw_beg[i]));
        std::allocator_traits<decltype(alloc)>::destroy(alloc, raw_beg + i);
      }
      alloc.deallocate(raw_beg, allocated_length);
      raw_beg = new_raw_beg;
      raw_end = raw_beg + current_length;
      allocated_length = new_allocated_length;
    }
    std::allocator_traits<decltype(alloc)>::construct(alloc, raw_end, std::move(value));
    raw_end++;
    current_length++;
  }
  /**
   * remove the last element from the end.
   * throw container_is_empty if size() == 0
   */
  void pop_back() {
    if (current_length == 0) [[unlikely]]
      throw container_is_empty();
    std::allocator_traits<decltype(alloc)>::destroy(alloc, raw_end - 1);
    raw_end--;
    current_length--;
    if (current_length != 0 && current_length <= allocated_length / 4) [[unlikely]] {
      size_t new_allocated_length = allocated_length / 2;
      T *new_raw_beg = alloc.allocate(new_allocated_length);
      for (size_t i = 0; i < current_length; ++i) {
        std::allocator_traits<decltype(alloc)>::construct(alloc, new_raw_beg + i, std::move(raw_beg[i]));
        std::allocator_traits<decltype(alloc)>::destroy(alloc, raw_beg + i);
      }
      alloc.deallocate(raw_beg, allocated_length);
      raw_beg = new_raw_beg;
      raw_end = raw_beg + current_length;
      allocated_length = new_allocated_length;
    }
  }
};
template <typename T>
std::allocator<T> vector<T>::alloc;

}  // namespace sjtu

#endif
