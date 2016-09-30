#pragma once
#include "types.h"
#include "ContainerInterface.h"

// Quadtree node
class QuadtreeNode {
public:
   QuadtreeNode(const spatial_t& tile);
   ~QuadtreeNode() = default;

   using node_ptr = std::unique_ptr<QuadtreeNode>;
   using container_t = std::array<node_ptr, 4>;
   using container_it = container_t::const_iterator;

   void update(const map_t_it& it_begin, const map_t_it& it_end);
   void query_tile(const region_t& region, std::vector<QuadtreeNode*>& subset);
   void query_region(const region_t& region, std::vector<QuadtreeNode*>& subset);

   inline uint32_t begin() const;
   inline uint32_t end() const;

   inline const spatial_t& el() const;

   inline const container_t& container() const;

   inline container_it get_first_child() const;
   inline container_it get_last_child() const;

   // get next sibling of the chiled pointed by it
   inline container_t::const_iterator get_next_child(const container_it& it) const;

   /**
    * @brief check_child_consistency
    * @return
    */
   uint32_t check_child_consistency() const;

   uint32_t check_child_level() const;

   /**
    * @brief check_count checks it the count of element in parent is equal to summation of elements on the childs
    * @param ContainerInterface
    * @return returns the difference between parent an child.
    */
   uint32_t check_count(const ContainerInterface& container) const;

private:
   void aggregate_tile(uint32_t zoom, std::vector<QuadtreeNode*>& subset);

   inline void set_child_index(uint32_t index);

   inline QuadtreeNode* get_node(uint32_t x, uint32_t y, uint32_t z, uint32_t index);

   inline static void get_child_coords(uint32_t& x, uint32_t& y, uint32_t index);

   spatial_t _el;
   uint32_t _beg, _end;
   container_t _container;
};

uint32_t QuadtreeNode::begin() const {
   return _beg;
}

inline uint32_t QuadtreeNode::end() const {
   return _end;
}

inline const spatial_t& QuadtreeNode::el() const {
   return _el;
}

const QuadtreeNode::container_t& QuadtreeNode::container() const {
   return _container;
}

QuadtreeNode::container_it QuadtreeNode::get_first_child() const {
   return _container.begin() + _el.beg_child;
}

QuadtreeNode::container_it QuadtreeNode::get_last_child() const {
   return _container.begin() + _el.end_child;
}

QuadtreeNode::container_it QuadtreeNode::get_next_child(const container_it& it) const {
   return std::find_if(it, _container.end(), [](const node_ptr& el) { return el != nullptr; });
}

void QuadtreeNode::set_child_index(uint32_t index) {
   _el.beg_child = std::min((uint32_t)_el.beg_child, index);
   _el.end_child = std::max((uint32_t)_el.end_child, index);
}

QuadtreeNode* QuadtreeNode::get_node(uint32_t x, uint32_t y, uint32_t z, uint32_t index) {
   if (_container[index] == nullptr) {
      get_child_coords(x, y, index);
      _container[index] = std::make_unique<QuadtreeNode>(spatial_t(x, y, z));
   }
   return _container[index].get();
}

void QuadtreeNode::get_child_coords(uint32_t& x, uint32_t& y, uint32_t index) {
   x *= 2;
   y *= 2;
   if (index == 1) {
      ++x;
   }
   else if (index == 2) {
      ++y;
   }
   else if (index == 3) {
      ++y;
      ++x;
   }
}