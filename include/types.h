#pragma once
#include "stde.h"
#include "morton.h"
#include "mercator_util.h"

extern uint32_t g_Quadtree_Depth;
using json_writer = rapidjson::Writer<rapidjson::StringBuffer>;

struct spatial_t {
  spatial_t(uint64_t _code, uint8_t _z) : code(_code), z(_z), leaf(1) {
    beg_child = 3;
    end_child = 0;
  }

  spatial_t(uint32_t _x, uint32_t _y, uint8_t _z, uint8_t _l = 1) : z(_z), leaf(_l) {
    beg_child = 3;
    end_child = 0;
    code = mortonEncode_RAM(_x, _y);
  }

  inline bool operator==(const spatial_t &rhs) const {
    return code == rhs.code;
  }

  inline bool operator<(const spatial_t &rhs) const {
    return code < rhs.code;
  }

  union {
    struct {
      uint64_t code : 50;
      uint64_t z : 5;
      uint64_t leaf : 1;
      uint64_t beg_child : 2;
      uint64_t end_child : 2;
    };

    uint64_t data;
  };
};

struct code_t {
  code_t(uint64_t _code, uint32_t _z) : code(_code), z(_z) {
    uint32_t diffDepth = 25 - z;

    if (diffDepth == 0) {
      min_code = code;
      max_code = code;
    } else {
      min_code = code << 2 * (diffDepth);
      max_code = min_code | ((uint64_t) ~0 >> (64 - 2 * diffDepth));
    }
  }

  operator spatial_t() const {
    return spatial_t(code, z);
  }

  uint32_t z;
  uint64_t code, min_code, max_code;
};

struct region_t {
  region_t() = default;

  // make a region given uper-left ( _lat0,_lon0) and bottom-right (_lat1,_lon1) corners
  region_t(float _lat0, float _lon0, float _lat1, float _lon1) {
    lat0 = _lat0;
    lon0 = _lon0;
    lat1 = _lat1;
    lon1 = _lon1;

    if (lat0 < -85.0511f) lat0 = -85.0511f;
    else if (lat0 > 85.0511f) lat0 = 85.0511f;

    if (lon0 < -179.9f) lon0 = -179.9f;
    else if (lon0 > 179.9f) lon0 = 179.9f;

    if (lat1 < -85.0511f) lat1 = -85.0511f;
    else if (lat1 > 85.0511f) lat1 = 85.0511f;

    if (lon1 < -179.9f) lon1 = -179.9f;
    else if (lon1 > 179.9f) lon1 = 179.9f;

    // computes the corners of the bounding box aligned to the morton codes of a quadtree at level Z
    z = 25;

    x0 = mercator_util::lon2tilex(lon0, z);
    y0 = mercator_util::lat2tiley(lat0, z);
    x1 = mercator_util::lon2tilex(lon1, z);
    y1 = mercator_util::lat2tiley(lat1, z);

    code0 = mortonEncode_RAM(x0, y0);
    code1 = mortonEncode_RAM(x1, y1);
  }

  friend inline std::ostream &operator<<(std::ostream &out, const region_t &e) {
    out << "[y0: " << e.y0 << " lat0: " << e.lat0 << "], "
        << "[x0: " << e.x0 << " lon0: " << e.lon0 << "], "
        << "[y1: " << e.y1 << " lat1: " << e.lat1 << "], "
        << "[x1: " << e.x1 << " lon1: " << e.lon1 << "], "
        << "z: " << e.z;

    return out;
  }

  enum overlap { none, full, partial };

  overlap test(const code_t &el) const {
    uint32_t x, y;
    mortonDecode_RAM(el.code, x, y);

    if (z < el.z) {
      uint64_t n = (uint64_t) 1 << (el.z - z);

      uint64_t x_min = x0 * n;
      uint64_t x_max = x1 * n;

      uint64_t y_min = y0 * n;
      uint64_t y_max = y1 * n;

      return (x_min <= x && x_max >= x && y_min <= y && y_max >= y) ? full : none;

    } else if (z > el.z) {
      uint64_t n = (uint64_t) 1 << (z - el.z);

      uint64_t x_min = x * n;
      uint64_t x_max = x_min + n - 1;

      uint64_t y_min = y * n;
      uint64_t y_max = y_min + n - 1;

      if (x0 <= x_min && x1 >= x_max && y0 <= y_min && y1 >= y_max) {
        // el is covered by region, but region.z > el.z
        return full;
      } else if (x0 <= x_max && x1 >= x_min && y0 <= y_max && y1 >= y_min) {
        // el and region intersect
        return partial;
      } else {
        return none;
      }
    } else {
      return (x0 <= x && x1 >= x && y0 <= y && y1 >= y) ? full : none;
    }
  }

 public:
  uint64_t code0, code1;
  uint32_t x0, y0, x1, y1, z;
  float lat0, lon0, lat1, lon1;
};

#ifdef _POSIX_TIMERS
using Timer = unixTimer<CLOCK_MONOTONIC>; //POSIX Timers
#else
// time resolution: high_resolution_clock
// represents the clock with the smallest tick period provided by the implementation.
using Timer = stdTimer<std::chrono::steady_clock>; //STL timers
#endif

struct duration_info {
  double duration;
  std::string name;

  duration_info(const std::string &_name, Timer &_timer) : name(_name), duration(_timer.milliseconds()) {
  };

  duration_info(const std::string &_name, double _duration) : name(_name), duration(_duration) {
  };

  friend inline std::ostream &operator<<(std::ostream &out, const duration_info &e) {
    return out << e.name << " ; " << e.duration;
  }

};

using duration_t = std::vector<duration_info>;
inline std::ostream &operator<<(std::ostream &out, const duration_t &vec) {
  auto it = vec.begin();
  out << *it++;
  for (; it < vec.end(); it++) {
    out << " ; " << *it;
  }
  return out;
}
