#include "stde.h"
#include "SpatialElement.h"

SpatialElement::SpatialElement(const spatial_t& tile) : _el(tile) {
   _beg = 0;
   _end = 0;
}

/**
 * @brief SpatialElement::update inserts a list of element in a quatree
 * @param range A list of elements that have been modified in the quadtree (added or updated) with their new ranges in the pma.
 *
 * @note we don't support deletes;
 * @return
 */
 void SpatialElement::update(pma_struct* pma, const map_t_it& it_begin, const map_t_it& it_end) {
   // empty container
   if (it_begin == it_end) return;

   if (_el.z == g_Quadtree_Depth - 1) {
      _beg = (*it_begin).begin;
      _end = (*std::prev(it_end)).end;
      return;
   }

   // node is not a leaf
   _el.leaf = 0;  
   
   uint32_t z_diff_2 = (g_Quadtree_Depth - _el.z - 1) * 2;
   
   uint32_t x, y;
   mortonDecode_RAM(_el.code, x, y);

   auto it_curr = it_begin;
   auto it_previous_begin = it_begin;
   
   uint32_t index = (*it_curr).get_index(z_diff_2);
   
   // update intermediate valid nodes
   while( (++it_curr) != it_end) {
      int curr_index = (*it_curr).get_index(z_diff_2);
      
      if (curr_index != index) {  
         get_node(x, y, _el.z + 1, index)->update(pma, it_previous_begin, it_curr);
         
         it_previous_begin = it_curr;
         index = curr_index;
      }
   }
      
   // update last valid node
   get_node(x, y, _el.z + 1, index)->update(pma, it_previous_begin, it_curr);

   // update beg index
   _beg = (*std::find_if(_container.begin(), _container.end(),[](const node_ptr& el) {
      return el != nullptr;
   }))->begin();
   
   // update end index
   _end = (*std::find_if(_container.rbegin(), _container.rend(),[](const node_ptr& el) {
      return el != nullptr;
   }))->end();

#ifndef NDEBUG
   if (!_el.leaf) {
      //count elements in this node
      uint32_t curr_count = count_elts_pma(pma, begin(), end(), code(), zoom());

      //count elements in the child notes
      uint32_t count = 0;
      for (auto& ptr : _container) {
         if (ptr) count += count_elts_pma(pma, ptr->begin(), ptr->end(), ptr->code(), ptr->zoom());
      }

      uint32_t diff = it_end - it_begin;

      //Parent and child count should match.
      if (curr_count != count)
      {
         it_curr = it_begin;
         while (it_curr != it_end) {
            auto& t = (*it_curr);

//            print_pma_keys_range(pma,_beg, _end)
            PRINTOUT("%lu %d %d\n", t.key, t.begin, t.end);

            it_curr++;
         }
      }
      
      assert(curr_count == count);
   }
#endif
}
 
void SpatialElement::query_tile(const region_t& region, std::vector<SpatialElement*>& subset) {   
   if (region.z() == _el.z && region.cover(_el)) {
         return aggregate_tile(_el.z + 8, subset);                  
   } else if (region.z() > _el.z) {
      if (_container[0] != nullptr) _container[0]->query_tile(region, subset);
      if (_container[1] != nullptr) _container[1]->query_tile(region, subset);
      if (_container[2] != nullptr) _container[2]->query_tile(region, subset);
      if (_container[3] != nullptr) _container[3]->query_tile(region, subset);
   } 
}

void SpatialElement::query_region(const region_t& region, std::vector<SpatialElement*>& subset) {
   if (region.cover(_el)) {
         subset.emplace_back(this);                  
   } else if (region.z() > _el.z) {
      if (_container[0] != nullptr) _container[0]->query_region(region, subset);
      if (_container[1] != nullptr) _container[1]->query_region(region, subset);
      if (_container[2] != nullptr) _container[2]->query_region(region, subset);
      if (_container[3] != nullptr) _container[3]->query_region(region, subset);
   }
}

int SpatialElement::check_child_consistency() const
{
    if (_el.leaf)
      return 0;

    //parent begin == fist child's begin
    if (_beg != (*get_first_child())->begin())
        return 1;

    //parent end == last child's end
    if (_end != (*get_last_child())->end())
        return 1;

    //child.end == next_child.begin
    for (auto child = get_first_child(); child < get_last_child().base()- 1; child++){
        if ( (*child)->end() != (*(child+1))->begin() )
            return 1;
    }

    return 0;
}

int SpatialElement::check_child_level() const {
    for (auto& child : _container) {
       if (child != nullptr ){
         if ( child->check_child_consistency() ) {
            PRINTOUT("ERROR");
            return 1;
         };
       };
    }
    return 0;
}

void SpatialElement::aggregate_tile(uint32_t zoom, std::vector<SpatialElement*>& subset) {
   if (_el.leaf || _el.z == zoom) {
      subset.emplace_back(this); 
   } else {
      if (_container[0] != nullptr) _container[0]->aggregate_tile(zoom, subset);
      if (_container[1] != nullptr) _container[1]->aggregate_tile(zoom, subset);
      if (_container[2] != nullptr) _container[2]->aggregate_tile(zoom, subset);
      if (_container[3] != nullptr) _container[3]->aggregate_tile(zoom, subset);
   }
}
