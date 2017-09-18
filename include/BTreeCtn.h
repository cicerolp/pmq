//
// Created by cicerolp on 8/20/17.
//

#pragma once

#include "GeoCtnIntf.h"

class BTreeCtn : public GeoCtnIntf {
 public:
  BTreeCtn(int argc, char *argv[], int _refLevel = 8) : GeoCtnIntf(_refLevel) {}
  virtual ~BTreeCtn();

  duration_t create(uint32_t size) override;

  duration_t insert(std::vector<elttype> batch) override;

  duration_t insert_rm(std::vector<elttype> batch, std::function<int(const void *)> is_removed) override;

  duration_t scan_at_region(const region_t &region, scantype_function __apply) override;
  uint32_t scan_btree_at_region(const code_t &el, const region_t &region, scantype_function __apply);

  duration_t apply_at_region(const region_t &region, applytype_function __apply) override;
  uint32_t apply_btree_at_region(const code_t &el, const region_t &region, applytype_function __apply);

  size_t size() const override {
    return _btree->size();
  }

  std::string name() const override {
    static auto name_str = "BTree";
    return name_str;
  }

  inline code_t get_parent_quadrant(const region_t &region) const {
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

 protected:
  typedef uint64_t key;
  typedef valuetype data;

  uint32_t _size;
  std::unique_ptr<stx::btree_multimap<key, data>> _btree;
};
