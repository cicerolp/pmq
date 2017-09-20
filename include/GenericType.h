//
// Created by cicerolp on 9/18/17.
//

#pragma once

#include "stde.h"

class GenericType {
 public:
  // this type represents a pointer-to-value_type.
  using pointer = const GenericType *;
  // this type represents a reference-to-value_type.
  using reference = const GenericType &;

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
};

class TweetType {
 public:
  // this type represents a pointer-to-value_type.
  using pointer = const TweetType *;
  // this type represents a reference-to-value_type.
  using reference = const TweetType &;

  TweetType() = default;

  TweetType(uint64_t time, float lat, float lon)
      : _time(time), _lat(lat), _lon(lon) {
  }

  ~TweetType() = default;

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

  inline bool operator==(const TweetType &rhs) const {
    return _time == rhs._time &&
        _lat == rhs._lat &&
        _lon == rhs._lon;
  }
  inline bool operator!=(const TweetType &rhs) const {
    return !(*this == rhs);
  }

  friend inline std::ostream &operator<<(std::ostream &os, const TweetType &data) {
    os << "lat: " << data._lat << " lon: " << data._lon << " time: " << data._time;
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
