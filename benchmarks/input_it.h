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

  virtual ~input_it() = default;

  pointer operator->() const {
    return &_curr_value;
  }

  reference operator*() const {
    return _curr_value;
  }

  virtual bool operator!=(const input_it &other) const {
    return _curr_elt != other._curr_elt;
  };

  virtual input_it &operator++() = 0;

  virtual size_type size() const = 0;

  virtual void reset() = 0;

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
    return input_tweet_it(file_ptr, true);
  }

  input_tweet_it(const std::shared_ptr<std::ifstream> &file_ptr, bool end = false) : input_it() {

    _file_ptr = file_ptr;

    if (!_file_ptr->is_open()) {
      std::cerr << "error opening file" << std::endl;
      return;
    }

    _file_ptr->unsetf(std::ios_base::skipws);

    reset();

    while (true) {
      // must read BEFORE checking EOF
      _file_ptr->read((char *) &_curr_value, RecordSize);

      if (_file_ptr->eof()) {
        break;
      }
      _size++;
    }

    reset();

    if (end) {
      _curr_elt = _size;
    }
  }

  virtual ~input_tweet_it() = default;

  input_it<TweetType> &operator++() override {
    if (!_file_ptr->is_open()) {
      std::cerr << "error opening file" << std::endl;
      return *this;
    }

    if (_curr_elt < _size) {
      // must read BEFORE checking EOF
      _file_ptr->read((char *) &_curr_value, RecordSize);

      ++_curr_elt;
    }

    return *this;
  }

  size_type size() const override {
    return _size;
  }

  void reset() override {
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

  size_type _size{0};
  std::shared_ptr<std::ifstream> _file_ptr;
};

class input_random_it : public input_it<GenericType> {
 public:
  typedef size_t size_type;

  static input_random_it begin(size_type size, size_type seed, size_type timestamp) {
    return input_random_it(size, seed, timestamp, size);
  }

  static input_random_it end(size_type size, size_type seed, size_type timestamp) {
    return input_random_it(size, seed, timestamp, true);
  }

  input_random_it(size_type size, size_type seed, size_type timestamp, bool end = false) :
      input_it(), _size(size), _gen(seed), _timestamp(timestamp) {
    if (end) {
      _curr_elt = _size;
    }
  }

  virtual ~input_random_it() = default;

  input_it &operator++() override {

    if (_curr_elt < _size) {

      float latitude = lat(_gen);
      float longitude = lon(_gen);
      uint64_t time = _curr_elt / _timestamp;

      _curr_value = GenericType(time, latitude, longitude);

      ++_curr_elt;
    }

    return *this;
  }

  size_type size() const override {
    return _size;
  }

  void reset() override {
    _curr_elt = 0;
  }

 protected:
  size_type _timestamp;

  size_type _size;

  std::mt19937 _gen;

  std::uniform_real_distribution<> lon{-180, 180};
  std::uniform_real_distribution<> lat{-85, 85};
};