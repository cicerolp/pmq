#include "stde.h"
#include "QuadtreeIntf.h"

QuadtreeIntf::QuadtreeIntf(const spatial_t& tile) : _el(tile) {
   _beg = 0;
   _end = 0;
}

/**
 * @brief QuadtreeIntf::update inserts a list of element in a quatree
 * @param range A list of elements that have been modified in the quadtree (added or updated) with their new ranges in the ContainerIntf.
 *
 * @note we don't support deletes;
 * @return
 */
void QuadtreeIntf::update(const diff_it& it_begin, const diff_it& it_end) {
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

   // update first child index
   set_child_index(index);

   // update intermediate valid nodes
   while ((++it_curr) != it_end) {
      int curr_index = (*it_curr).get_index(z_diff_2);

      if (curr_index != index) {
         get_node(x, y, _el.z + 1, index)->update(it_previous_begin, it_curr);

         it_previous_begin = it_curr;
         index = curr_index;
      }
   }

   // update last valid node
   get_node(x, y, _el.z + 1, index)->update(it_previous_begin, it_curr);

   // update last child index
   set_child_index(index);

   // update beg index
   _beg = (*get_first_child())->begin();

   // update end index
   _end = (*get_last_child())->end();
}

/**
 * @brief QuadtreeIntf::query_tile :
 * @param region
 * @param subset
 *
 * - Goes down the quadtree to the zoom level of the selected region;
 * - If the node it completly inside the region, puts all the 8-descendents into the returned subset.
 */
void QuadtreeIntf::query_tile(const region_t& region, std::vector<QuadtreeIntf*>& subset) {
   if (region.z() == _el.z && region.cover(_el)) {
      return aggregate_tile(_el.z + 8, subset);
   } else if (region.z() > _el.z) {
      if (_container[0] != nullptr) _container[0]->query_tile(region, subset);
      if (_container[1] != nullptr) _container[1]->query_tile(region, subset);
      if (_container[2] != nullptr) _container[2]->query_tile(region, subset);
      if (_container[3] != nullptr) _container[3]->query_tile(region, subset);
   }
}
/**
 * @brief QuadtreeIntf::query_region : finds the smallest set of quadtree-nodes that are completely inside the \a region
 * @param region
 * @param subset : the resulting subset
 */
void QuadtreeIntf::query_region(const region_t& region, std::vector<QuadtreeIntf*>& subset) {
   if (region.cover(_el)) {
      subset.emplace_back(this);
   } else if (region.z() > _el.z) {
      if (_container[0] != nullptr) _container[0]->query_region(region, subset);
      if (_container[1] != nullptr) _container[1]->query_region(region, subset);
      if (_container[2] != nullptr) _container[2]->query_region(region, subset);
      if (_container[3] != nullptr) _container[3]->query_region(region, subset);
   }
}

/**
 * @brief QuadtreeIntf::aggregate_tile : creates as list with all the "descendent" nodes \a zoom levels deeper in the quadtree.
 * @param zoom : the relative depth to search for descedants
 * @param subset : the list os descendents at depth \a zoom or leaf nodes in there is not enough depth.
 */
void QuadtreeIntf::aggregate_tile(uint32_t zoom, std::vector<QuadtreeIntf*>& subset) {
   if (_el.leaf || _el.z == zoom) {
      subset.emplace_back(this);
   } else {
      if (_container[0] != nullptr) _container[0]->aggregate_tile(zoom, subset);
      if (_container[1] != nullptr) _container[1]->aggregate_tile(zoom, subset);
      if (_container[2] != nullptr) _container[2]->aggregate_tile(zoom, subset);
      if (_container[3] != nullptr) _container[3]->aggregate_tile(zoom, subset);
   }
}

uint32_t QuadtreeIntf::check_child_consistency() const {
   if (_el.leaf) return 0;

   // parent begin == fist child's begin
   if (_beg != (*get_first_child())->begin()) return 1;

   // parent end == last child's end
   if (_end != (*get_last_child())->end()) return 1;

   // child.end >= next_child.begin (because a same segment can contain elements from different nodes.
   for (auto child = get_first_child(); child < get_last_child(); child = get_next_child(child + 1)) {
      if ((*child)->end() < (*(get_next_child(child)))->begin()) return 1;
   }

   return 0;
}

uint32_t QuadtreeIntf::check_child_level() const {
   for (auto& child : _container) {
      if (child != nullptr) {
         if (child->check_child_consistency()) {
            return 1;
         }
      }
   }

   return 0;
}

uint32_t QuadtreeIntf::check_count(const ContainerIntf& container) const {
   if (!_el.leaf) {
      // count elements in this node
      uint32_t curr_count = 0;
      container.count(begin(), end(), _el, curr_count);

      // count elements in the child notes
      uint32_t child_count = 0;
      for (auto& ptr : _container) {
         if (ptr) container.count(ptr->begin(), ptr->end(), ptr->_el, child_count);
      }

      // parent and child count should match
      return (curr_count - child_count);
   }

   return 0;
}
