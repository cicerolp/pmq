#include "stde.h"
#include "PMQ.h"

PMQ::PMQ(int argc, char *argv[], int _refLevel) : GeoCtnIntf(_refLevel) {
  seg_size = cimg_option("-s", 8, "PMQ arg: segment size");
  tau_0 = cimg_option("-t0", 0.92f, "PMQ arg: tau_0");
  tau_h = cimg_option("-th", 0.7f, "PMQ arg: tau_h");
  rho_0 = cimg_option("-r0", 0.08f, "PMQ arg: rho_0");
  rho_h = cimg_option("-rh", 0.3f, "PMQ arg: rho_0");
}

PMQ::~PMQ() { if (_pma != nullptr) pma::destroy_pma(_pma); }

duration_t PMQ::create(uint32_t size) {
  Timer timer;

  timer.start();
  _pma = (struct pma_struct *) pma::build_pma(size, sizeof(valuetype), tau_0, tau_h, rho_0, rho_h, seg_size);
  timer.stop();

  return {duration_info("create", timer)};
}

duration_t PMQ::insert(std::vector<elttype> batch) {
  Timer timer;

  if (_pma == nullptr) { return {duration_info("Error", timer)}; }

  // insert start
  timer.start();

  //gfx::timsort(batch.begin(), batch.end());
  std::sort(batch.begin(), batch.end());

  void *begin = (void *) (&batch[0]);
  void *end = (void *) ((char *) (&batch[0]) + (batch.size()) * sizeof(elttype));

  pma::batch::add_array_elts(_pma, begin, end, comp<uint64_t>);

  // insert end
  timer.stop();

  return {duration_info("insert", timer)};
}

duration_t PMQ::insert_rm(std::vector<elttype> batch, std::function<int(const void *)> is_removed) {
  duration_t duration;
  Timer timer;

  if (_pma == nullptr) { return {duration_info("Error", timer)}; }

  // insert start
  timer.start();

  std::sort(batch.begin(), batch.end());

  void *begin = (void *) (&batch[0]);
  void *end = (void *) ((char *) (&batch[0]) + (batch.size()) * sizeof(elttype));

  int rm_count = pma::batch::add_rm_array_elts(_pma, begin, end, comp<uint64_t>, is_removed);
  // insert end
  timer.stop();
  duration.emplace_back("insert", timer);

  if (rm_count > 0) {
    duration.emplace_back("remove", timer);
  }

  return duration;
}

/**
 * @brief GeoHash::scan_at_region applies the function __apply on the elements inside a region of the implicit quadtree.
 * @param region A square region in the implict quadtree.
 * @param __apply
 * @return
 */
duration_t PMQ::scan_at_region(const region_t &region, scantype_function __apply) {
  duration_t duration;
  Timer timer;

  if (_pma == nullptr) {
    duration.emplace_back("scan_at_region", 0);
    duration.emplace_back("scan_at_region_refinements", 0);
    return duration;
  }

  timer.start();

  // recursive search on the pma
  auto curr_seg = pma_seg_it::begin(_pma);
  uint32_t refinements = scan_pma_at_region(get_parent_quadrant(region), curr_seg, region, __apply);

  timer.stop();
  duration.emplace_back("scan_at_region", timer);

  duration.emplace_back("scan_at_region_refinements", refinements);

  return duration;
}

duration_t PMQ::apply_at_tile(const region_t &region, applytype_function __apply) {
  duration_t duration;
  Timer timer;

  if (_pma == nullptr) {
    duration.emplace_back("apply_at_tile", 0);
    duration.emplace_back("apply_at_tile_refinements", 0);
    return duration;
  }

  timer.start();

  auto curr_seg = pma_seg_it::begin(_pma);
  uint32_t refinements = apply_pma_at_tile(get_parent_quadrant(region), curr_seg, region, __apply);

  timer.stop();
  duration.emplace_back("apply_at_tile", timer);

  duration.emplace_back("apply_at_tile_refinements", refinements);

  return duration;
}

duration_t PMQ::apply_at_region(const region_t &region, applytype_function __apply) {
  duration_t duration;
  Timer timer;

  if (_pma == nullptr) {
    duration.emplace_back("apply_at_region", 0);
    duration.emplace_back("apply_at_region_refinements", 0);
    return duration;
  }

  timer.start();

  // recursive search on the pma
  auto curr_seg = pma_seg_it::begin(_pma);
  uint32_t refinements = apply_pma_at_region(get_parent_quadrant(region), curr_seg, region, __apply);

  timer.stop();
  duration.emplace_back("apply_at_region", timer);

  duration.emplace_back("apply_at_region_refinements", refinements);

  return duration;
}

/**
 * @brief GeoHash::scan_pma_at_region
 * @param el : a prefix mortonCode on the implicit quadtree and is detph (z)
 * @param seg : a lower bound on the segment that contains the elements inside region;
 * @param region : a square region on implicit quadtree.
 * @param __apply
 *
 * Applies function __apply in every element inside region r;
 *
 */

