//
// Created by cicerolp on 8/20/17.
//

#include "BTreeCtn.h"

BTreeCtn::~BTreeCtn() {
  _btree->clear();
}

duration_t BTreeCtn::create(uint32_t size) {
  Timer timer;
  timer.start();

  _size = size;

  _btree = std::make_unique<stx::btree_multimap<key, data>>();

  timer.stop();
  return {duration_info("create", timer)};
}

duration_t BTreeCtn::insert(std::vector<elttype> batch) {
  Timer timer;
  timer.start();

  for (const auto &elt: batch) {
    _btree->insert(elt.key, elt.value);
  }

  timer.stop();
  return {duration_info("insert", timer)};
}

duration_t BTreeCtn::insert_rm(std::vector<elttype> batch, std::function<int(const void *)> is_removed) {
  duration_t duration;

  Timer timer;
  timer.start();

  for (const auto &elt: batch) {
    _btree->insert(elt.key, elt.value);
  }

  // insert end
  timer.stop();
  duration.emplace_back("insert", timer);

  // remove start
  timer.start();
  if (_btree->size() >= _size) {
    auto it = _btree->begin();
    while (it != _btree->end()) {
      // keep valid iterator
      auto it_rm = it;
      // increment to the next iterator
      ++it;

      if (is_removed(&(*it_rm).second)) {
        _btree->erase(it_rm);
      }
    }
  }
  // remove end
  timer.stop();
  duration.emplace_back("remove", timer);

  return duration;
}

duration_t BTreeCtn::scan_at_region(const region_t &region, scantype_function __apply) {
  duration_t duration;
  Timer timer;

  timer.start();

  uint32_t refinements = scan_btree_at_region(get_parent_quadrant(region), region, __apply);

  timer.stop();
  duration.emplace_back("scan_at_region", timer);

  duration.emplace_back("scan_at_region_refinements", refinements);

  return duration;
}

uint32_t BTreeCtn::scan_btree_at_region(const code_t &el, const region_t &region, scantype_function __apply) {
  if (el.z > region.z) return 0;

  region_t::overlap overlap = region.test(el);

  if (overlap == region_t::full) {
    auto lower_it = _btree->lower_bound(el.min_code);
    auto upper_it = _btree->lower_bound(el.max_code);

    for (auto it = lower_it; it != upper_it; ++it) {
      __apply((*it).second);
    }

    if (lower_it != upper_it) {
      return 1;
    } else {
      return 0;
    }
  } else if (overlap != region_t::none) {
    uint32_t refinements = 0;

    // break code into four
    uint64_t code = el.code << 2;

    refinements += scan_btree_at_region(code_t(code | 0, (uint32_t) (el.z + 1)), region, __apply);
    refinements += scan_btree_at_region(code_t(code | 1, (uint32_t) (el.z + 1)), region, __apply);
    refinements += scan_btree_at_region(code_t(code | 2, (uint32_t) (el.z + 1)), region, __apply);
    refinements += scan_btree_at_region(code_t(code | 3, (uint32_t) (el.z + 1)), region, __apply);

    return refinements;
  } else {
    return 0;
  }
}
duration_t BTreeCtn::apply_at_tile(const region_t &region, applytype_function __apply) {
  duration_t duration;
  Timer timer;

  timer.start();

  uint32_t refinements = apply_btree_at_tile(get_parent_quadrant(region), region, __apply);

  timer.stop();
  duration.emplace_back("apply_at_tile", timer);

  duration.emplace_back("apply_at_tile_refinements", refinements);

  return duration;
}

uint32_t BTreeCtn::apply_btree_at_tile(const code_t &el, const region_t &region, applytype_function __apply) {
  if (el.z >= 25 || (int) el.z - (int) region.z > 8) return 0;

  region_t::overlap overlap = region.test(el);

  if ((int) el.z - (int) region.z < 8) {
    if (overlap != region_t::none) {
      uint32_t refinements = 0;

      // break morton code into four
      uint64_t code = el.code << 2;

      refinements += apply_btree_at_tile(code_t(code | 0, (uint32_t) (el.z + 1)), region, __apply);
      refinements += apply_btree_at_tile(code_t(code | 1, (uint32_t) (el.z + 1)), region, __apply);
      refinements += apply_btree_at_tile(code_t(code | 2, (uint32_t) (el.z + 1)), region, __apply);
      refinements += apply_btree_at_tile(code_t(code | 3, (uint32_t) (el.z + 1)), region, __apply);

      return refinements;
    }
    return 0;
  } else if (overlap == region_t::full) {
    uint32_t count = 0;

    auto lower_it = _btree->lower_bound(el.min_code);
    auto upper_it = _btree->lower_bound(el.max_code);

    while (lower_it++ != upper_it) {
      ++count;
    }

    if (count) {
      __apply(el, count);
      return 1;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}
duration_t BTreeCtn::apply_at_region(const region_t &region, applytype_function __apply) {
  duration_t duration;
  Timer timer;

  timer.start();

  uint32_t refinements = apply_btree_at_region(get_parent_quadrant(region), region, __apply);

  timer.stop();
  duration.emplace_back("apply_at_region", timer);

  duration.emplace_back("apply_at_region_refinements", refinements);

  return duration;
}

uint32_t BTreeCtn::apply_btree_at_region(const code_t &el, const region_t &region, applytype_function __apply) {
  if (el.z > region.z) return 0;

  region_t::overlap overlap = region.test(el);

  if (overlap == region_t::full) {
    uint32_t count = 0;

    auto lower_it = _btree->lower_bound(el.min_code);
    auto upper_it = _btree->lower_bound(el.max_code);

    while (lower_it++ != upper_it) {
      ++count;
    }

    if (count) {
      __apply(el, count);
      return 1;
    } else {
      return 0;
    }
  } else if (overlap != region_t::none) {
    uint32_t refinements = 0;

    // break morton code into four
    uint64_t code = el.code << 2;

    refinements += apply_btree_at_region(code_t(code | 0, (uint32_t) (el.z + 1)), region, __apply);
    refinements += apply_btree_at_region(code_t(code | 1, (uint32_t) (el.z + 1)), region, __apply);
    refinements += apply_btree_at_region(code_t(code | 2, (uint32_t) (el.z + 1)), region, __apply);
    refinements += apply_btree_at_region(code_t(code | 3, (uint32_t) (el.z + 1)), region, __apply);

    return refinements;
  } else {
    return 0;
  }
}


