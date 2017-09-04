#pragma once

#include "GeoCtnIntf.h"
#include "pma_it.h"

class GeoHash : public GeoCtnIntf {
 public:
  GeoHash(int argc, char *argv[], int _refLevel = 8);

  virtual ~GeoHash();

  // build container
  duration_t create(uint32_t size) override;

  // update container
  duration_t insert(std::vector<elttype> batch) override;
  duration_t insert_rm(std::vector<elttype> batch, std::function<int(const void *)> is_removed);

  // apply function for every el<valuetype>
  duration_t scan_at_region(const region_t &region, scantype_function __apply) override;

  // apply function for every spatial area/region
  duration_t apply_at_tile(const region_t &region, applytype_function __apply) override;
  duration_t apply_at_region(const region_t &region, applytype_function __apply) override;

  duration_t topk_search(const region_t &region, topk_t &topk, scantype_function __apply) override;

  inline virtual std::string name() const = 0;

  size_t size() const { return _pma->nb_elements(); }
  size_t capacity() const { return _pma->array_size; }

 protected:
#define PMA_ELT(x) ((*(uint64_t*)x))

  inline pma_seg_it find_elt_pma(const uint64_t code_min, const uint64_t code_max, const pma_seg_it &seg) const;

  virtual pma_seg_it search_pma(const code_t &el, pma_seg_it &seg) const = 0;

  // apply function for every el<valuetype>
  uint32_t scan_pma_at_region(const code_t &el, pma_seg_it &seg, const region_t &region, scantype_function __apply);

  // apply function for every spatial area/region
  uint32_t apply_pma_at_tile(const code_t &el, pma_seg_it &seg, const region_t &region, applytype_function __apply);

  uint32_t apply_pma_at_region(const code_t &el, pma_seg_it &seg, const region_t &region, applytype_function __apply);

  //void topk_pma_search()

  uint32_t count_pma(const code_t &el, pma_seg_it &seg) const;

  uint32_t count_if_pma(const code_t &el, pma_seg_it &seg, const region_t &region) const;

  void scan_pma(const code_t &el, pma_seg_it &seg, scantype_function _apply) const;

  void scan_if_pma(const code_t &el, pma_seg_it &seg, const region_t &region, scantype_function _apply) const;

  inline code_t get_parent_quadrant(const region_t &region) const;

  uint32_t seg_size;
  float tau_0, tau_h, rho_0, rho_h;

  pma_struct *_pma{nullptr};
};

pma_seg_it GeoHash::find_elt_pma(const uint64_t code_min, const uint64_t code_max, const pma_seg_it &seg) const {
  auto it = pma_offset_it::begin(_pma, seg);

  while (it < pma_offset_it::end(_pma, seg)) {
    if (PMA_ELT(*it) > code_max) return pma_seg_it::end(_pma);
    else if (PMA_ELT(*it) >= code_min) return seg;
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
inline code_t GeoHash::get_parent_quadrant(const region_t &region) const {
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

class GeoHashSequential : public GeoHash {
 public:
  GeoHashSequential(int argc, char *argv[]) : GeoHash(argc, argv) {
  };

  virtual ~GeoHashSequential() = default;

  std::string name() const override {
    static auto name_str = "GeoHashSequential";
    return name_str;
  }

 protected:
  pma_seg_it search_pma(const code_t &el, pma_seg_it &seg) const override final;

  friend class TEST_GeoHashSequential; //gives access to private and protected method for testing purposes
};

class GeoHashBinary : public GeoHash {
 public:
  GeoHashBinary(int argc, char *argv[], int refLevel) : GeoHash(argc, argv, refLevel) {
  };

  virtual ~GeoHashBinary() = default;

  std::string name() const override {
    static auto name_str = "GeoHashBinary";
    return name_str;
  }

 protected:
  pma_seg_it search_pma(const code_t &el, pma_seg_it &seg) const override final;

  friend class TEST_GeoHashBinary; //gives access to private and protected method for testing purposes
};
