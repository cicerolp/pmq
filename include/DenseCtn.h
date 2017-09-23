#pragma once

#include "GeoCtnIntf.h"

template<typename T>
class DenseCtn : public GeoCtnIntf<T> {
 public:
  // function that access a reference to the element in the container
  using scantype_function = typename GeoCtnIntf<T>::scantype_function;
  // function that counts elements in spatial areas
  using applytype_function = typename GeoCtnIntf<T>::applytype_function;

  using elt_type = std::pair<uint64_t, T>;
  using ctn_t = std::vector<elt_type>;
  using ctn_it = typename std::vector<elt_type>::iterator;

  DenseCtn(int argc, char *argv[], int refLevel = 8) : GeoCtnIntf<T>(refLevel) {};

  virtual ~DenseCtn() = default;

  // build container
  duration_t create(uint32_t size) override {
    Timer timer;

    timer.start();
    _size = size;
    _container.reserve(size);
    timer.stop();

    return {duration_info("create", timer)};
  }

  // update container
  duration_t insert(std::vector<T> batch) override {
    duration_t duration;
    Timer timer;

    // insert start
    timer.start();
    std::vector<elt_type> converted_batch = transform(batch);
    _container.insert(_container.end(), converted_batch.begin(), converted_batch.end());
    // insert end
    timer.stop();
    duration.emplace_back("insert", timer);

    // sorting start
    timer.start();
    gfx::timsort(_container.begin(), _container.end(),
                 [](const elt_type &lhs, const elt_type &rhs) {
                   return lhs.first < rhs.first;
                 });
    // sorting end
    timer.stop();
    duration.emplace_back("sorting", timer);

    return duration;
  }
  // Only for PMQ
  duration_t insert_rm(std::vector<T> batch, std::function<int(const void *)> is_removed) override {
    duration_t duration;
    Timer timer;

    // remove before reallocation
    if ((_container.size() + batch.size()) > _size) {
      uint32_t removed_elts = 0;

      // tagging start
      timer.start();
      for (auto &elt: _container) {
        if (is_removed(&(elt))) {
          elt.first = ~0;
          ++removed_elts;
        }
      }
      // tagging end
      timer.stop();
      duration.emplace_back("tagging", timer);


      // remove start
      timer.start();
      gfx::timsort(_container.begin(), _container.end(),
                   [](const elt_type &lhs, const elt_type &rhs) {
                     return lhs.first < rhs.first;
                   });
      _container.resize(_container.size() - removed_elts);
      // remove end
      timer.stop();
      duration.emplace_back("remove", timer);
    }

    // insert start
    timer.start();
    std::vector<elt_type> converted_batch = transform(batch);
    _container.insert(_container.end(), converted_batch.begin(), converted_batch.end());
    // insert end
    timer.stop();
    duration.emplace_back("insert", timer);

    // sorting start
    timer.start();
    gfx::timsort(_container.begin(), _container.end(),
                 [](const elt_type &lhs, const elt_type &rhs) {
                   return lhs.first < rhs.first;
                 });
    // sorting end
    timer.stop();
    duration.emplace_back("sorting", timer);

    return duration;
  }

