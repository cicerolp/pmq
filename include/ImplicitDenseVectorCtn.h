#pragma once

#include "GeoCtnIntf.h"

class ImplicitDenseVectorCtn : public GeoCtnIntf {
 public:
  using ctn_t = std::vector<elttype>;

  ImplicitDenseVectorCtn(int argc, char *argv[]);

  virtual ~ImplicitDenseVectorCtn() = default;

  // build container
  duration_t create(uint32_t size) override;

  // update container
  duration_t insert(std::vector<elttype> batch) override;
  // Only for GeoHash
  duration_t insert_rm(std::vector<elttype> batch, std::function<int(const void *)> is_removed) override;

  duration_t scan_at_region(const region_t &region, scantype_function __apply) override;

  duration_t apply_at_tile(const region_t &region, applytype_function __apply) override;

  duration_t apply_at_region(const region_t &region, applytype_function __apply) override;

  inline size_t size() const { return _container.size(); }

  inline std::string name() const;

 protected:
  inline code_t get_parent_quadrant(const region_t &region) const;

  uint32_t scan_ctn_at_region(const code_t &el, ctn_t::iterator &it,
                              const region_t &region, scantype_function __apply);

  uint32_t apply_ctn_at_region(const code_t &el, ctn_t::iterator &it,
                               const region_t &region, applytype_function __apply);

  ctn_t::iterator search_ctn(const code_t &el, ctn_t::iterator &it);

  ctn_t _container;
  size_t _size;
};

inline code_t ImplicitDenseVectorCtn::get_parent_quadrant(const region_t &region) const {
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

inline std::string ImplicitDenseVectorCtn::name() const {
  static auto name_str = "ImplicitDenseVector";
  return name_str;
}

