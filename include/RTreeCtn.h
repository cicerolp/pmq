//
// Created by cicerolp on 8/13/17.
//

#pragma once

#include "GeoCtnIntf.h"

class RTreeCtn : public GeoCtnIntf {
 public:
  RTreeCtn(int argc, char *argv[]);
  virtual ~RTreeCtn() = default;

  // build container
  duration_t create(uint32_t size) override;

  // update container
  duration_t insert(std::vector<elttype> batch) override;

  // apply function for every el<valuetype>
  duration_t scan_at_region(const region_t &region, scantype_function __apply) override;

  // apply function for every spatial area/region
  duration_t apply_at_tile(const region_t &region, applytype_function __apply) override;
  duration_t apply_at_region(const region_t &region, applytype_function __apply) override;

  duration_t topk_search(const region_t &region, topk_t &topk, scantype_function __apply) override;

  inline std::string name() const override {
    static auto name_str = "RTree";
    return name_str;
  }

};
