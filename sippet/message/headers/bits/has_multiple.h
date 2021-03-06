// Copyright (c) 2013 The Sippet Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SIPPET_MESSAGE_HEADERS_BITS_HAS_MULTIPLE_H_
#define SIPPET_MESSAGE_HEADERS_BITS_HAS_MULTIPLE_H_

#include <vector>

namespace sippet {

template<class T> class has_multiple {
public:
  typedef typename std::vector<T>::value_type value_type;
  typedef typename std::vector<T>::iterator iterator;
  typedef typename std::vector<T>::const_iterator const_iterator;
  typedef typename std::vector<T>::reverse_iterator reverse_iterator;
  typedef typename std::vector<T>::const_reverse_iterator const_reverse_iterator;
  typedef typename std::vector<T>::reference reference;
  typedef typename std::vector<T>::const_reference const_reference;
  typedef typename std::vector<T>::size_type size_type;

protected:
  has_multiple(const has_multiple &other) : items_(other.items_) {}
  has_multiple &operator=(const has_multiple &other) {
    items_ = other.items_;
    return *this;
  }
public:
  has_multiple() {}
  ~has_multiple() {}

  // Iterator creation methods.
  iterator begin()             { return items_.begin(); }
  const_iterator begin() const { return items_.begin(); }
  iterator end()               { return items_.end();   }
  const_iterator end() const   { return items_.end();   }

  // reverse iterator creation methods.
  reverse_iterator rbegin()             { return items_.rbegin(); }
  const_reverse_iterator rbegin() const { return items_.rbegin(); }
  reverse_iterator rend()               { return items_.rend();   }
  const_reverse_iterator rend() const   { return items_.rend();   }

  // Miscellaneous inspection routines.
  size_type max_size() const { return items_.max_size(); }
  bool empty() const { return items_.empty(); }

  // Front and back accessor functions...
  reference front() { return items_.front(); }
  const_reference front() const { return items_.front(); }
  reference back() { return items_.back(); }
  const_reference back() const { return items_.back(); }

  // modifiers
  template<typename InIt> void assign(InIt first, InIt last) {
    items_.assign(first, last);
  }
  void insert(iterator where, const value_type &val) {
    items_.insert(where, val);
  }
  template<typename InIt> void insert(iterator where, InIt first, InIt last) {
    items_.insert(where, first, last);
  }
  void push_back(const value_type &val) {
    items_.push_back(val);
  }

  // erase - remove a node from the controlled sequence... and delete it.
  iterator erase(iterator where) { return items_.erase(where); }

  // clear everything
  void clear() { items_.clear(); }

  // print elements
  void print(raw_ostream &os) const {
    for (const_iterator i = begin(), ie = end(); i != ie; ++i) {
      if (i != begin())
        os << ", ";
      os << *i;
    }
  }
private:
  std::vector<T> items_;
};

} // End of sippet namespace

#endif // SIPPET_MESSAGE_HEADERS_BITS_HAS_MULTIPLE_H_
