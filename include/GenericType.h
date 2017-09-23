//
// Created by cicerolp on 9/18/17.
//

#pragma once

#include "stde.h"

template<std::size_t N>
class GenericType {
 public:
  // this type represents a pointer-to-value_type.
  using pointer = const GenericType<N> *;
  // this type represents a reference-to-value_type.
  using reference = const GenericType<N> &;

  GenericType() = default;

  GenericType(uint64_t time, float lat, float lon)
      : _time(time), _lat(lat), _lon(lon) {
  }

  ~GenericType() = default;

  inline uint64_t getTime() const {
    return _time;
  }
  inline void setTime(uint64_t time) {
    _time = time;
  }

  inline float getLatitude() const {
    return _lat;
  }
  inline void setLatitude(float lat) {
    _lat = lat;
  }

  inline float getLongitude() const {
    return _lon;
  }
  inline void setLongitude(float lon) {
    _lon = lon;
  }

  inline pointer operator->() const {
    return this;
  }

  inline reference operator*() const {
    return *this;
  }

  inline bool operator==(const GenericType &rhs) const {
    return _time == rhs._time &&
        _lat == rhs._lat &&
        _lon == rhs._lon;
  }
  inline bool operator!=(const GenericType &rhs) const {
    return !(*this == rhs);
  }

  friend inline std::ostream &operator<<(std::ostream &os, const GenericType &data) {
    os << "lat: " << data._lat << " lon: " << data._lon << " time: " << data._time;
    return os;
  }

 protected:
  uint64_t _time;
  float _lat, _lon;

  char _extra[N];
};

class TweetDatType {
 public:
  // file header size
  static const size_t header_size = (size_t) 764;
  // file record size
  static const size_t record_size = (size_t) 19;
  // this type represents a pointer-to-value_type.
  using pointer = const TweetDatType *;
  // this type represents a reference-to-value_type.
  using reference = const TweetDatType &;

  TweetDatType() = default;

  TweetDatType(uint64_t time, float lat, float lon)
      : _time(time), _lat(lat), _lon(lon) {
  }

  ~TweetDatType() = default;

  inline uint64_t getTime() const {
    return _time;
  }
  inline void setTime(uint64_t time) {
    _time = time;
  }

  inline float getLatitude() const {
    return _lat;
  }
  inline void setLatitude(float lat) {
    _lat = lat;
  }

  inline float getLongitude() const {
    return _lon;
  }
  inline void setLongitude(float lon) {
    _lon = lon;
  }

  inline pointer operator->() const {
    return this;
  }

  inline reference operator*() const {
    return *this;
  }

  inline bool operator==(const TweetDatType &rhs) const {
    return _time == rhs._time &&
        _lat == rhs._lat &&
        _lon == rhs._lon;
  }
  inline bool operator!=(const TweetDatType &rhs) const {
    return !(*this == rhs);
  }

  friend std::ostream &operator<<(std::ostream &os, const TweetDatType &rhs) {
    os << "lat: " << rhs._lat
       << " lon: " << rhs._lon
       << " time: " << rhs._time
       << " language: " << (uint32_t) rhs._language
       << " device: " << (uint32_t) rhs._device
       << " app: " << (uint32_t) rhs._app;
    return os;
  }

 protected:
  float _lat;
  float _lon;

  uint64_t _time;

  uint8_t _language;
  uint8_t _device;
  uint8_t _app;
};

class TweetDmpType {
 public:
  // file header size
  static const size_t header_size = (size_t) 0;
  // file record size
  static const size_t record_size = (size_t) 156;
  // this type represents a pointer-to-value_type.
  using pointer = const TweetDmpType *;
  // this type represents a reference-to-value_type.
  using reference = const TweetDmpType &;

  TweetDmpType() = default;

  TweetDmpType(uint64_t time, float lat, float lon)
      : _time(time), _lat(lat), _lon(lon) {
  }

  ~TweetDmpType() = default;

  inline uint64_t getTime() const {
    return _time;
  }
  inline void setTime(uint64_t time) {
    _time = time;
  }

  inline float getLatitude() const {
    return _lat;
  }
  inline void setLatitude(float lat) {
    _lat = lat;
  }

  inline float getLongitude() const {
    return _lon;
  }
  inline void setLongitude(float lon) {
    _lon = lon;
  }

  inline pointer operator->() const {
    return this;
  }

  inline reference operator*() const {
    return *this;
  }

  inline bool operator==(const TweetDmpType &rhs) const {
    return _time == rhs._time &&
        _lat == rhs._lat &&
        _lon == rhs._lon;
  }
  inline bool operator!=(const TweetDmpType &rhs) const {
    return !(*this == rhs);
  }

  friend std::ostream &operator<<(std::ostream &os, const TweetDmpType &rhs) {
    os << "lat: " << rhs._lat
       << " lon: " << rhs._lon
       << " time: " << rhs._time
       << " text: " << rhs._text;
    return os;
  }

 protected:
  float _lat;
  float _lon;

  uint64_t _time;

  char _text[140];
};