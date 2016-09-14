#include "stde.h"
#include "SpatialElement.h"

SpatialElement::SpatialElement(const spatial_t& tile) : el(tile) {
   beg = 0;
   end = 0;
}

/**
 * @brief SpatialElement::update inserts a list of element in a quatree
 * @param range A list of elements that have been modified in the quadtree (added or updated) with their new ranges in the pma.
 *
 * @note we don't support deletes;
 * @return
 */
 void SpatialElement::update(const map_t_it& it_begin, const map_t_it& it_end) {
   // empty container
   if (it_begin == it_end) return;


   if (el.z == g_Quadtree_Depth - 1) {
      beg = (*it_begin).begin;
      end = (*std::prev(it_end)).end;
      return;
   }

   // node is not a leaf
   el.leaf = 0;  

   std::vector<map_t_it> v_begin(4, it_end);
   std::vector<map_t_it> v_end(4);

   // initialize with an invalid value
   uint32_t index = 4;

   // splits range of modified keys into 4 quadrants
   for (auto it_curr = it_begin; it_curr != it_end; ++it_curr) {
      int q = (*it_curr).key.getQuadrant(g_Quadtree_Depth, el.z + 1);

      //gets the first element of each quadrant
      if (index != q) {
         index = q;
         v_begin[q] = it_curr; 
         v_end[q] = it_curr;
      }
      
      //increment the iterator that points to end of the current quadrant q
      v_end[q]++; 
   }
   
   uint32_t x, y;
   mortonDecode_RAM(el.code, y, x);

   for (int i = 0; i < 4 ; ++i) {
      //quadrant q is empty : continue
       if (v_begin[i] == it_end) continue;
       
       // quadrant was empty before, create it to insert the new elements
       if ( _container[i] == NULL ) { 
           auto pair = get_tile(x * 2, y * 2, i);
           _container[i] = std::make_unique<SpatialElement>(spatial_t(pair.first, pair.second, el.z + 1));
       }

      _container[i]->update(v_begin[i], v_end[i]);
   }

   // update end index
   end = (*std::find_if(_container.rbegin(),_container.rend(),[](const node_ptr& el) {
      return el != nullptr;
   }))->end;

   // update beg index
   beg = (*std::find_if(_container.begin(),_container.end(),[](const node_ptr& el) {
      return el != nullptr;
   }))->beg;

   assert(beg < end);
}
 
void SpatialElement::query_tile(const region_t& region, json_ctn& subset) const {
   if (region.intersect(el)) {
      if (region.z == el.z) {
         return aggregate_tile(el.z + 8, subset);                  
      } else {
         if (_container[0] != nullptr) _container[0]->query_tile(region, subset);
         if (_container[1] != nullptr) _container[1]->query_tile(region, subset);
         if (_container[2] != nullptr) _container[2]->query_tile(region, subset);
         if (_container[3] != nullptr) _container[3]->query_tile(region, subset);
      }
   }  
}

void SpatialElement::query_region(const region_t& region, json_ctn& subset) const {
   if (region.intersect(el)) {
      if (region.z == el.z || region.cover(el)) {
         subset.emplace_back(json_t(el, beg, end));
                  
   } else {
         if (_container[0] != nullptr) _container[0]->query_region(region, subset);
         if (_container[1] != nullptr) _container[1]->query_region(region, subset);
         if (_container[2] != nullptr) _container[2]->query_region(region, subset);
         if (_container[3] != nullptr) _container[3]->query_region(region, subset);
      }
   }
}

void SpatialElement::aggregate_tile(uint32_t zoom, json_ctn& subset) const {
   if (el.leaf || el.z == zoom) {
      subset.emplace_back(json_t(el, beg, end));
   } else {
      if (_container[0] != nullptr) _container[0]->aggregate_tile(zoom, subset);
      if (_container[1] != nullptr) _container[1]->aggregate_tile(zoom, subset);
      if (_container[2] != nullptr) _container[2]->aggregate_tile(zoom, subset);
      if (_container[3] != nullptr) _container[3]->aggregate_tile(zoom, subset);
   }
}
