#pragma once
#include "types.h"

// Quadtree node
class SpatialElement {
public:
   SpatialElement(const spatial_t& tile);
   ~SpatialElement() = default;

   void update(const map_t_it& it_begin, const map_t_it& it_end);
   void query_tile(const region_t& region, json_ctn& subset) const;
   void query_region(const region_t& region, json_ctn& subset) const;
      
private:
   void aggregate_tile(uint32_t zoom, json_ctn& subset) const;

   inline SpatialElement* get_node(uint32_t x, uint32_t y, uint32_t z, uint32_t index) {
      if ( _container[index] == nullptr ) {
         child_coords(x, y, index);
         _container[index] = std::make_unique<SpatialElement>(spatial_t(x, y, z));
      }
      return _container[index].get();
   }
   
   inline static void child_coords(uint32_t& x, uint32_t& y, uint32_t index) {
      x *= 2;
      y *= 2;
      if (index == 1) {
         ++x;
      } else if (index == 2) {
         ++y;
      } else if (index == 3) {
         ++y;
         ++x;
      }
   }

   uint32_t beg, end;

   using node_ptr = std::unique_ptr<SpatialElement>;
   
   spatial_t el; // tile of quadtree
   std::array<node_ptr, 4> _container;
};
