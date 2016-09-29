#pragma once
#include "types.h"

// Quadtree node
class SpatialElement {
public:
   SpatialElement(const spatial_t& tile);
   ~SpatialElement() = default;

   using node_ptr = std::unique_ptr<SpatialElement>;
   using container_t = std::array<node_ptr, 4>;
   using container_it = container_t::const_iterator;

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

   inline const container_t& container() const {
      return _container;
   }

   inline container_it get_first_child() const {
      return _container.begin() + _el.beg_child;
   }

   inline container_it get_last_child() const {
      return _container.begin() + _el.end_child;
   }

   //Get next sibling of the chiled pointed by it
   inline container_t::const_iterator get_next_child(const container_it& it) const {
      return std::find_if(it, _container.end(), [](const node_ptr& el) { return el != nullptr; });
   }

   /**
    * @brief check_child_consistency
    * @return
    */
   int check_child_consistency() const;
   int check_child_level() const;

   /**
    * @brief check_count checks it the count of element in parent is equal to summation of elements on the childs
    * @param pma
    * @return returns the difference between parent an child.
    */
   int check_count(const pma_struct* pma) const;

private:
   void aggregate_tile(uint32_t zoom, std::vector<SpatialElement*>& subset);

   inline void set_child_index(uint32_t index) {
      _el.beg_child = std::min((uint32_t)_el.beg_child, index);
      _el.end_child = std::max((uint32_t)_el.end_child, index);
   }

   inline SpatialElement* get_node(uint32_t x, uint32_t y, uint32_t z, uint32_t index) {
      if (_container[index] == nullptr) {
         get_child_coords(x, y, index);
         _container[index] = std::make_unique<SpatialElement>(spatial_t(x, y, z));
      }
      return _container[index].get();
   }

   inline static void get_child_coords(uint32_t& x, uint32_t& y, uint32_t index) {
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