uint32_t PMQ::scan_pma_at_region(const code_t &el, pma_seg_it &seg,
                                     const region_t &region, scantype_function __apply) {
  if (seg == pma_seg_it::end(_pma) || PMA_ELT(seg.front()) > el.max_code) return 0;

  if (el.z > region.z) return 0;

  region_t::overlap overlap = region.test(el);

  if (overlap == region_t::full) {
    if (search_pma(el, seg) == pma_seg_it::end(_pma)) {
      return 0;
    } else {
      //scans a contiguous region of the PMA
      scan_pma(el, seg, __apply);
      return 1;
    }
  } else if (overlap == region_t::partial) {

    if (el.z < getRefLevel()) {
      //Keep doing recursive refinements
      uint32_t refinements = 0;

      // break morton code into four
      uint64_t code = el.code << 2;

      refinements += scan_pma_at_region(code_t(code | 0, (uint32_t) (el.z + 1)), seg, region, __apply);
      refinements += scan_pma_at_region(code_t(code | 1, (uint32_t) (el.z + 1)), seg, region, __apply);
      refinements += scan_pma_at_region(code_t(code | 2, (uint32_t) (el.z + 1)), seg, region, __apply);
      refinements += scan_pma_at_region(code_t(code | 3, (uint32_t) (el.z + 1)), seg, region, __apply);

      return refinements;

    } else {
      if (search_pma(el, seg) == pma_seg_it::end(_pma)) {
        return 0;
      } else {
        // scans a tile checking longitude an latitude.
        scan_if_pma(el, seg, region, __apply);
        return 1;
      }
    }
  } else {
    return 0;
  }
}

uint32_t PMQ::apply_pma_at_tile(const code_t &el, pma_seg_it &seg,
                                    const region_t &region, applytype_function __apply) {
  if (seg == pma_seg_it::end(_pma) || PMA_ELT(seg.front()) > el.max_code) return 0;

  if (el.z >= 25 || (int) el.z - (int) region.z > 8) return 0;

  region_t::overlap overlap = region.test(el);

  if ((int) el.z - (int) region.z < 8) {
    if (overlap != region_t::none) {
      uint32_t refinements = 0;

      // break morton code into four
      uint64_t code = el.code << 2;

      refinements += apply_pma_at_tile(code_t(code | 0, (uint32_t) (el.z + 1)), seg, region, __apply);
      refinements += apply_pma_at_tile(code_t(code | 1, (uint32_t) (el.z + 1)), seg, region, __apply);
      refinements += apply_pma_at_tile(code_t(code | 2, (uint32_t) (el.z + 1)), seg, region, __apply);
      refinements += apply_pma_at_tile(code_t(code | 3, (uint32_t) (el.z + 1)), seg, region, __apply);

      return refinements;
    } else {
      return 0;
    }
  } else if (overlap == region_t::full) {
    if (search_pma(el, seg) == pma_seg_it::end(_pma)) {
      return 0;
    } else {
      __apply(el, count_pma(el, seg));
      return 1;
    }
  } else {
    return 0;
  }
}

uint32_t PMQ::apply_pma_at_region(const code_t &el,
                                      pma_seg_it &seg,
                                      const region_t &region,
                                      applytype_function __apply) {
  if (seg == pma_seg_it::end(_pma) || PMA_ELT(seg.front()) > el.max_code) return 0;

  if (el.z > region.z) return 0;

  region_t::overlap overlap = region.test(el);

  if (overlap == region_t::full) {
    if (search_pma(el, seg) == pma_seg_it::end(_pma)) {
      return 0;
    } else {
      __apply(el, count_pma(el, seg));
      return 1;
    }
  } else if (overlap == region_t::partial) {

    if (el.z < getRefLevel()) {
      //Keep doing recursive refinements
      uint32_t refinements = 0;

      // break morton code into four
      uint64_t code = el.code << 2;

      refinements += apply_pma_at_region(code_t(code | 0, (uint32_t) (el.z + 1)), seg, region, __apply);
      refinements += apply_pma_at_region(code_t(code | 1, (uint32_t) (el.z + 1)), seg, region, __apply);
      refinements += apply_pma_at_region(code_t(code | 2, (uint32_t) (el.z + 1)), seg, region, __apply);
      refinements += apply_pma_at_region(code_t(code | 3, (uint32_t) (el.z + 1)), seg, region, __apply);

      return refinements;

    } else {
      if (search_pma(el, seg) == pma_seg_it::end(_pma)) {
        return 0;
      } else {
        // scans a tile checking longitude an latitude.
        uint32_t elts = count_if_pma(el, seg, region);
        __apply(el, elts);

        return 1;
      }
    }
  } else {
    return 0;
  }
}

