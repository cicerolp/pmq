#pragma once
#include <stdint.h>
#include <map>
#include <memory>

#include "types.h"



// Quadtree node
class SpatialElement {


public:
   SpatialElement(const spatial_t& tile);
   ~SpatialElement() = default;

   uint32_t update( map_t &range );

//   void query_tile(const spatial_t& tile, uint64_t resolution, binned_ctn& subset, uint64_t zoom) const;
//   void query_region(const region_t& region, binned_ctn& subset, uint64_t zoom) const;

private:

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

   spatial_t el; // tile of quadtree
   std::array<std::unique_ptr<SpatialElement>, 4> _container;
};
