#ifndef SJTU_LIST_HPP
#define SJTU_LIST_HPP

#include <cstddef>
#include <exception>
namespace sjtu {
/**
 * a data container like std::list
 * allocate random memory addresses for data and they are doubly-linked in a
 * list.
 */
template<typename T>class list {
protected:

  class node {
public:

    /**
     * add data members and constructors & destructor
     */
    T val;
    node *prev, *next;
    node() : prev(nullptr), next(nullptr) {}

    node(const T &val, node *prev = nullptr, node *next = nullptr) :
      val(val), prev(prev), next(next) {}
  };

protected:

  /**
   * add data members for linked list as protected members
   */
  node *head, *tail;
  int element_count;

  /**
   * insert node cur before node pos
   * return the inserted node cur
   */
  node* insert(node *pos, node *cur) {
    if (pos->prev != nullptr) pos->prev->next = cur;
    cur->prev = pos->prev;
    cur->next = pos;
    pos->prev = cur;
    return cur;
  }

  /**
   * remove node pos from list (no need to delete the node)
   * return the removed node pos
   */
  node* erase(node *pos) {
    if (pos->prev != nullptr) pos->prev->next = pos->next;
    if (pos->next != nullptr) pos->next->prev = pos->prev;
    return pos;
  }

public:

  class const_iterator;
  class iterator {
private:

    /**
     * TODO add data members
     *   just add whatever you want.
     */

public:

    bool is_end;
    node *cur;
    iterator() : is_end(true), cur(nullptr) {}

    iterator(node *cur, bool is_end = false) : is_end(is_end), cur(cur) {}

    iterator operator++(int) {
      iterator res = *this;
      ++(*this);
      return res;
    }

    iterator& operator++() {
      if (cur != nullptr) {
        if (cur->next != nullptr) cur = cur->next;
        else is_end = true;
      }
      return *this;
    }

    iterator operator--(int) {
      iterator res = *this;
      --(*this);
      return res;
    }

    iterator& operator--() {
      if (is_end) is_end = false;
      else if (cur != nullptr) cur = cur->prev;
      return *this;
    }

    /**
     * TODO *it
     * throw std::exception if iterator is invalid
     */
    T& operator*() const                           {
      if ((cur == nullptr) || is_end) throw std::exception();
      return cur->val;
    }

    /**
     * TODO it->field
     * throw std::exception if iterator is invalid
     */
    T * operator->() const noexcept                 {
      if ((cur == nullptr) || is_end) throw std::exception();
      return &(cur->val);
    }

    /**
     * a operator to check whether two iterators are same (pointing to the same
     * memory).
     */
    bool operator==(const iterator &rhs) const       {
      return cur == rhs.cur && is_end == rhs.is_end;
    }

    bool operator==(const const_iterator &rhs) const {
      return cur == rhs.cur && is_end == rhs.is_end;
    }

    /**
     * some other operator for iterator.
     */
    bool operator!=(const iterator &rhs) const       {
      return cur != rhs.cur || is_end != rhs.is_end;
    }

    bool operator!=(const const_iterator &rhs) const {
      return cur != rhs.cur || is_end != rhs.is_end;
    }
  };

  /**
   * TODO
   * has same function as iterator, just for a const object.
   * should be able to construct from an iterator.
   */
  class const_iterator {
private:

    bool is_end;
    node *cur;

    /**
     * TODO add data members
     *   just add whatever you want.
     */

public:

    const_iterator() : is_end(true), cur(nullptr) {}

    const_iterator(const iterator &other) : is_end(other.is_end),
      cur(other.cur) {}

    const_iterator(node *cur, bool is_end = false) : is_end(is_end),
      cur(cur) {}

    const_iterator operator++(int) {
      const_iterator res = *this;
      ++(*this);
      return res;
    }

    const_iterator& operator++() {
      if (cur != nullptr) {
        if (cur->next != nullptr) cur = cur->next;
        else is_end = true;
      }
      return *this;
    }

    const_iterator operator--(int) {
      const_iterator res = *this;
      --(*this);
      return res;
    }

    const_iterator& operator--() {
      if (is_end) is_end = false;
      else if (cur != nullptr) cur = cur->prev;
      return *this;
    }

    /**
     * TODO *it
     * throw std::exception if iterator is invalid
     */
    const T& operator*() const                           {
      if ((cur == nullptr) || is_end) throw std::exception();
      return cur->val;
    }

    /**
     * TODO it->field
     * throw std::exception if iterator is invalid
     */
    const T * operator->() const noexcept                 {
      if ((cur == nullptr) || is_end) throw std::exception();
      return &(cur->val);
    }

    /**
     * a operator to check whether two iterators are same (pointing to the same
     * memory).
     */
    bool operator==(const iterator &rhs) const       {
      return cur == rhs.cur && is_end == rhs.is_end;
    }

    bool operator==(const const_iterator &rhs) const {
      return cur == rhs.cur && is_end == rhs.is_end;
    }

    /**
     * some other operator for iterator.
     */
    bool operator!=(const iterator &rhs) const       {
      return cur != rhs.cur || is_end != rhs.is_end;
    }

    bool operator!=(const const_iterator &rhs) const {
      return cur != rhs.cur || is_end != rhs.is_end;
    }
  };

  /**
   * TODO Constructs
   * Atleast two: default constructor, copy constructor
   */
  list() : head(nullptr), tail(nullptr), element_count(0) {}

  void copy(const list &other) {
    head          = nullptr;
    tail          = nullptr;
    element_count = 0;
    if (other.element_count != 0) {
      head      = tail = new node();
      head->val = other.head->val;
      node *p = other.head->next;
      while (p != nullptr) {
        tail->next = new node(p->val, tail);
        tail       = tail->next;
        p          = p->next;
      }
      element_count = other.element_count;
    }
  }

  list(const list &other) {
    copy(other);
  }

  /**
   * TODO Destructor
   */
  virtual ~list() {
    clear();
  }

  /**
   * TODO Assignment operator
   */
  list& operator=(const list &other) {
    if (this == &other) return *this;
    clear();
    copy(other);
    return *this;
  }

  /**
   * access the first / last element
   * throw container_is_empty when the container is empty.
   */
  const T& front() const  {
    if (element_count == 0) throw std::exception();
    return head->val;
  }

  const T& back() const   {
    if (element_count == 0) throw std::exception();
    return tail->val;
  }

  /**
   * returns an iterator to the beginning.
   */
  iterator begin() {
    return iterator(head, head == nullptr);
  }

  const_iterator cbegin() const {
    return const_iterator(head, head == nullptr);
  }

  /**
   * returns an iterator to the end.
   */
  iterator end() {
    return iterator(tail, true);
  }

  const_iterator cend() const {
    return const_iterator(tail, true);
  }

  /**
   * checks whether the container is empty.
   */
  virtual bool empty() const                {
    return element_count == 0;
  }

  /**
   * returns the number of elements
   */
  virtual size_t size() const                         {
    return element_count;
  }

  /**
   * clears the contents
   */
  virtual void clear() {
    node *p = head;
    while (p != nullptr) {
      node *p_nxt = p->next;
      delete p;
      p = p_nxt;
    }
    head          = nullptr;
    tail          = nullptr;
    element_count = 0;
  }

  /**
   * insert value before pos (pos may be the end() iterator)
   * return an iterator pointing to the inserted value
   * throw if the iterator is invalid
   */
  virtual iterator insert(iterator pos, const T &value) {
    if (!pos.is_end) {
      node *cur = new node(value);
      insert(pos.cur, cur);
      if (pos.cur == head) head = cur;
      element_count++;
      return iterator(cur);
    } else {
      if (element_count == 0) {
        head = tail = new node(value);
        element_count++;
        return iterator(tail);
      }
      tail->next = new node(value, tail);
      tail       = tail->next;
      element_count++;
      return iterator(tail);
    }
  }

  /**
   * remove the element at pos (the end() iterator is invalid)
   * returns an iterator pointing to the following element, if pos pointing to
   * the last element, end() will be returned. throw if the container is empty,
   * the iterator is invalid
   */
  virtual iterator erase(iterator pos) {
    if ((element_count == 0) || pos.is_end) throw std::exception();
    iterator res = pos;
    ++res;
    if (pos.cur == head) {
      head = head->next;
    }
    if (pos.cur == tail) {
      tail = tail->prev;
    }
    delete erase(pos.cur);
    element_count--;
    return res;
  }

  /**
   * adds an element to the end
   */
  void push_back(const T &value) {
    insert(end(), value);
  }

  /**
   * removes the last element
   * throw when the container is empty.
   */
  void pop_back() {
    if (element_count == 0) throw std::exception();
    erase(iterator(tail));
  }

  /**
   * inserts an element to the beginning.
   */
  void push_front(const T &value) {
    insert(begin(), value);
  }

  /**
   * removes the first element.
   * throw when the container is empty.
   */
  void pop_front() {
    if (element_count == 0) throw std::exception();
    erase(iterator(head));
  }
};
} // namespace sjtu

#endif // SJTU_LIST_HPP