  duration_t scan_at_region(const region_t &region, scantype_function __apply) override {
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

  duration_t apply_at_region(const region_t &region, applytype_function __apply) override {
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

  inline size_t size() const { return _container.size(); }

  inline std::string name() const;

 protected:
  inline code_t get_parent_quadrant(const region_t &region) const;

  uint32_t scan_ctn_at_region(const code_t &el, ctn_it &it,
                              const region_t &region, scantype_function __apply) {

    if (el.z > region.z) return 0;

    region_t::overlap overlap = region.test(el);

    if (overlap == region_t::full) {
      if (this->search_ctn(el, it) == _container.end()) {
        return 0;
      } else {
        //scans a contiguous region of the PMA
        //scan_pma(el, seg, __apply);

        while (it < _container.end()) {
          uint64_t code = (*it).first;

          if (code > el.max_code) {
            break;

          } else if (code >= el.min_code) {
            __apply((*it).second);
          }
          ++it;
        }

        return 1;
      }
    } else if (overlap == region_t::partial) {

      if (el.z < this->getRefLevel()) {
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
        if (this->search_ctn(el, it) == _container.end()) {
          return 0;
        } else {
          // scans a tile checking longitude an latitude.
          //scan_if_pma(el, seg, region, __apply);

          while (it < _container.end()) {
            uint64_t code = (*it).first;

            if (code > el.max_code) {
              break;

            } else if (code >= el.min_code) {
              uint32_t x, y;
              mortonDecode_RAM(code, x, y);

              if (region.x0 <= x && region.x1 >= x && region.y0 <= y && region.y1 >= y) {
                __apply((*it).second);
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

  uint32_t apply_ctn_at_region(const code_t &el, ctn_it &it,
                               const region_t &region, applytype_function __apply) {
    if (el.z > region.z) return 0;

    region_t::overlap overlap = region.test(el);

    if (overlap == region_t::full) {
      if (this->search_ctn(el, it) == _container.end()) {
        return 0;
      } else {
        //__apply(el, count_pma(el, seg));
        uint32_t elts = 0;

        auto upper_it = std::lower_bound(it, _container.end(), el.max_code + 1,
                                         [](const elt_type &elt, uint64_t value) {
                                           return elt.first < value;
                                         });
        elts = upper_it - it;
        __apply(el, elts);

        it = upper_it;

        return 1;
      }
    } else if (overlap == region_t::partial) {

      if (el.z < this->getRefLevel()) {
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
        if (this->search_ctn(el, it) == _container.end()) {
          return 0;
        } else {
          // scans a tile checking longitude an latitude.
          //uint32_t elts = count_if_pma(el, seg, region);
          uint32_t elts = 0;

          auto upper_it = std::lower_bound(it, _container.end(), el.max_code + 1,
                                           [](const elt_type &elt, uint64_t value) {
                                             return elt.first < value;
                                           });

          elts += std::count_if(it, upper_it,
                                [&region, &el](const elt_type &elt) {

                                  uint64_t code = elt.first;

                                  if (code >= el.min_code && code <= el.max_code) {
                                    // count how many elements in this tile are inside the region
                                    uint32_t x = 0, y = 0;
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

  typename ctn_t::iterator search_ctn(const code_t &el, ctn_it &it) {
    it = std::lower_bound(it, _container.end(), el.min_code,
                          [](const elt_type &elt, uint64_t value) {
                            return elt.first < value;
                          });

    return it;
  }

  std::vector<elt_type> transform(std::vector<T> batch) const {
    // convert T to value_type
    std::vector<elt_type> converted_batch;
    std::transform(batch.begin(), batch.end(), std::back_inserter(converted_batch),
                   [](const auto &value) -> std::pair<uint64_t, T> {
                     uint32_t y = mercator_util::lat2tiley(value.getLatitude(), 25);
                     uint32_t x = mercator_util::lon2tilex(value.getLongitude(), 25);
                     return std::make_pair(mortonEncode_RAM(x, y), value);
                   });

    return converted_batch;
  }

  ctn_t _container;
  size_t _size;
};

template<typename T>
inline code_t DenseCtn<T>::get_parent_quadrant(const region_t &region) const {
  uint64_t mask = region.code0 ^region.code1;

  mask |= mask >> 32;
  mask |= mask >> 16;
  mask |= mask >> 8;
  mask |= mask >> 4;
  mask |= mask >> 2;
  mask |= mask >> 1;

  mask = (~mask);

  uint64_t prefix = mask & region.code1;

  uint32_t depth_diff = 0;
  while ((mask & 3) != 3) {
    depth_diff++;
    mask = mask >> 2;
  }

  prefix = prefix >> (depth_diff * 2);

  return code_t(prefix, region.z - depth_diff);
}

template<typename T>
inline std::string DenseCtn<T>::name() const {
  static auto name_str = "ImplicitDenseVector";
  return name_str;
}

