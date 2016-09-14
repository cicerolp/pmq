#pragma once
#include "stde.h"

extern uint32_t g_Quadtree_Depth;

struct quadtree_key {
   quadtree_key() = default;
   
   quadtree_key(uint64_t mortonCode) {
      mCode = mortonCode;
   }

   friend inline bool operator<(const quadtree_key& lhs, const quadtree_key& rhs) {
      return (lhs.mCode < rhs.mCode);
   }

   friend std::ostream& operator<<(std::ostream& stream, const quadtree_key& qtree) {
      uint32_t y, x;      
      mortonDecode_RAM(qtree.mCode, y, x);
      
      float lat = mercator_util::tiley2lat(y, g_Quadtree_Depth);
      float lon = mercator_util::tilex2lon(x, g_Quadtree_Depth);
      
      stream << "lat: " << lat << ", lon: " << lon ;      
      return stream;
   }

   /**
   * @brief getQuadrant from given morton code in a quadtree.
   * @param mCode : The morton code computed for a quadtree of depth \a max_depth;
   * @param max_depth : The max depth of the tree.
   * @param node_depth The current depth of a node.
   *
   * @note Root node has depth 0 which has only a single quadrant.
   * @return
   */
   inline
   unsigned int getQuadrant(uint32_t max_depth, uint32_t node_depth){
      return (mCode >> ((max_depth - node_depth) * 2)) & 3;
   }

   uint64_t mCode;
};


struct elinfo_t {
   elinfo_t() = default;
   elinfo_t(uint64_t value, unsigned int _begin, unsigned int _end) : key(value) {
      begin = _begin;
      end = _end;
   }
   
   quadtree_key key;
   //Segment index of the interval containing this key on the pma array;
   unsigned int begin;
   unsigned int end;
};

using map_t = std::vector<elinfo_t>;
using map_t_it = std::vector<elinfo_t>::iterator;

struct spatial_t {
   spatial_t(uint32_t x, uint32_t y, uint8_t z, uint8_t l = 1) : z(z), leaf(l) {
      code = mortonEncode_RAM(y,x);
   }
   inline bool operator==(const spatial_t& rhs) const {
      return code == rhs.code;
   }
   
   inline std::pair<uint32_t, uint32_t> get_tile() const {
      uint32_t x, y;
      mortonDecode_RAM(code, y, x);
      return {x, y};
   }

   friend std::ostream& operator<<(std::ostream& stream, const spatial_t& el) {
      stream << el.code << "/" << el.z;
      return stream;
   }

   union {
      struct {
         uint64_t code : 50;
         uint64_t z    : 5;
         uint64_t leaf : 1;
      };

      uint64_t data;
   };
};

struct region_t {
   region_t() = default;

   region_t(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint8_t z) :  z(z) {
      code0 = mortonEncode_RAM(y0, x0);
      code1 = mortonEncode_RAM(y1, x1);
   }

   inline bool cover(const spatial_t& el) const {
      if (z > el.z) {
         uint64_t min, max;
         morton_min_max(el.code, z - el.z, min, max);
         return min >= code0&& max <= code1;
         
      } else {
         return false;
      }
   }

   inline bool intersect(const spatial_t& el) const {
      if (z > el.z) {
         uint64_t min, max;
         morton_min_max(el.code, z - el.z, min, max);         
         return code0 <= max && code1 >= min;
         
      } else if (z == el.z) {
         return code0 <= el.code && code1 >= el.code;
         
      } else {
         return false;
      }
   }
   
   friend std::ostream& operator<<(std::ostream& stream, const region_t& el) {
      stream << el.code0 << "/" << el.code1 << "/" << el.z;
      return stream;
   }

   uint32_t z;
   uint64_t code0, code1;
};

struct json_t {
   spatial_t tile;
   uint32_t begin, end;
   
   json_t(const spatial_t& el, uint32_t beg, uint32_t end) 
      : tile(el), begin(beg), end(end) {}
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
      key = mortonEncode_RAM(y,x);
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
