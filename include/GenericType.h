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

  virtual ~GenericType() = default;

  virtual inline uint64_t getTime() const;
  virtual inline void setTime(uint64_t time);

  virtual inline float getLatitude() const;
  virtual inline void setLatitude(float lat);

  virtual inline float getLongitude() const;
  virtual inline void setLongitude(float lon);

  virtual inline pointer operator->() const {
    return this;
  }

  virtual inline reference operator*() const {
    return *this;
  }

  virtual bool operator==(const GenericType &rhs) const;
  virtual bool operator!=(const GenericType &rhs) const;

  bool operator<(const GenericType &rhs) const;
  bool operator>(const GenericType &rhs) const;
  bool operator<=(const GenericType &rhs) const;
  bool operator>=(const GenericType &rhs) const;

  friend std::ostream &operator<<(std::ostream &os, const GenericType &data);

 protected:
  uint64_t _time;
  float _lat, _lon;
};

uint64_t GenericType::getTime() const {
  return _time;
}
float GenericType::getLatitude() const {
  return _lat;
}
float GenericType::getLongitude() const {
  return _lon;
}

void GenericType::setTime(uint64_t time) {
  _time = time;
}

void GenericType::setLatitude(float lat) {
  _lat = lat;
}

void GenericType::setLongitude(float lon) {
  _lon = lon;
}

bool GenericType::operator==(const GenericType &rhs) const {
  return _time == rhs._time &&
      _lat == rhs._lat &&
      _lon == rhs._lon;
}

bool GenericType::operator!=(const GenericType &rhs) const {
  return !(rhs == *this);
}

bool GenericType::operator<(const GenericType &rhs) const {
  return _time < rhs._time;
}

bool GenericType::operator>(const GenericType &rhs) const {
  return rhs < *this;
}

bool GenericType::operator<=(const GenericType &rhs) const {
  return !(rhs < *this);
}

bool GenericType::operator>=(const GenericType &rhs) const {
  return !(*this < rhs);
}

std::ostream &operator<<(std::ostream &os, const GenericType &data) {
  os << "lat: " << data._lat << " lon: " << data._lon << " time: " << data._time;
  return os;
}