/***
 * Counts the number of elements in the PMA between el.min_code and el.max_code , start search at segment seg
 *
 */
uint32_t PMQ::count_pma(const code_t &el, pma_seg_it &seg) const {

  int32_t count = 0;

  // lower_bound
  auto prev_seg = seg;

  // subtract extra elements for first segment
  count -= std::count_if(pma_offset_it::begin(_pma, prev_seg), pma_offset_it::end(_pma, prev_seg),
                         [&el](void *elt) {
                           uint64_t code = PMA_ELT(elt);
                           return (code < el.min_code);
                         }
  );

  // upper bound
  seg = std::lower_bound(prev_seg, pma_seg_it::end(_pma), el.max_code + 1,
                         [](void *elt, uint64_t value) {
                           return PMA_ELT(elt) < value;
                         });

  while (prev_seg != pma_seg_it::end(_pma) && prev_seg <= seg) {
    count += prev_seg.size();

    // iterate over segments
    ++prev_seg;
  }

  // subtract extra elements for last segment
  count -= std::count_if(pma_offset_it::begin(_pma, seg), pma_offset_it::end(_pma, seg),
                         [&el](void *elt) {
                           uint64_t code = PMA_ELT(elt);
                           return (code > el.max_code);
                         }
  );

  return count;
}

uint32_t PMQ::count_if_pma(const code_t &el, pma_seg_it &seg, const region_t &region) const {

  uint32_t count = 0;

  // lower_bound
  auto prev_seg = seg;

  // upper bound
  seg = std::lower_bound(prev_seg, pma_seg_it::end(_pma), el.max_code + 1,
                         [](void *elt, uint64_t value) {
                           return PMA_ELT(elt) < value;
                         });

  while (prev_seg != pma_seg_it::end(_pma) && prev_seg <= seg) {
    // iterate over offsets
    count += std::count_if(pma_offset_it::begin(_pma, prev_seg), pma_offset_it::end(_pma, prev_seg),
                           [&region, &el](void *elt) {

                             uint64_t code = PMA_ELT(elt);

                             if (code >= el.min_code && code <= el.max_code) {
                               // count how many elements in this tile are inside the region
                               uint32_t x, y;
                               mortonDecode_RAM(code, x, y);

                               return (region.x0 <= x && region.x1 >= x && region.y0 <= y && region.y1 >= y);
                             } else {
                               return false;
                             }
                           }
    );
    // iterate over segments
    ++prev_seg;
  }

  return count;
}

void PMQ::scan_pma(const code_t &el, pma_seg_it &seg, scantype_function _apply) const {
  while (seg < pma_seg_it::end(_pma)) {
    auto it = pma_offset_it::begin(_pma, seg);

    while (it < pma_offset_it::end(_pma, seg)) {
      uint64_t code = PMA_ELT(*it);

      if (code > el.max_code) {
        return;

      } else if (code >= el.min_code) {
        _apply(*(valuetype *) ELT_TO_CONTENT(*it));
      }
      ++it;
    }
    ++seg;
  }
}

void PMQ::scan_if_pma(const code_t &el, pma_seg_it &seg, const region_t &region, scantype_function _apply) const {
  while (seg < pma_seg_it::end(_pma)) {
    auto it = pma_offset_it::begin(_pma, seg);

    while (it < pma_offset_it::end(_pma, seg)) {
      uint64_t code = PMA_ELT(*it);

      if (code > el.max_code) {
        return;

      } else if (code >= el.min_code) {
        uint32_t x, y;
        mortonDecode_RAM(code, x, y);

        if (region.x0 <= x && region.x1 >= x && region.y0 <= y && region.y1 >= y) {
          _apply(*(valuetype *) ELT_TO_CONTENT(*it));
        }
      }
      ++it;
    }
    ++seg;
  }
}

/**
 * @brief GeoHashSequential::search_pma
 * Sequential search on the pma.
 */
pma_seg_it PMQSequential::search_pma(const code_t &el, pma_seg_it &seg) const {
  while (seg < pma_seg_it::end(_pma)) {
    if (PMA_ELT(*seg) >= el.min_code) break;
    ++seg;
  }

  if (seg < pma_seg_it::end(_pma)) return find_elt_pma(el.min_code, el.max_code, seg);

  // not found
  return pma_seg_it::end(_pma);
}

pma_seg_it PMQBinary::search_pma(const code_t &el, pma_seg_it &seg) const {
  // current segment
  if (PMA_ELT(*seg) >= el.min_code) {
    return find_elt_pma(el.min_code, el.max_code, seg);
  }

  seg = std::lower_bound(seg, pma_seg_it::end(_pma), el.min_code,
                         [](void *elt, uint64_t value) {
                           return PMA_ELT(elt) < value;
                         });

  if (seg < pma_seg_it::end(_pma)) return find_elt_pma(el.min_code, el.max_code, seg);

  // not found
  return pma_seg_it::end(_pma);
}
