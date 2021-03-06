#pragma once

#include "GeoCtnIntf.h"
#include "pma_it.h"
#include "GenericType.h"

template<typename T>
class PMQ : public GeoCtnIntf<T> {
 public:
  // function that access a reference to the element in the container
  using scantype_function = typename GeoCtnIntf<T>::scantype_function;
  // function that counts elements in spatial areas
  using applytype_function = typename GeoCtnIntf<T>::applytype_function;
  using value_type = std::pair<uint64_t, T>;

  PMQ(int argc, char *argv[], int refLevel = 8) : GeoCtnIntf<T>(refLevel) {
    seg_size = cimg_option("-s", 8, "PMQ arg: segment size");
    tau_0 = cimg_option("-t0", 0.92f, "PMQ arg: tau_0");
    tau_h = cimg_option("-th", 0.7f, "PMQ arg: tau_h");
    rho_0 = cimg_option("-r0", 0.08f, "PMQ arg: rho_0");
    rho_h = cimg_option("-rh", 0.3f, "PMQ arg: rho_0");
  }

  virtual ~PMQ() {
    if (_pma != nullptr) pma::destroy_pma(_pma);
  }

  // build container
  duration_t create(uint32_t size) override {
    Timer timer;

    timer.start();
    _pma = (struct pma_struct *) pma::build_pma(size, sizeof(T), tau_0, tau_h, rho_0, rho_h, seg_size);
    timer.stop();

    return {duration_info("create", timer)};
  }

  // update container
  duration_t insert(std::vector<T> batch) override {
    Timer timer;

    if (_pma == nullptr) {
      return {duration_info("error", timer)};
    }

    // insert start
    timer.start();

    std::vector<value_type> converted_batch = transform(batch);

    void *begin = (void *) (&converted_batch[0]);
    void *end = (void *) ((char *) (&converted_batch[0]) + (converted_batch.size()) * sizeof(value_type));

    pma::batch::add_array_elts(_pma, begin, end, comp<uint64_t>);

    //insert end
    timer.stop();
    return {duration_info("insert", timer)};
  }

  duration_t insert_rm(std::vector<T> batch, std::function<int(const void *)> pred) {
    duration_t duration;
    Timer timer;

    if (_pma == nullptr) {
      return {duration_info("error", timer)};
    }

    // insert start
    timer.start();

    std::vector<value_type> converted_batch = transform(batch);

    void *begin = (void *) (&converted_batch[0]);
    void *end = (void *) ((char *) (&converted_batch[0]) + (converted_batch.size()) * sizeof(value_type));

    int rm_count = pma::batch::add_rm_array_elts(_pma, begin, end, comp<uint64_t>, pred);
    // insert end
    timer.stop();
    duration.emplace_back("insert", timer);

    if (rm_count > 0) {
      duration.emplace_back("remove", timer);
    }

    return duration;
  }

  duration_t scan_at_region(const region_t &region, scantype_function __apply) override {
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
    uint32_t false_positives = 0, true_positives = 0;
    uint32_t refinements = scan_pma_at_region(get_parent_quadrant(region), curr_seg,
                                              region, __apply, false_positives, true_positives);

    timer.stop();
    duration.emplace_back("scan_at_region", timer);

    duration.emplace_back("scan_at_region_refinements", refinements);

    duration.emplace_back("scan_at_region_true_positives", true_positives);
    duration.emplace_back("scan_at_region_false_positives", false_positives);
    duration.emplace_back("scan_at_region_true_and_false", true_positives + false_positives);

    return duration;
  }

