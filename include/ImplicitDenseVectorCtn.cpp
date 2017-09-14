//
// Created by cicerolp on 9/14/17.
//

#include "ImplicitDenseVectorCtn.h"

ImplicitDenseVectorCtn::ImplicitDenseVectorCtn(int argc, char *argv[]) {

}

duration_t ImplicitDenseVectorCtn::create(uint32_t size) {
  Timer timer;

  timer.start();
  _container.reserve(size);
  timer.stop();

  return {duration_info("create", timer)};
}
duration_t ImplicitDenseVectorCtn::insert(std::vector<elttype> batch) {
  duration_t duration;
  Timer timer;

  // insert start
  timer.start();
  _container.insert(_container.end(), batch.begin(), batch.end());
  // insert end
  timer.stop();
  duration.emplace_back("insert", timer);

  // sorting start
  timer.start();
  gfx::timsort(_container.begin(), _container.end());
  // sorting end
  timer.stop();
  duration.emplace_back("sorting", timer);

  return duration;
}
duration_t ImplicitDenseVectorCtn::insert_rm(std::vector<elttype> batch, std::function<int(const void *)> is_removed) {
  return GeoCtnIntf::insert_rm(batch, is_removed);
}
duration_t ImplicitDenseVectorCtn::scan_at_region(const region_t &region, scantype_function __apply) {
  duration_t duration;
  Timer timer;

  timer.start();

  // recursive search on the pma
  auto curr_seg = _container.begin();
  uint32_t refinements = scan_ctn_at_region(get_parent_quadrant(region), curr_seg, region, __apply);

  timer.stop();
  duration.emplace_back("scan_at_region", timer);

  duration.emplace_back("scan_at_region_refinements", refinements);

  return duration;
}
uint32_t ImplicitDenseVectorCtn::scan_ctn_at_region(const code_t &el, ctn_t::iterator &it,
                                                    const region_t &region, scantype_function __apply) {

  if (el.z > region.z) return 0;

  region_t::overlap overlap = region.test(el);

  if (overlap == region_t::full) {
    if (search_ctn(el, it) == _container.end()) {
      return 0;
    } else {
      //scans a contiguous region of the PMA
      //scan_pma(el, seg, __apply);

      while (it < _container.end()) {
        uint64_t code = (*it).key;

        if (code > el.max_code) {
          break;

        } else if (code >= el.min_code) {
          __apply((*it).value);
        }
        ++it;
      }

      return 1;
    }
  } else if (overlap == region_t::partial) {

    if (el.z < refLevel) {
      //Keep doing recursive refinements
      uint32_t refinements = 0;

      // break morton code into four
      uint64_t code = el.code << 2;

      refinements += scan_ctn_at_region(code_t(code | 0, (uint32_t) (el.z + 1)), it, region, __apply);
      refinements += scan_ctn_at_region(code_t(code | 1, (uint32_t) (el.z + 1)), it, region, __apply);
      refinements += scan_ctn_at_region(code_t(code | 2, (uint32_t) (el.z + 1)), it, region, __apply);
      refinements += scan_ctn_at_region(code_t(code | 3, (uint32_t) (el.z + 1)), it, region, __apply);

      return refinements;

    } else {
      if (search_ctn(el, it) == _container.end()) {
        return 0;
      } else {
        // scans a tile checking longitude an latitude.
        //scan_if_pma(el, seg, region, __apply);

        while (it < _container.end()) {
          uint64_t code = (*it).key;

          if (code > el.max_code) {
            break;

          } else if (code >= el.min_code) {
            uint32_t x, y;
            mortonDecode_RAM(code, x, y);

            if (region.x0 <= x && region.x1 >= x && region.y0 <= y && region.y1 >= y) {
              __apply((*it).value);
            }
          }
          ++it;
        }

        return 1;
      }
    }
  } else {
    return 0;
  }

  return 0;
}
duration_t ImplicitDenseVectorCtn::apply_at_tile(const region_t &region, applytype_function __apply) {
  duration_t duration;
  Timer timer;

  timer.start();
  timer.stop();
  duration.emplace_back("wip", timer);

  return duration;
}
duration_t ImplicitDenseVectorCtn::apply_at_region(const region_t &region, applytype_function __apply) {
  duration_t duration;
  Timer timer;

  timer.start();

  // recursive search on the pma
  auto curr_seg = _container.begin();
  uint32_t refinements = apply_ctn_at_region(get_parent_quadrant(region), curr_seg, region, __apply);

  timer.stop();
  duration.emplace_back("apply_at_region", timer);

  duration.emplace_back("apply_at_region_refinements", refinements);

  return duration;
}

uint32_t ImplicitDenseVectorCtn::apply_ctn_at_region(const code_t &el, ctn_t::iterator &it,
                                                     const region_t &region, applytype_function __apply) {
  if (el.z > region.z) return 0;

  region_t::overlap overlap = region.test(el);

  if (overlap == region_t::full) {
    if (search_ctn(el, it) == _container.end()) {
      return 0;
    } else {
      //__apply(el, count_pma(el, seg));
      uint32_t elts = 0;

      auto upper_it = std::lower_bound(it, _container.end(), el.max_code + 1,
                                       [](const elttype &elt, uint64_t value) {
                                         return elt.key < value;
                                       });
      elts = upper_it - it;
      __apply(el, elts);

      it = upper_it;

      return 1;
    }
  } else if (overlap == region_t::partial) {

    if (el.z < refLevel) {
      //Keep doing recursive refinements
      uint32_t refinements = 0;

      // break morton code into four
      uint64_t code = el.code << 2;

      refinements += apply_ctn_at_region(code_t(code | 0, (uint32_t) (el.z + 1)), it, region, __apply);
      refinements += apply_ctn_at_region(code_t(code | 1, (uint32_t) (el.z + 1)), it, region, __apply);
      refinements += apply_ctn_at_region(code_t(code | 2, (uint32_t) (el.z + 1)), it, region, __apply);
      refinements += apply_ctn_at_region(code_t(code | 3, (uint32_t) (el.z + 1)), it, region, __apply);

      return refinements;

    } else {
      if (search_ctn(el, it) == _container.end()) {
        return 0;
      } else {
        // scans a tile checking longitude an latitude.
        //uint32_t elts = count_if_pma(el, seg, region);
        uint32_t elts = 0;

        auto upper_it = std::lower_bound(it, _container.end(), el.max_code + 1,
                                         [](const elttype &elt, uint64_t value) {
                                           return elt.key < value;
                                         });

        elts += std::count_if(it, upper_it,
                              [&region, &el](const elttype &elt) {

                                uint64_t code = elt.key;

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

        __apply(el, elts);
        it = upper_it;

        return 1;
      }
    }
  } else {
    return 0;
  }
}

ImplicitDenseVectorCtn::ctn_t::iterator ImplicitDenseVectorCtn::search_ctn(const code_t &el,
                                                                           ImplicitDenseVectorCtn::ctn_t::iterator &it) {
  it = std::lower_bound(it, _container.end(), el.min_code, [](const elttype &elt, uint64_t value) {
    return elt.key < value;
  });

  return it;
}