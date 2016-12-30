#pragma once
#include "stde.h"
#include "morton.h"
#include "mercator_util.h"

extern uint32_t g_Quadtree_Depth;

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

   inline bool operator==(const spatial_t& rhs) const {
      return code == rhs.code;
   }

   inline bool operator<(const spatial_t& rhs) const {
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

struct region_t {
   region_t() = default;

   region_t(uint64_t _code0, uint64_t _code1, uint8_t _z) {
      z = _z;

      code0 = _code0;
      code1 = _code1;

      mortonDecode_RAM(code0, x0, y0);
      mortonDecode_RAM(code1, x1, y1);

      _xmin = std::to_string(mercator_util::tilex2lon(x0, z));
      _xmax = std::to_string(mercator_util::tilex2lon(x1 + 1, z));
      _ymin = std::to_string(mercator_util::tiley2lat(y1 + 1, z));
      _ymax = std::to_string(mercator_util::tiley2lat(y0, z));
   }

   region_t(uint32_t _x0, uint32_t _y0, uint32_t _x1, uint32_t _y1, uint8_t _z) {
      z = _z;

      x0 = _x0;
      y0 = _y0;
      x1 = _x1;
      y1 = _y1;

      code0 = mortonEncode_RAM(x0, y0);
      code1 = mortonEncode_RAM(x1, y1);

      _xmin = std::to_string(mercator_util::tilex2lon(x0, z));
      _xmax = std::to_string(mercator_util::tilex2lon(x1 + 1, z));
      _ymin = std::to_string(mercator_util::tiley2lat(y1 + 1, z));
      _ymax = std::to_string(mercator_util::tiley2lat(y0, z));
   }

   inline bool cover(const spatial_t& el) const {
      uint32_t x, y;
      mortonDecode_RAM(el.code, x, y);

      if (z <= el.z) {
         uint64_t n = (uint64_t)1 << (el.z - z);

         uint64_t x_min = x0 * n;
         uint64_t x_max = x1 * n;

         uint64_t y_min = y0 * n;
         uint64_t y_max = y1 * n;

         return x_min <= x && x_max >= x && y_min <= y && y_max >= y;
      } else {
         uint64_t n = (uint64_t)1 << (z - el.z);

         uint64_t x_min = x * n;
         uint64_t x_max = x_min + n - 1;

         uint64_t y_min = y * n;
         uint64_t y_max = y_min + n - 1;

         return x0 <= x_min && x1 >= x_max && y0 <= y_min && y1 >= y_max;
      }
   }

   const std::string& xmin() const { return _xmin; }
   const std::string& xmax() const { return _xmax; }
   const std::string& ymin() const { return _ymin; }
   const std::string& ymax() const { return _ymax; }

public:
   uint64_t code0, code1;
   uint32_t x0, y0, x1, y1, z;
   std::string _xmin, _xmax, _ymin, _ymax;
};

struct tweet_t {
   float latitude;
   float longitude;

   uint64_t time;

   uint8_t language;
   uint8_t device;
   uint8_t app;
};

using valuetype = tweet_t;

struct elttype {
   uint64_t key;
   valuetype value;

   elttype() {
   };

   elttype(const tweet_t& el, uint32_t depth) : value(el) {
      uint32_t y = mercator_util::lat2tiley(value.latitude, depth);
      uint32_t x = mercator_util::lon2tilex(value.longitude, depth);
      key = mortonEncode_RAM(x, y);
   }

   friend inline bool operator<(const elttype& lhs, const elttype& rhs) {
      return (lhs.key < rhs.key);
   }

   friend inline std::ostream& operator<<(std::ostream& out, const elttype& e) {
      return out << e.key;
   }
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

   duration_info(const std::string& _name, Timer& _timer) : name(_name), duration(_timer.milliseconds()) {
   };

   duration_info(const std::string& _name, double _duration) : name(_name), duration(_duration) {
   };
};

using duration_t = std::vector<duration_info>;
using json_writer = rapidjson::Writer<rapidjson::StringBuffer>;
