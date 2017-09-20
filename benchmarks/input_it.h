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

  difference_type operator-(const input_it &rhs) const {
    return _curr_elt - rhs._curr_elt;
  }

  bool operator<(const input_it &rhs) const {
    return _curr_elt < rhs._curr_elt;
  }

  virtual bool operator!=(const input_it &rhs) const {
    return _curr_elt != rhs._curr_elt;
  };

 protected:
  size_type _curr_elt{0};
  T _curr_value;
};

class input_tweet_it : public input_it<TweetType> {
 public:
  typedef size_t size_type;

  static input_tweet_it begin(const std::shared_ptr<std::ifstream> &file_ptr) {
    return input_tweet_it(file_ptr);
  }

  static input_tweet_it end(const std::shared_ptr<std::ifstream> &file_ptr) {
    auto _it = input_tweet_it(file_ptr);

    while (true) {
      // must read BEFORE checking EOF
      _it._file_ptr->read((char *) &_it._curr_value, RecordSize);

      if (_it._file_ptr->eof()) {
        break;
      }
      _it._curr_elt++;
    }

    _it.reset();

    return _it;
  }

  input_tweet_it(const std::shared_ptr<std::ifstream> &file_ptr)
      : input_it() {

    _file_ptr = file_ptr;

    if (!_file_ptr->is_open()) {
      std::cerr << "error opening file" << std::endl;
      return;
    }

    _file_ptr->unsetf(std::ios_base::skipws);

    reset();
  }

  virtual ~input_tweet_it() = default;

  input_tweet_it operator+(difference_type n) {
    auto _it = *this;
    while (n-- != 0) {
      _it.operator++();
    }
    return _it;
  }

  input_tweet_it &operator++() {
    if (!_file_ptr->is_open()) {
      std::cerr << "error opening file" << std::endl;
      return *this;
    }


    // must read BEFORE checking EOF
    _file_ptr->read((char *) &_curr_value, RecordSize);

    if (!_file_ptr->eof()) {
      ++_curr_elt;
    }

    return *this;
  }

  void reset() {
    // rewind file
    _file_ptr->clear();
    _file_ptr->seekg(0);

    // skip file header
    for (int i = 0; i < 32; ++i) {
      _file_ptr->ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    _curr_elt = 0;
  }

 protected:
  static const size_t RecordSize = 19; //file record size
  std::shared_ptr<std::ifstream> _file_ptr;
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