//#include "stdafx.h"
#include "SpatialElement.h"
#include "mercator_util.h"
#include <vector>

#include <pma/utils/debugMacros.h>

SpatialElement::SpatialElement(const spatial_t& tile) {
   el = tile;
   beg = NULL;
   end = NULL;
}

/**
 * @brief SpatialElement::update inserts a list of element in a quatree
 * @param range A list of elements that have been modified in the quadtree (added or updated) with their new ranges in the pma.
 *
 * @note we don't support deletes;
 * @return
 */
void SpatialElement::update(map_t &range) {

   // points to beggining of the first quadrant
   if (beg == nullptr)
      beg = range.begin()->second.first;
      
   //points to the end of the last quadrant
   end = range.begin()->second.second;
   
   if (el.z == g_Quadtree_Depth -1 )
      return;
      
   // node is not a leaf
   el.leaf = 0;

   std::vector<map_t> quads(4);

   for (auto& m : range) {
      int x = mercator_util::lon2tilex(m.first.lgt, el.z + 1);
      int y = mercator_util::lat2tiley(m.first.lat, el.z + 1);
      
      int q = mercator_util::index(x,y);

      (quads[q]).emplace(m);
   }

   for (int i=0 ; i < 4 ; i++ ) {
      if (quads[i].empty()) continue;

      if ( _container[i] == NULL ) {
         auto pair = get_tile(el.x * 2, el.y * 2, i);
         _container[i] = std::make_unique<SpatialElement>(spatial_t(pair.first, pair.second, el.z + 1));
      }

      _container[i]->update(quads[i]);
   }
}

void SpatialElement::query_tile(pma_struct* pma, const spatial_t& tile, json_ctn& subset) const {
   if (el.contains(tile)) {
      if (el.leaf || el.z == tile.z) {
         return aggregate_tile(pma, tile, subset);
      } else {
         if (_container[0] != nullptr) _container[0]->query_tile(pma, tile, subset);
         if (_container[1] != nullptr) _container[1]->query_tile(pma, tile, subset);
         if (_container[2] != nullptr) _container[2]->query_tile(pma, tile, subset);
         if (_container[3] != nullptr) _container[3]->query_tile(pma, tile, subset);
      }
   }
}

void SpatialElement::aggregate_tile(pma_struct* pma, const spatial_t& tile, json_ctn& subset) const {
   if (el.z == tile.z + 8 || el.leaf) {
      subset.emplace_back(json_t(el, count_elts_pma(pma, beg, end)));
   } else {
      if (_container[0] != nullptr) _container[0]->aggregate_tile(pma, tile, subset);
      if (_container[1] != nullptr) _container[1]->aggregate_tile(pma, tile, subset);
      if (_container[2] != nullptr) _container[2]->aggregate_tile(pma, tile, subset);
      if (_container[3] != nullptr) _container[3]->aggregate_tile(pma, tile, subset);
   }
}
