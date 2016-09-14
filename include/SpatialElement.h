#pragma once
#include "types.h"

// Quadtree node
class SpatialElement {
public:
   SpatialElement(const spatial_t& tile);
   ~SpatialElement() = default;

   void update(const map_t_it& it_begin, const map_t_it& it_end);
   void query_tile(const std::vector<spatial_t>& tile, json_ctn& subset) const;
   void query_region(const region_t& region, json_ctn& subset) const;
      
private:
   void aggregate_tile(uint32_t zoom, json_ctn& subset) const;

   inline static std::pair<uint32_t, uint32_t> get_tile(uint32_t x, uint32_t y, uint32_t index) {
      if (index == 1) {
         ++y;
      } else if (index == 2) {
         ++x;
      } else if (index == 3) {
         ++y;
         ++x;
      }
      return { x, y };
   }

   uint32_t beg, end;

   spatial_t el; // tile of quadtree
   std::array<std::unique_ptr<SpatialElement>, 4> _container;
};
