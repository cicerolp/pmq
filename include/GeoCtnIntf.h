#pragma once

#include "types.h"

// Function that access a reference to the element in the container
using scantype_function = std::function<void(const valuetype &)>;

// Function that counts elements in spatial areas.
using applytype_function = std::function<void(const spatial_t & /*spatial area*/, uint32_t /*area count*/)>;

class GeoCtnIntf {
 public:
  GeoCtnIntf(int _refLevel = 8):refLevel(_refLevel){}

  virtual ~GeoCtnIntf() = default;

  // build container
  virtual duration_t create(uint32_t size) = 0;

  // update container
  virtual duration_t insert(std::vector<elttype> batch) = 0;
  // Only for GeoHash
  virtual duration_t insert_rm(std::vector<elttype> batch, std::function<int(const void *)> is_removed) {
    return duration_t();
  }

  /** @brief Applies a scantype_function on the elements contained on a selection region .
   *
   *  Finds all the elements contained in the region and run function __apply with their values as parameter.
   */
  virtual duration_t scan_at_region(const region_t &region, scantype_function __apply) = 0;

  /** @brief Uses the applytype function to count elements on tiles (spatial_t)
   *
   * This function decomposed the selected 'region' using a grid of 2^8 tiles (256 X 256)
   */
  virtual duration_t apply_at_tile(const region_t &region, applytype_function __apply) = 0;

  /** @brief Uses the applytype function to count elements on tiles (spatial_t)
   *
   *  This function decomposes the region into the coarset possible tiles that represent the region.
   */
  virtual duration_t apply_at_region(const region_t &region, applytype_function __apply) = 0;

  virtual duration_t topk_search(const region_t &region, topk_t &topk, scantype_function __apply) {
    return duration_t();
  };

  inline virtual size_t size() const { return 0; }

  inline virtual std::string name() const;

 protected:
  const int refLevel; // Max number of levels to refine searches in the GeoHash


};

std::string GeoCtnIntf::name() const {
  static auto name_str = "Unknown";
  return name_str;
}
