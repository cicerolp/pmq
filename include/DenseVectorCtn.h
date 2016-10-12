#pragma once

#include "GeoCtnIntf.h"
#include "QuadtreeIntf.h"

class DenseVectorCtn : public GeoCtnIntf {
public:
   DenseVectorCtn();
   virtual ~DenseVectorCtn() = default;

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

private:
   virtual void sort(std::vector<elttype>& cnt) = 0;

   uint32_t _diff_index {0};
   std::vector<elttype> _container;
   std::unique_ptr<QuadtreeIntf> _quadtree;
};

class DenseCtnStdSort : public DenseVectorCtn {
protected:
   inline void sort(std::vector<elttype>& cnt) override final {
      std::sort(cnt.begin(), cnt.end());
   }
};

class DenseCtnTimSort : public DenseVectorCtn {
protected:
   inline void sort(std::vector<elttype>& cnt) override final {
      gfx::timsort(cnt.begin(), cnt.end());
   }
};