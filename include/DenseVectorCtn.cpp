#include "DenseVectorCtn.h"

DenseVectorCtn::DenseVectorCtn() {
   _quadtree = std::make_unique<QuadtreeIntf>(spatial_t(0, 0, 0));
}

// build container
duration_t DenseVectorCtn::create(uint32_t size) {
   Timer timer;

   timer.start();
   _container.reserve(size);
   timer.stop();
}

// update container
duration_t DenseVectorCtn::insert(std::vector<elttype> batch) {
   Timer timer;
   timer.start();
   // std algorithm

   // we need the batch sorted
   sort(batch);

   // first element in batch
   auto ref = batch.front();
   // find insertion point before sorting
   auto ref_it = std::lower_bound(_container.begin(), _container.end(), ref);
   // store initial diff index
   _diff_index = ref_it - _container.begin();

   // insert batch at end
   _container.insert(_container.end(), batch.begin(), batch.end());

   // sorting algorithm
   sort(_container);

   // diff
   diff_cnt keys = diff();

   // update quadtree
   _quadtree->update(keys.begin(), keys.end());

   timer.stop();
   return timer;
}

// apply function for every el<valuetype>
duration_t DenseVectorCtn::scan_at_region(const region_t& region, scantype_function __apply) {
   Timer timer;
   timer.start();

   std::vector<QuadtreeIntf*> subset;
   _quadtree->query_region(region, subset);

   for (auto& el : subset) {
      for (uint32_t index = el->begin(); index < el->end(); ++index) {
         __apply(_container[index].value);
      }
   }

   timer.stop();

   return timer;
}

// apply function for every spatial area/region
duration_t DenseVectorCtn::apply_at_tile(const region_t& region, applytype_function __apply) {
   Timer timer;
   timer.start();

   std::vector<QuadtreeIntf*> subset;
   _quadtree->query_tile(region, subset);

   for (auto& el : subset) {
      __apply(el->el(), el->end() - el->begin());
   }

   timer.stop();

   return timer;
}

duration_t DenseVectorCtn::apply_at_region(const region_t& region, applytype_function __apply) {
   Timer timer;
   timer.start();

   std::vector<QuadtreeIntf*> subset;
   _quadtree->query_region(region, subset);

   for (auto& el : subset) {
      __apply(el->el(), el->end() - el->begin());
   }

   timer.stop();

   return timer;
}

diff_cnt DenseVectorCtn::diff() {
   diff_cnt keys;

   uint32_t begin_index = _diff_index;
   uint32_t curr_index = _diff_index;

   uint64_t last_key = _container[curr_index].key;

   while (curr_index < _container.size()) {

      if (_container[curr_index].key != last_key) {
         keys.emplace_back(last_key, begin_index, curr_index);

         begin_index = curr_index;
         last_key = _container[curr_index].key;
      }
      curr_index++;
   }

   keys.emplace_back(last_key, begin_index, curr_index);

   return keys;
}

void DenseVectorCtn::clear_diff() {
   _diff_index = 0;
}