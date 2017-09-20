#pragma once

#include "stde.h"
#include "types.h"
#include "date_util.h"
#include "GenericType.h"

#include <random>

template<typename T>
class input_it : public std::iterator<std::forward_iterator_tag, T> {
 public:
  typedef size_t size_type;
  // this type represents a pointer-to-value_type
  using pointer = const T *;
  // this type represents a reference-to-value_type
  using reference = const T &;
  /// Distance between iterators is represented as this type.
  using difference_type = std::ptrdiff_t;

  virtual ~input_it() = default;

  pointer operator->() const {
    return &_curr_value;
  }

  reference operator*() const {
    return _curr_value;
  }

  difference_type operator-(const input_it<T> &rhs) const {
    return _curr_elt - rhs._curr_elt;
  }

  bool operator<(const input_it<T> &rhs) const {
    return _curr_elt < rhs._curr_elt;
  }

  virtual bool operator!=(const input_it<T> &rhs) const {
    return _curr_elt != rhs._curr_elt;
  };

 protected:
  size_type _curr_elt{0};
  T _curr_value;
};

template <typename T>
class input_file_it : public input_it<T> {
 public:
  typedef size_t size_type;
  // this type represents a pointer-to-value_type
  using pointer = typename input_it<T>::pointer ;
  // this type represents a reference-to-value_type
  using reference = typename input_it<T>::reference ;
  /// Distance between iterators is represented as this type.
  using difference_type = typename input_it<T>::difference_type ;

  static input_file_it<T> begin(const std::shared_ptr<std::ifstream> &file_ptr) {
    return input_file_it(file_ptr);
  }

  static input_file_it<T> end(const std::shared_ptr<std::ifstream> &file_ptr) {
    auto _it = input_file_it(file_ptr);

    // seek to end
    while (true) {
      // must read BEFORE checking EOF
      file_ptr->read((char *) &_it._curr_value, T::record_size);

      if (file_ptr->eof()) {
        break;
      }

      // update current element
      _it._curr_elt++;
    }

    // update current istream position
    _it._pos = file_ptr->tellg();

    return _it;
  }

  input_file_it(const std::shared_ptr<std::ifstream> &file_ptr)
      : input_it<T>() {

    _file_ptr = file_ptr;

    if (!_file_ptr->is_open()) {
      std::cerr << "error opening file" << std::endl;
      return;
    }

    _file_ptr->unsetf(std::ios_base::skipws);

    // skip header
    seek(this->_file_ptr, T::header_size);

    _pos = _file_ptr->tellg();
  }

  virtual ~input_file_it() = default;

  input_file_it<T> operator+(difference_type n) {
    auto _it = *this;
    while (n-- != 0) {
      _it.operator++();
    }
    return _it;
  }

  input_file_it<T> &operator++() {
    if (!_file_ptr->is_open()) {
      std::cerr << "error opening file" << std::endl;
      return *this;
    }

    seek(_file_ptr, _pos);

    if (_file_ptr->eof()) {
      return *this;
    }

    // must read BEFORE checking EOF
    _file_ptr->read((char *) &this->_curr_value, T::record_size);

    if (!_file_ptr->eof()) {
      ++this->_curr_elt;
    }

    _pos = _file_ptr->tellg();

    return *this;
  }

 protected:
  static void seek(const std::shared_ptr<std::ifstream> &file_ptr, std::streampos pos) {
    // rewind file
    file_ptr->clear();
    file_ptr->seekg(pos);
  }

  std::shared_ptr<std::ifstream> _file_ptr;
  std::streampos _pos{0};
};

class input_random_it : public input_it<GenericType> {
 public:
  typedef size_t size_type;

  static input_random_it begin(size_type seed, size_type timestamp) {
    return input_random_it(seed, timestamp);
  }

  static input_random_it end(size_type seed, size_type timestamp, size_type size) {
    auto _it = input_random_it(seed, timestamp);

    // end
    _it._curr_elt = size;

    return _it;
  }

  input_random_it(size_type seed, size_type timestamp) :
      input_it(), _gen(seed), _timestamp(timestamp) {
  }

  virtual ~input_random_it() = default;

  input_random_it operator+(difference_type n) {
    auto _it = *this;

    while (n-- != 0) {
      _it.operator++();
    }
    return _it;
  }

  input_random_it &operator++() {

    float latitude = lat(_gen);
    float longitude = lon(_gen);
    uint64_t time = _curr_elt / _timestamp;

    _curr_value = GenericType(time, latitude, longitude);

    ++_curr_elt;

    return *this;
  }

 protected:
  size_type _timestamp;

  std::mt19937 _gen;

  std::uniform_real_distribution<> lon{-180, 180};
  std::uniform_real_distribution<> lat{-85, 85};
};