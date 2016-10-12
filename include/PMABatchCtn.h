#pragma once

#include "GeoCtnIntf.h"
#include "QuadtreeIntf.h"

class PMABatchCtn : public GeoCtnIntf {
public:
   PMABatchCtn(int argc, char* argv[]);
   virtual ~PMABatchCtn();

   // build container
   duration_t create(uint32_t size) override;

   // update container
   duration_t insert(std::vector<elttype> batch) override;

   // apply function for every el<valuetype>
   duration_t scan_at_region(const region_t& region, scantype_function __apply) override;

   // apply function for every spatial area/region
   duration_t apply_at_tile(const region_t& region, applytype_function __apply) override;
   duration_t apply_at_region(const region_t& region, applytype_function __apply) override;

protected:
   diff_cnt diff();
   void clear_diff();

   uint32_t count_pma(const uint32_t& begin, const uint32_t& end, const spatial_t& el) const;
   void scan_pma(const uint32_t& begin, const uint32_t& end, const spatial_t& el, scantype_function _apply) const;

   static inline void get_mcode_range(uint64_t code, uint32_t zoom, uint64_t& min, uint64_t& max, uint32_t mCodeSize);

private:
   uint32_t seg_size;
   float tau_0, tau_h, rho_0, rho_h;

   pma_struct* _pma{ nullptr };
   std::unique_ptr<QuadtreeIntf> _quadtree;
};

void PMABatchCtn::get_mcode_range(uint64_t code, uint32_t zoom, uint64_t& min, uint64_t& max, uint32_t mCodeSize) {
   uint32_t diffDepth = mCodeSize - zoom;
   min = code << 2 * (diffDepth);
   max = min | ((uint64_t)~0 >> (64 - 2 * diffDepth));
}