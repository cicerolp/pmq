#pragma once
#include "stde.h"

// Quadtree node
class SpatialElement {
public:
   SpatialElement(const spatial_t& tile);
   ~SpatialElement() = default;

   void update( map_t &range );
   void query_tile(pma_struct* pma, const spatial_t& tile, json_ctn& subset) const;
      
private:
   void aggregate_tile(pma_struct* pma, const spatial_t& tile, json_ctn& subset) const;

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

   char* beg;
   char* end;

   spatial_t el; // tile of quadtree
   std::array<std::unique_ptr<SpatialElement>, 4> _container;
};
