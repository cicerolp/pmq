#pragma once

#include "types.h"

using scantype_function = std::function<void(const valuetype&)>;
using applytype_function = std::function<void(const spatial_t& /*spatial area*/, uint32_t /*area count*/)>;

class GeoCtnIntf {
public:
   GeoCtnIntf() = default;

   virtual ~GeoCtnIntf() = default;

   // build container
   virtual duration_t create(uint32_t size) = 0;

   // update container
   virtual duration_t insert(std::vector<elttype> batch) = 0;
//   virtual duration_t insert_rm(std::vector<elttype> batch, std::function< int (const void*) > is_removed );

   // apply function for every el<valuetype>
   virtual duration_t scan_at_region(const region_t& region, scantype_function __apply) = 0;

   // apply function for every spatial area/region
   virtual duration_t apply_at_tile(const region_t& region, applytype_function __apply) = 0;

   virtual duration_t apply_at_region(const region_t& region, applytype_function __apply) = 0;

   virtual duration_t topk_search(const region_t& region, const topk_t& topk, std::vector<valuetype>& output) {
      return duration_t();
   };

   inline virtual std::string name() const;
};

std::string GeoCtnIntf::name() const {
   static auto name_str = "Unknown";
   return name_str;
}
