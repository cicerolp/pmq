#include "ExplicitDenseVectorCtn.h"

ExplicitDenseVectorCtn::ExplicitDenseVectorCtn() {
  _quadtree = std::make_unique<QuadtreeIntf>(spatial_t(0, 0, 0));
}

// build container
duration_t ExplicitDenseVectorCtn::create(uint32_t size) {
  Timer timer;

  timer.start();
  _container.reserve(size);
  timer.stop();

  return {duration_info("create", timer)};
}

// update container
duration_t ExplicitDenseVectorCtn::insert(std::vector<elttype> batch) {
  duration_t duration;
  Timer timer;

  if (_quadtree == nullptr) {
    return {duration_info("Error", timer)};
  }

  // insert start
  timer.start();
  // we need the batch sorted
  sort(batch);

  // first element in batch
  auto ref = batch.front();
  // find insertion point before sorting
  auto ref_it = std::lower_bound(_container.begin(), _container.end(), ref);
  // store initial diff index
  _diff_index = (uint32_t) (ref_it - _container.begin());

  // insert batch at end
  _container.insert(_container.end(), batch.begin(), batch.end());

  // sorting algorithm
  sort(_container);

  // insert end
  timer.stop();
  duration.emplace_back("Insert", timer);

  // diff start
  timer.start();
  diff_cnt keys = diff();

  // diff end
  timer.stop();
  duration.emplace_back("ModifiedKeys", timer);

  // quadtree update start
  timer.start();
  _quadtree->update(keys.begin(), keys.end());

  // quadtree update end
  timer.stop();
  duration.emplace_back("QuadtreeUpdate", timer);

  return duration;
}

// apply function for every el<valuetype>
duration_t ExplicitDenseVectorCtn::scan_at_region(const region_t &region, scantype_function __apply) {
  Timer timer;
  timer.start();

  std::vector<QuadtreeIntf *> subset;
  _quadtree->query_region(region, subset);

  for (auto &el : subset) {
    for (uint32_t index = el->begin(); index < el->end(); ++index) {
      __apply(_container[index].value);
    }
  }

  timer.stop();
  return {duration_info("total", timer)};
}

// apply function for every spatial area/region
duration_t ExplicitDenseVectorCtn::apply_at_tile(const region_t &region, applytype_function __apply) {
  Timer timer;
  timer.start();

  std::vector<QuadtreeIntf *> subset;
  _quadtree->query_tile(region, subset);

  for (auto &el : subset) {
    __apply(el->el(), el->end() - el->begin());
  }

  timer.stop();

  return {duration_info("total", timer)};
}

duration_t ExplicitDenseVectorCtn::apply_at_region(const region_t &region, applytype_function __apply) {
  Timer timer;
  timer.start();

  std::vector<QuadtreeIntf *> subset;
  _quadtree->query_region(region, subset);

  for (auto &el : subset) {
    __apply(el->el(), el->end() - el->begin());
  }

  timer.stop();

  return {duration_info("total", timer)};
}

diff_cnt ExplicitDenseVectorCtn::diff() {
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

void ExplicitDenseVectorCtn::clear_diff() {
  _diff_index = 0;
}
