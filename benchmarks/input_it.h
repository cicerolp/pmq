#pragma once

#include "stde.h"
#include "types.h"
#include "date_util.h"

#include <random>

class input_it : public std::iterator<std::forward_iterator_tag, elttype, std::ptrdiff_t, elttype *, elttype &> {
 public:
  typedef size_t size_type;

  virtual ~input_it() = default;

  pointer operator->() /*const*/ {
    return &_curr_value;
  }

  reference operator*() /*const*/ {
    return _curr_value;
  }

  virtual bool operator!=(const input_it &other) const = 0;

  virtual input_it &operator++() = 0;

  virtual size_type size() const = 0;

  virtual void reset() = 0;

 protected:
  elttype _curr_value;
};

class input_tweet_it : public input_it {
 public:
  typedef size_t size_type;

  static input_tweet_it begin(const std::string &fname) {
    return input_tweet_it(fname);
  }

  static input_tweet_it end(const std::string &fname) {
    return input_tweet_it(fname, true);
  }

  input_tweet_it(const input_tweet_it &elt) {
    _curr_elt = elt._curr_elt;
    _size = elt._size;
    _fname = elt._fname;

    _infile = std::ifstream(elt._fname, std::ios::binary);
  }

  input_tweet_it(const std::string &fname, bool end = false) : input_it() {

    _fname = fname;

    _infile = std::ifstream(fname, std::ios::binary);

    if (!_infile.is_open()) {
      std::cerr << "Error Opening File." << std::endl;
    }

    _infile.unsetf(std::ios_base::skipws);

    reset();

    tweet_t record;
    size_t record_size = 19; //file record size

    while (true) {
      _infile.read((char *) &record, record_size); // Must read BEFORE checking EOF
      if (_infile.eof()) break;

      _size++;
    }

    reset();

    if (end) {
      _curr_elt = _size;
    }
  }

  virtual ~input_tweet_it() {
    _infile.close();
  };

  input_it &operator++() override {
    if (_curr_elt < _size) {
      tweet_t record;
      size_t record_size = 19; //file record size

      _infile.read((char *) &record, record_size); // Must read BEFORE checking EOF
      _curr_value = elttype(record, 25);

      ++_curr_elt;
    }

    return *this;
  }

  bool operator!=(const input_it &other) const {
    return *this != dynamic_cast<const input_tweet_it &>(other);
  }

  bool operator!=(const input_tweet_it &other) const {
    return _curr_elt != other._curr_elt;
  };

  size_type size() const override {
    return _size;
  }

  void reset() override {
    // rewind file
    _infile.clear();
    _infile.seekg(0);

    // skip file header
    for (int i = 0; i < 32; ++i) {
      _infile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    _curr_elt = 0;
  }

 protected:
  std::string _fname;
  size_type _curr_elt{0};
  size_type _size{0};
  std::ifstream _infile;
};

class input_random_it : public input_it {
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
      tweet_t record;
      record.longitude = (float) lon(_gen);
      record.latitude = (float) lat(_gen);
      record.time = _curr_elt / _timestamp;

      _curr_value = elttype(record, 25);

      ++_curr_elt;
    }

    return *this;
  }

  bool operator!=(const input_it &other) const {
    return *this != dynamic_cast<const input_random_it &>(other);
  }

  bool operator!=(const input_random_it &other) const {
    return _curr_elt != other._curr_elt;
  };

  size_type size() const override {
    return _size;
  }

  void reset() override {
    _curr_elt = 0;
  }

 protected:
  size_type _timestamp;

  size_type _size;
  size_type _curr_elt{0};

  std::mt19937 _gen;

  std::uniform_real_distribution<> lon{-180, 180};
  std::uniform_real_distribution<> lat{-85, 85};
};