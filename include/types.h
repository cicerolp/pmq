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
  region_t(float lat0, float lon0, float lat1, float lon1) {
    if (lat0 < -85.051132f) lat0 = -85.051132f;
    else if (lat0 > 85.051132f) lat0 = 85.051132f;

    if (lon0 < -180.f) lon0 = -180.f;
    else if (lon0 > 180.f) lon0 = 180.f;

    if (lat1 < -85.051132f) lat1 = -85.051132f;
    else if (lat1 > 85.051132f) lat1 = 85.051132f;

    if (lon1 < -180.f) lon1 = -180.f;
    else if (lon1 > 180.f) lon1 = 180.f;

    // computes the corners of the bounding box aligned to the morton codes of a quadtree at level Z
    z = 25;

    x0 = mercator_util::lon2tilex(lon0, z);
    y0 = mercator_util::lat2tiley(lat0, z);
    x1 = mercator_util::lon2tilex(lon1, z);
    y1 = mercator_util::lat2tiley(lat1, z);

    code0 = mortonEncode_RAM(x0, y0);
    code1 = mortonEncode_RAM(x1, y1);

    // final bb center
    lon = mercator_util::tilex2lon((((x1 - x0) / 2.f) + x0) + 0.5f, z);
    lat = mercator_util::tiley2lat((((y1 - y0) / 2.f) + y0) + 0.5f, z);
  }

  region_t(float _lat, float _lon, float radius) {
    static const float PI_180_INV = 180.f / (float) M_PI;
    static const float PI_180 = (float) M_PI / 180.f;
    static const float r_earth = 6378.f;

    lon = _lon;
    lat = _lat;

    float lat0 = lat + (radius / r_earth) * (PI_180_INV);

    if (lat0 < -85.051132f) lat0 = -85.051132f;
    else if (lat0 > 85.051132f) lat0 = 85.051132f;

    float lon0 = lon - (radius / r_earth) * (PI_180_INV) / cos(lat * PI_180);

    if (lon0 < -180.f) lon0 = -180.f;
    else if (lon0 > 180.f) lon0 = 180.f;

    float lat1 = lat - (radius / r_earth) * (PI_180_INV);

    if (lat1 < -85.051132f) lat1 = -85.051132f;
    else if (lat1 > 85.051132f) lat1 = 85.051132f;

    float lon1 = lon + (radius / r_earth) * (PI_180_INV) / cos(lat * PI_180);

    if (lon1 < -180.f) lon1 = -180.f;
    else if (lon1 > 180.f) lon1 = 180.f;

    z = 8;

    x0 = mercator_util::lon2tilex(lon0, z);
    y0 = mercator_util::lat2tiley(lat0, z);
    x1 = mercator_util::lon2tilex(lon1, z);
    y1 = mercator_util::lat2tiley(lat1, z);

    code0 = mortonEncode_RAM(x0, y0);
    code1 = mortonEncode_RAM(x1, y1);
  }

  region_t(uint32_t _x0, uint32_t _y0, uint32_t _x1, uint32_t _y1, uint8_t _z) {
    z = _z;

    x0 = _x0;
    y0 = _y0;
    x1 = _x1;
    y1 = _y1;

    code0 = mortonEncode_RAM(x0, y0);
    code1 = mortonEncode_RAM(x1, y1);

    lon = mercator_util::tilex2lon((((x1 - x0) / 2.f) + x0) + 0.5f, z);
    lat = mercator_util::tiley2lat((((y1 - y0) / 2.f) + y0) + 0.5f, z);
  }

  friend inline std::ostream &operator<<(std::ostream &out, const region_t &e) {
    out << "[y0: " << e.y0 << " lat0: " << mercator_util::tiley2lat(e.y0, e.z) << "], "
        << "[x0: " << e.x0 << " lon0: " << mercator_util::tilex2lon(e.x0, e.z) << "], "
        << "[y1:" << e.y1 << " lat1: " << mercator_util::tiley2lat(e.y1, e.z) << "], "
        << "[x1: " << e.x1 << " lon1: " << mercator_util::tilex2lon(e.x1, e.z) << "], "
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
  float lat, lon;
  uint64_t code0, code1;
  uint32_t x0, y0, x1, y1, z;
};

struct tweet_all_t {
  float latitude;
  float longitude;

  uint64_t time;

  uint8_t language;
  uint8_t device;
  uint8_t app;

  static void write(const tweet_all_t &el, json_writer &writer) {
    writer.StartArray();
    writer.Uint((unsigned int) el.time);
    writer.Uint(el.language);
    writer.Uint(el.device);
    writer.Uint(el.app);
    writer.EndArray();
  }

  bool operator==(const tweet_all_t &rhs) const {
    // simplified comparison
    return (time == rhs.time) && (latitude == rhs.latitude) && (longitude == rhs.longitude);
  };

  friend inline std::ostream &operator<<(std::ostream &out, const tweet_all_t &e) {
    return out << e.latitude << "; " << e.longitude << "; " << e.time << "; " << (int) e.language << "; "
               << (int) e.device << "; " << (int) e.app;
  }
};

struct tweet_text_t {
  float latitude;
  float longitude;
  uint64_t time;
  char text[140];

  static void write(const tweet_text_t &el, json_writer &writer) {
    writer.StartArray();
    writer.Uint((unsigned int) el.time);
    writer.String(el.text);
    writer.EndArray();
  }

  bool operator==(const tweet_text_t &rhs) const {
    // simplified comparison
    return (time == rhs.time) && (latitude == rhs.latitude) && (longitude == rhs.longitude);
  };

  friend inline std::ostream &operator<<(std::ostream &out, const tweet_text_t &e) {
    return out << e.latitude << "; " << e.longitude << "; " << e.text;
  }

};

//using tweet_t = tweet_text_t;
using tweet_t = tweet_all_t;
using valuetype = tweet_t;

// JULIO : elttype conflict with definition on pma / test_utils.h
// Check how to fix this later

template<typename valuetype_>
struct elttype_pmq {
  uint64_t key;
  valuetype_ value;

  elttype_pmq() {
  };

  elttype_pmq(const valuetype_ &el, uint32_t depth) : value(el) {
    uint32_t y = mercator_util::lat2tiley(value.latitude, depth);
    uint32_t x = mercator_util::lon2tilex(value.longitude, depth);
    key = mortonEncode_RAM(x, y);
  }

  friend inline bool operator<(const elttype_pmq &lhs, const elttype_pmq &rhs) {
    return (lhs.key < rhs.key);
  }

  friend inline std::ostream &operator<<(std::ostream &out, const elttype_pmq &e) {
    return out << e.key;
  }
};

using elttype = elttype_pmq<tweet_t>;

struct triggers_t {
  float frequency;
};

struct elinfo_t {
  elinfo_t() = default;

  elinfo_t(uint64_t value, uint32_t _begin, uint32_t _end) {
    // morton code
    key = value;

    // segments interval
    begin = _begin;
    end = _end;
  }

  /**
   * @brief get_index returns the quadrant id of this node
   * @param z_diff_2 is the depth from this node to the bottom of the tree X 2;
   * @return
   */
  inline uint32_t get_index(uint32_t z_diff_2) const {
    return (key >> z_diff_2) & 3;
  }

  uint64_t key;
  uint32_t begin, end;
};

using diff_cnt = std::vector<elinfo_t>;
using diff_it = std::vector<elinfo_t>::iterator;

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