  // apply function for every spatial area/region
  duration_t apply_at_tile(const region_t &region, applytype_function __apply) override {
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

  duration_t apply_at_region(const region_t &region, applytype_function __apply) override {
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

  inline virtual std::string name() const = 0;

  size_t size() const { return _pma->nb_elements(); }
  size_t capacity() const { return _pma->array_size; }

 protected:
  //Returns the PMA key pointed by x
#define PMA_ELT(x) ((*(uint64_t*)x))

  inline pma_seg_it find_elt_pma(const uint64_t code_min, const uint64_t code_max, const pma_seg_it &seg) const;

  virtual pma_seg_it search_pma(const code_t &el, pma_seg_it &seg) const = 0;

  // apply function for every el<valuetype>
  /**
   * @brief scan_pma_at_region
   * Recursively breaks down the quadrant @param el.
   *
   * @param el
   * @param seg
   * @param region
   * @param __apply
   * @param false_positives
   * @param true_positives
   * @return
   */
  uint32_t scan_pma_at_region(const code_t &el, pma_seg_it &seg, const region_t &region,
                              scantype_function __apply, uint32_t &false_positives, uint32_t &true_positives) {

    //Check if the current segment has its first element greater than the max possibel key ot the current quandrant el.
    if (seg == pma_seg_it::end(_pma) || PMA_ELT(seg.front()) > el.max_code) return 0; // return if so.

    if (el.z > region.z) return 0;

    region_t::overlap overlap = region.test(el);

    if (overlap == region_t::full) {
      if (search_pma(el, seg) == pma_seg_it::end(_pma)) { // WARNING: advances seg
          //the quadrant el is empty.
        return 0;
      } else {
        //scans a contiguous region of the PMA
        scan_pma(el, seg, __apply); // WARNING : advances seg
        return 1;
      }
    } else if (overlap == region_t::partial) {

      if (el.z < this->getRefLevel()) {
        //Keep doing recursive refinements
        uint32_t refinements = 0;

        // break morton code into four
        uint64_t code = el.code << 2;

        // Warning the order of this calls is important because each function may modify the seg pointer.
        refinements +=
            scan_pma_at_region(code_t(code | 0, (uint32_t) (el.z + 1)),
                               seg, region, __apply,
                               false_positives,
                               true_positives);
        refinements +=
            scan_pma_at_region(code_t(code | 1, (uint32_t) (el.z + 1)),
                               seg, region, __apply,
                               false_positives,
                               true_positives);
        refinements +=
            scan_pma_at_region(code_t(code | 2, (uint32_t) (el.z + 1)),
                               seg, region, __apply,
                               false_positives,
                               true_positives);
        refinements +=
            scan_pma_at_region(code_t(code | 3, (uint32_t) (el.z + 1)),
                               seg, region, __apply,
                               false_positives,
                               true_positives);

        return refinements;

      } else {
        if (search_pma(el, seg) == pma_seg_it::end(_pma)) {
          return 0;
        } else {
          // scans a tile checking longitude an latitude.
          scan_if_pma(el, seg, region, __apply, false_positives, true_positives); // WARNING: Advances seg
          return 1;
        }
      }
    } else {
      return 0;
    }
  }

  // apply function for every spatial area/region
  uint32_t apply_pma_at_tile(const code_t &el, pma_seg_it &seg, const region_t &region, applytype_function __apply) {
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

  uint32_t apply_pma_at_region(const code_t &el, pma_seg_it &seg, const region_t &region, applytype_function __apply) {
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

      if (el.z < this->getRefLevel()) {
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

  //void topk_pma_search()

  uint32_t count_pma(const code_t &el, pma_seg_it &seg) const {

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

  uint32_t count_if_pma(const code_t &el, pma_seg_it &seg, const region_t &region) const {

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
                                 uint32_t x = 0, y = 0;
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

  void scan_pma(const code_t &el, pma_seg_it &seg, scantype_function _apply) const {
    while (seg < pma_seg_it::end(_pma)) {
      auto it = pma_offset_it::begin(_pma, seg);

      while (it < pma_offset_it::end(_pma, seg)) {
        uint64_t code = PMA_ELT(*it);

        if (code > el.max_code) {
          return;

        } else if (code >= el.min_code) {
          _apply(*(T *) ELT_TO_CONTENT(*it));
        }
        ++it;
      }
      ++seg;
    }
  }

  void scan_if_pma(const code_t &el, pma_seg_it &seg, const region_t &region,
                   scantype_function _apply, uint32_t &false_positives, uint32_t &true_positives) const {
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
            _apply(*(T *) ELT_TO_CONTENT(*it));
            ++true_positives;
          } else {
            ++false_positives;
          }
        }
        ++it;
      }
      ++seg;
    }
  }

  std::vector<value_type> transform(std::vector<T> batch) const {
    // convert T to value_type
    std::vector<value_type> converted_batch;
    converted_batch.reserve(batch.size());

    std::transform(batch.begin(), batch.end(), std::back_inserter(converted_batch),
                   [](const auto &value) -> std::pair<uint64_t, T> {
                     uint32_t y = mercator_util::lat2tiley(value.getLatitude(), 25);
                     uint32_t x = mercator_util::lon2tilex(value.getLongitude(), 25);
                     return std::make_pair(mortonEncode_RAM(x, y), value);
                   });

    gfx::timsort(converted_batch.begin(), converted_batch.end(),
                 [](const value_type &lhs, const value_type &rhs) {
                   return lhs.first < rhs.first;
                 });

    return converted_batch;
  }

  inline code_t get_parent_quadrant(const region_t &region) const;

  uint32_t seg_size;
  float tau_0, tau_h, rho_0, rho_h;

  pma_struct *_pma{nullptr};
};

/**
 * @brief PMQ<T>::find_elt_pma
 * @param code_min
 * @param code_max
 * @param seg
 *
 * Searches inside a single segment \a seg.
 * Checks if there is Key with values between \a code_min and \a code_max in segment \a seg.
 *
 * @return
 * If YES -> returns the same \a seg from input without modification
 * If NOT -> returns the last segment from the PMA.
 */
template<typename T>
pma_seg_it PMQ<T>::find_elt_pma(const uint64_t code_min, const uint64_t code_max, const pma_seg_it &seg) const {
  auto it = pma_offset_it::begin(_pma, seg);

  while (it < pma_offset_it::end(_pma, seg)) {
    if (PMA_ELT(*it) > code_max)
         return pma_seg_it::end(_pma); //returns last segment of the PMA
    else if (PMA_ELT(*it) >= code_min)
        return seg; // returns same segment of input
    ++it;
  }

  return pma_seg_it::end(_pma);

  /*auto begin = pma_offset_it::begin(_pma, seg);
  auto end = pma_offset_it::end(_pma, seg);

  auto it = std::lower_bound(begin, end, code_min,
                          [](void* elt, uint64_t value) {
                             return PMA_ELT(elt) < value;
                          });

  if (it == end) return pma_seg_it::end(_pma);
  else if (PMA_ELT(*it) <= code_max) return seg;
  else return pma_seg_it::end(_pma);*/
}

/**
 * @brief GeoHash::get_parent_quadrant
 * @param region : a square region represented by upper left , lower right points (motron codes)
 * @return A code_t structure ( with the correct morton code ) that completley covers the given region.
 *
 * Example between two morton codes of 4 bits , 1110 and 1101 , returns a two-bit motronCode 11.
 *
 */
template<typename T>
inline code_t PMQ<T>::get_parent_quadrant(const region_t &region) const {
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
class PMQSequential : public PMQ<T> {
 public:
  PMQSequential(int argc, char *argv[]) : PMQ<T>(argc, argv) {
  };

  virtual ~PMQSequential() = default;

  std::string name() const override {
    static auto name_str = "GeoHashSequential";
    return name_str;
  }

 protected:
  pma_seg_it search_pma(const code_t &el, pma_seg_it &seg) const override final {
    while (seg < pma_seg_it::end(this->_pma)) {
      if (PMA_ELT(*seg) >= el.min_code) break;
      ++seg;
    }

    if (seg < pma_seg_it::end(this->_pma)) return PMQ<T>::find_elt_pma(el.min_code, el.max_code, seg);

    // not found
    return pma_seg_it::end(this->_pma);
  }

  template<typename _T>
  friend
  class TEST_PMQSequential; //gives access to private and protected method for testing purposes
};

template<typename T>
class PMQBinary : public PMQ<T> {
 public:
  PMQBinary(int argc, char *argv[], int refLevel = 8) : PMQ<T>(argc, argv, refLevel) {
  };

  virtual ~PMQBinary() = default;

  std::string name() const override {
    static auto name_str = "GeoHashBinary";
    return name_str;
  }

 protected:
  /**
   * @brief Searches in the PMA if there is an element from quandrant \a el.
   * Starts searching from segment \a seg
   * @param el A geohash quadrant
   * @param seg The segement to start the search
   * @return
   * If an element is found , return the segment where is was found.
   * If an element if not found, returns the end iterator for the PMA
   *
   * The parameter \a seg advances to the matching segment
   */
  pma_seg_it search_pma(const code_t &el, pma_seg_it &seg) const override final {
    // searches the current segment
    if (PMA_ELT(*seg) >= el.min_code) { // if first element of this segment is larger that the minimum possible index of this quadrant
        return PMQ<T>::find_elt_pma(el.min_code, el.max_code, seg); // checks if there is a key in seg inside quadrant el
    }

    //finds the fisrt segment starting by "seg" containing a key not less than el's minumm possible index
    seg = std::lower_bound(seg, pma_seg_it::end(this->_pma), el.min_code,
                           [](void *elt, uint64_t value) {
                             return PMA_ELT(elt) < value; //return the pointer to the segment where this condition doesn't hold
                           });

   // is a segment was found, searches the first element inside el
    if (seg < pma_seg_it::end(this->_pma)) return PMQ<T>::find_elt_pma(el.min_code, el.max_code, seg);

    // not found
    return pma_seg_it::end(this->_pma);
  }

  template<typename _T>
  friend
  class TEST_PMQBinary; //gives access to private and protected method for testing purposes
};
