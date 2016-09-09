#pragma once
#include "stde.h"

extern uint32_t g_Quadtree_Depth;

struct quadtree_key {
   quadtree_key() = default;
   
    quadtree_key(uint64_t mortonCode){
        uint32_t x = 0;
        uint32_t y = 0;
        mortonDecode_RAM(mortonCode,x,y);
        lat = mercator_util::tiley2lat(y,g_Quadtree_Depth);
        lgt = mercator_util::tilex2lon(x,g_Quadtree_Depth);
        mCode = mortonCode;
    }

   friend inline bool operator<(const quadtree_key& lhs, const quadtree_key& rhs) {
      return (lhs.mCode < rhs.mCode);
   }

    friend std::ostream& operator<<(std::ostream& stream, const quadtree_key& qtree) {
      stream << qtree.lat << "  " << qtree.lgt ;
      return stream;
    }
    uint64_t mCode;
    float lat;
    float lgt;
};


struct elinfo_t {
   elinfo_t() = default;
   elinfo_t(uint64_t value, char* _begin, char* _end) : key(value) {
      begin = _begin;
      end = _end;
   }
   
   quadtree_key key;
   char* begin;
   char* end;
};

using map_t = std::vector<elinfo_t>;
using map_t_it = std::vector<elinfo_t>::iterator;

struct spatial_t {
   spatial_t() : spatial_t(0, 0, 0) { }

   spatial_t(uint32_t _x, uint32_t _y, uint8_t _z, uint8_t _l = 1)
      : x(_x), y(_y), z(_z), leaf(_l) { }


   inline bool contains(const spatial_t& other) const {
      if (other.z > z) {
         uint64_t n = (uint64_t)1 << (other.z - z);

         uint64_t x_min = x * n;
         uint64_t x_max = x_min + n;

         uint64_t y_min = y * n;
         uint64_t y_max = y_min + n;

         return x_min <= other.x && x_max >= other.x && y_min <= other.y && y_max >= other.y;
      } else if (other.z == z) {
         return x == other.x && y == other.y;
      } else {
         return false;
      }
   }

   friend std::ostream& operator<<(std::ostream& stream, const spatial_t& tile) {
      stream << tile.x << "/" << tile.y << "/" << tile.z;
      return stream;
   }

   union {
      struct {
         uint64_t x : 25;
         uint64_t y : 25;
         uint64_t z : 5;
         uint64_t leaf : 1;
      };

      uint64_t data;
   };
};

struct json_t {
   uint32_t count;
   spatial_t tile;

   json_t(const spatial_t& el, uint32_t sum) : tile(el), count(sum) {}
};

using json_ctn = std::vector<json_t>;

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
   tweet_t value;
   
   elttype(const tweet_t& el, uint32_t depth) : value(el) {
      uint32_t y = mercator_util::lat2tiley(value.latitude, depth);
      uint32_t x = mercator_util::lon2tilex(value.longitude, depth);
      key = mortonEncode_RAM(x,y);
   }
   
   // Pma uses only the key to sort elements.
   friend inline bool operator==(const elttype& lhs, const elttype& rhs) { 
      return (lhs.key == rhs.key); 
   }
   friend inline bool operator!=(const elttype& lhs, const elttype& rhs) { 
      return !(lhs == rhs); 
   }
   friend inline bool operator<(const elttype& lhs, const elttype& rhs) { 
      return (lhs.key < rhs.key); 
   }
   friend inline std::ostream& operator<<(std::ostream &out, const elttype& e) {
      return out << e.key; 
   }
};