//#include "stdafx.h"
#include "SpatialElement.h"
#include "mercator_util.h"
#include <vector>

SpatialElement::SpatialElement(const spatial_t& tile) {
   el = tile;
   beg = NULL;
   end = NULL;
}

/**
 * @brief SpatialElement::update inserts a list of element in a quatree
 * @param range
 * @return
 */
uint32_t SpatialElement::update(map_t &range) {

  if (el.z == 24)
      return 0;

  std::vector<map_t> quads(4);
  for (auto& m : range){
      int y = mercator_util::lat2tiley(m.first.lat,el.z);
      int x = mercator_util::lon2tilex(m.first.lgt,el.z);
      int q = mercator_util::index(x,y);

      quads[q].emplace(m);
  }




  for (int i=0 ; i < 4 ; i++ ){
      if (quads[i].empty()) continue;

      if ( _container[i] == NULL ){
        auto tile = get_tile(el.x * 2, el.y * 2, i);
        _container[i] = std::make_unique<SpatialElement> (spatial_t(tile.first,tile.second,el.z+1));
      }

      if (beg == NULL ){
          beg = (* (quads[i].begin())).second.first;
      }

      end = (* (quads[i].rbegin())).second.second;

      _container[i]->update(quads[i]);


  }

//  std::cout << "("<< el << ") " << "[" << (void * ) beg << " - " << (void*) end << "]" << std::endl;

}
/*
void SpatialElement::query_tile(const spatial_t& tile, uint64_t resolution, binned_ctn& subset, uint64_t zoom) const {
   const spatial_t& value = (*reinterpret_cast<const spatial_t*>(&el.value));

   if (value.contains(tile)) {
      if (value.leaf || zoom == tile.z) {
         return aggregate_tile(tile.z + resolution, subset, zoom);
      } else {
         if (_container[0] != nullptr) _container[0]->query_tile(tile, resolution, subset, zoom + 1);
         if (_container[1] != nullptr) _container[1]->query_tile(tile, resolution, subset, zoom + 1);
         if (_container[2] != nullptr) _container[2]->query_tile(tile, resolution, subset, zoom + 1);
         if (_container[3] != nullptr) _container[3]->query_tile(tile, resolution, subset, zoom + 1);
      }
   }
}

void SpatialElement::query_region(const region_t& region, binned_ctn& subset, uint64_t zoom) const {
   const spatial_t& value = (*reinterpret_cast<const spatial_t*>(&el.value));

   if (region.intersect(value)) {
      if (zoom == region.z || value.leaf) {
         subset.emplace_back(&el);
      } else if (region.cover(value)) {
         subset.emplace_back(&el);
      } else {
         if (_container[0] != nullptr) _container[0]->query_region(region, subset, zoom + 1);
         if (_container[1] != nullptr) _container[1]->query_region(region, subset, zoom + 1);
         if (_container[2] != nullptr) _container[2]->query_region(region, subset, zoom + 1);
         if (_container[3] != nullptr) _container[3]->query_region(region, subset, zoom + 1);
      }
   }
}

void SpatialElement::aggregate_tile(uint64_t resolution, binned_ctn& subset, uint64_t zoom) const {
   const spatial_t& value = (*reinterpret_cast<const spatial_t*>(&el.value));

   if (zoom == resolution || value.leaf) {
      subset.emplace_back(&el);
   } else {
      if (_container[0] != nullptr) _container[0]->aggregate_tile(resolution, subset, zoom + 1);
      if (_container[1] != nullptr) _container[1]->aggregate_tile(resolution, subset, zoom + 1);
      if (_container[2] != nullptr) _container[2]->aggregate_tile(resolution, subset, zoom + 1);
      if (_container[3] != nullptr) _container[3]->aggregate_tile(resolution, subset, zoom + 1);
   }
}
*/
