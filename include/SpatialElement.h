#pragma once
#include "types.h"
#include "pma_util.h"

// Quadtree node
class SpatialElement {
public:
   SpatialElement(const spatial_t& tile);
   ~SpatialElement() = default;

   using node_ptr = std::unique_ptr<SpatialElement>;
   using container_t = std::array<node_ptr, 4>;

   void update(pma_struct* pma, const map_t_it& it_begin, const map_t_it& it_end);
   void query_tile(const region_t& region, std::vector<SpatialElement*>& subset);
   void query_region(const region_t& region, std::vector<SpatialElement*>& subset);
      
   inline uint32_t begin() const {
      return _beg;
   }
   inline uint32_t end() const {
      return _end;
   }
   inline uint64_t code() const {
      return _el.code;
   }
   inline uint32_t zoom() const {
      return _el.z;
   }

   inline container_t::const_iterator  get_first_child() const {
       return std::find_if(_container.begin(), _container.end(),[](const node_ptr& el) { return el != nullptr;});
   }

   inline container_t::const_reverse_iterator get_last_child() const {
       return std::find_if(_container.rbegin(), _container.rend(),[](const node_ptr& el) { return el != nullptr;});
   }


private:
   void aggregate_tile(uint32_t zoom, std::vector<SpatialElement*>& subset);

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

   uint32_t _beg, _end;
   spatial_t _el;   

   container_t _container;


};
