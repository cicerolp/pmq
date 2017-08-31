#include "stde.h"
#include "GeoHash.h"

GeoHash::GeoHash(int argc, char *argv[]) {
  seg_size = cimg_option("-s", 8, "GeoHash arg: segment size");
  tau_0 = cimg_option("-t0", 0.92f, "GeoHash arg: tau_0");
  tau_h = cimg_option("-th", 0.7f, "GeoHash arg: tau_h");
  rho_0 = cimg_option("-r0", 0.08f, "GeoHash arg: rho_0");
  rho_h = cimg_option("-rh", 0.3f, "GeoHash arg: rho_0");
}

GeoHash::~GeoHash() { if (_pma != nullptr) pma::destroy_pma(_pma); }

duration_t GeoHash::create(uint32_t size) {
  Timer timer;

  timer.start();
  _pma = (struct pma_struct *) pma::build_pma(size, sizeof(valuetype), tau_0, tau_h, rho_0, rho_h, seg_size);
  timer.stop();

  return {duration_info("create", timer)};
}

duration_t GeoHash::insert(std::vector<elttype> batch) {
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

duration_t GeoHash::insert_rm(std::vector<elttype> batch, std::function<int(const void *)> is_removed) {
  Timer timer;

  if (_pma == nullptr) { return {duration_info("Error", timer)}; }

  // insert start
  timer.start();

  std::sort(batch.begin(), batch.end());

  void *begin = (void *) (&batch[0]);
  void *end = (void *) ((char *) (&batch[0]) + (batch.size()) * sizeof(elttype));

  pma::batch::add_rm_array_elts(_pma, begin, end, comp<uint64_t>, is_removed);

  // insert end
  timer.stop();

  return {duration_info("insert_rm", timer)};
}

/**
 * @brief GeoHash::scan_at_region applies the function __apply on the elements inside a region of the implicit quadtree.
 * @param region A square region in the implict quadtree.
 * @param __apply
 * @return
 */
duration_t GeoHash::scan_at_region(const region_t &region, scantype_function __apply) {
  duration_t duration;
  Timer timer;

  if (_pma == nullptr) {
    duration.emplace_back("scan_at_region", 0);
    duration.emplace_back("scan_at_region_refinements", 0);
    return duration;
  }

  timer.start();

  auto curr_seg = pma_seg_it::begin(_pma);
  code_t boundingQuadrant =  get_parent_quadrant(region);
  PRINTOUT("Bounding Quadrant: %llu  %d \n",boundingQuadrant.code,boundingQuadrant.z);
  // Recursive search on the pma
  uint32_t refinements = scan_pma_at_region(boundingQuadrant, curr_seg, region, __apply);

  timer.stop();
  duration.emplace_back("scan_at_region", timer);

  duration.emplace_back("scan_at_region_refinements", refinements);

  return duration;
}

duration_t GeoHash::apply_at_tile(const region_t &region, applytype_function __apply) {
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

duration_t GeoHash::apply_at_region(const region_t &region, applytype_function __apply) {
  duration_t duration;
  Timer timer;

  if (_pma == nullptr) {
    duration.emplace_back("apply_at_region", 0);
    duration.emplace_back("apply_at_region_refinements", 0);
    return duration;
  }

  timer.start();

  auto curr_seg = pma_seg_it::begin(_pma);
  uint32_t refinements = apply_pma_at_region(get_parent_quadrant(region), curr_seg, region, __apply);

  timer.stop();
  duration.emplace_back("apply_at_region", timer);

  duration.emplace_back("apply_at_region_refinements", refinements);

  return duration;
}

duration_t GeoHash::topk_search(const region_t &region, topk_t &topk, scantype_function __apply) {
  Timer timer;

  if (_pma == nullptr) { return {duration_info("topk_search", timer)}; }

  timer.start();

  // store intermediate results
  topk_cnt container(topk);

  // ranking function
  scantype_function fn_topk = std::bind(
      [](const region_t &region, topk_t &topk, topk_cnt &container, const valuetype &el) {

        // temporal ranking
        uint64_t temporal_distance = topk.now - el.time;

        // element is out of region
        if (temporal_distance > topk.time) return;

        // temporal score
        float temporal_score;
        if (topk.time != 0) {
          temporal_score = (float) (temporal_distance / topk.time);
        } else {
          if (temporal_distance == 0) {
            temporal_score = 0.f;
          } else {
            return;
          }
        }

        // spatial ranking
        static const float PI_180_INV = 180.0f / (float) M_PI;
        static const float PI_180 = (float) M_PI / 180.0f;
        static const float r_earth = 6378.f;

        float d_lat = (el.latitude - region.lat) * PI_180;
        float d_lon = (el.longitude - region.lon) * PI_180;

        float a = std::sin(d_lat / 2.0f) * std::sin(d_lat / 2.0f) +
            std::cos(region.lat * PI_180) * std::cos(el.latitude * PI_180) *
                std::sin(d_lon / 2.0f) * std::sin(d_lon / 2);

        float c = 2.0f * std::atan2(std::sqrt(a), std::sqrt(1.0f - a));

        float spatial_distance = (r_earth * c);

        // element is out of time window
        if (spatial_distance > topk.radius) return;

        // spatial score
        float spatial_score = spatial_distance / topk.radius;

        // ranking score
        float score = (topk.alpha * spatial_score) + ((1.f - topk.alpha) * temporal_score);

        container.insert(topk, el, score);

      }, std::ref(region), std::ref(topk), std::ref(container), std::placeholders::_1);

  auto curr_seg = pma_seg_it::begin(_pma);
  scan_pma_at_region(get_parent_quadrant(region), curr_seg, region, fn_topk);

  std::vector<valuetype> output(container.get_output(topk));

  timer.stop();

  for (auto &el : output) {
    __apply(el);
  }

  return {duration_info("topk_search", timer)};
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

uint32_t GeoHash::scan_pma_at_region(const code_t &el,
                                     pma_seg_it &seg,
                                     const region_t &region,
                                     scantype_function __apply) {
  if (seg == pma_seg_it::end(_pma) || PMA_ELT(seg.front()) > el.max_code) return 0;

  /// region.z is allways our max quadree resolution and el.z is depths of the bounding quadrant.
  if (el.z > region.z) return 0;

  region_t::overlap overlap = region.test(el);

  if (overlap == region_t::full) {
    if (search_pma(el, seg) == pma_seg_it::end(_pma)) {
      // this sub-region has no elements in the pma.
      return 0;
    }

    //scans a contiguous region of the PMA
    scan_pma(el, seg, __apply);
    return 1;

  } else if (overlap != region_t::none) {
    uint32_t refinements = 0;

    // break code into four
    uint64_t code = el.code << 2;

#ifndef STATS_REFINE
    refinements += scan_pma_at_region(code_t(code | 0, (uint32_t) (el.z + 1)), seg, region, __apply);
    refinements += scan_pma_at_region(code_t(code | 1, (uint32_t) (el.z + 1)), seg, region, __apply);
    refinements += scan_pma_at_region(code_t(code | 2, (uint32_t) (el.z + 1)), seg, region, __apply);
    refinements += scan_pma_at_region(code_t(code | 3, (uint32_t) (el.z + 1)), seg, region, __apply);
    return refinements;
#else
    scan_pma_at_region(code_t(code | 0, (uint32_t) (el.z + 1)), seg, region, __apply);
    scan_pma_at_region(code_t(code | 1, (uint32_t) (el.z + 1)), seg, region, __apply);
    scan_pma_at_region(code_t(code | 2, (uint32_t) (el.z + 1)), seg, region, __apply);
    scan_pma_at_region(code_t(code | 3, (uint32_t) (el.z + 1)), seg, region, __apply);
    return 0;
#endif
  } else {
    return 0;
  }
}

uint32_t GeoHash::apply_pma_at_tile(const code_t &el,
                                    pma_seg_it &seg,
                                    const region_t &region,
                                    applytype_function __apply) {
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

uint32_t GeoHash::apply_pma_at_region(const code_t &el,
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


     if (el.z < 8) {
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

           uint32_t elts = count_if_pma(el,seg,region);
           __apply(el, elts);

           return 1;
        }
     }
  } else {
    return 0;
  }
}
/*** Counts elements in region
JULIO : INPROGRESS
- Check if the pma_offset_it are beeing used correctly.
*/
uint32_t GeoHash::count_if_pma(const code_t &el, pma_seg_it &seg, const region_t &region ) const {

  assert(region.z == 25);

  pma_offset_it off_begin, off_end;

  //get the first element (upper-left corner) in the tile
  off_begin = std::lower_bound(pma_offset_it::begin(_pma, seg), pma_offset_it::end(_pma, seg), el.min_code,
                                 [](void *elt, uint64_t value) { return PMA_ELT(elt) < value; });

  //get the last element (botoom-right corner) in the tile
  //Julio : why the upper_bound Doesn't work ??

  //off_end = std::upper_bound(pma_offset_it::begin(_pma, seg), pma_offset_it::end(_pma, seg), el.max_code ,
  off_end = std::lower_bound(pma_offset_it::begin(_pma, seg), pma_offset_it::end(_pma, seg), el.max_code ,
                                 [](void *elt, uint64_t value) { return PMA_ELT(elt) < value; });

  //Count how many elements in this tile are inside the region
  uint32_t elts = std::count_if(pma_offset_it::begin(_pma, seg),
                                pma_offset_it::end(_pma, seg),
                                [ &region ](void *elt){
      uint32_t x, y;
      mortonDecode_RAM(PMA_ELT(elt),x,y);

      return ( x <= region.x1 && x >= region.x0 && y <= region.y0 && y >= region.y1 );
      }
  );

}

/***
 * Counts the number of elements in the PMA between el.min_code and el.max_code , start search at segment seg
 *
 */
uint32_t GeoHash::count_pma(const code_t &el, pma_seg_it &seg) const {
  size_t count = 0;
  auto prev_seg = seg;

  count += seg.size();

  //count full segments
  while (++seg != pma_seg_it::end(_pma) && PMA_ELT(*seg) <= el.max_code) {
    count += seg.size();
  }

  assert(count != 0);

  pma_offset_it off_begin, off_end;

  if (seg != pma_seg_it::end(_pma) && PMA_ELT(*pma_offset_it::begin(_pma, seg)) >= el.min_code) {
    count += seg.size();

    //subtract extra elements for last segment
    off_end = pma_offset_it::end(_pma, seg);
    off_begin = std::lower_bound(pma_offset_it::begin(_pma, seg), pma_offset_it::end(_pma, seg), el.max_code + 1,
                                 [](void *elt, uint64_t value) {
                                   return PMA_ELT(elt) < value;
                                 });
    count -= (off_end - off_begin);

    assert(count != 0);
  }

  //subtract extra elements for first segment
  off_begin = pma_offset_it::begin(_pma, prev_seg);
  off_end = std::lower_bound(off_begin, pma_offset_it::end(_pma, prev_seg), el.min_code,
                             [](void *elt, uint64_t value) {
                               return PMA_ELT(elt) < value;
                             });
  count -= (off_end - off_begin);

  assert(count != 0);

  return (uint32_t) count;
}

void GeoHash::scan_pma(const code_t &el, pma_seg_it &seg, scantype_function _apply) const {
  while (seg < pma_seg_it::end(_pma)) {

    auto it = pma_offset_it::begin(_pma, seg);

    while (it < pma_offset_it::end(_pma, seg)) {
      if (PMA_ELT(*it) > el.max_code) {
        return;
      } else if (PMA_ELT(*it) >= el.min_code) {
        _apply(*(valuetype *) ELT_TO_CONTENT(*it));
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
pma_seg_it GeoHashSequential::search_pma(const code_t &el, pma_seg_it &seg) const {
  while (seg < pma_seg_it::end(_pma)) {
    if (PMA_ELT(*seg) >= el.min_code) break;
    ++seg;
  }

  if (seg < pma_seg_it::end(_pma)) return find_elt_pma(el.min_code, el.max_code, seg);

  // not found
  return pma_seg_it::end(_pma);
}

pma_seg_it GeoHashBinary::search_pma(const code_t &el, pma_seg_it &seg) const {
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
