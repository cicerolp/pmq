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
   
   uint32_t z_diff_2 = (g_Quadtree_Depth - el.z - 1) * 2;
   
   uint32_t x, y;
   mortonDecode_RAM(el.code, x, y);

   auto it_curr = it_begin;
   auto it_last = it_begin;
   
   uint32_t index = (*it_curr).key.get_index(z_diff_2);
   
   // update intermediate valid nodes
   while(++it_curr != it_end) {
      int curr_index = (*it_curr).key.get_index(z_diff_2);
      
      if (curr_index != index) {  
         get_node(x, y, el.z + 1, index)->update(it_last, it_curr);
         
         it_last = it_curr;
         index = curr_index;
      }
   }
      
   // update last valid node
   get_node(x, y, el.z + 1, index)->update(it_last, it_curr);

   // update beg index
   beg = (*std::find_if(_container.begin(),_container.end(),[](const node_ptr& el) {
      return el != nullptr;
   }))->beg;
   
   // update end index
   end = (*std::find_if(_container.rbegin(),_container.rend(),[](const node_ptr& el) {
      return el != nullptr;
   }))->end;
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
