#pragma once

#include "GeoCtnIntf.h"

class GeoHash : public GeoCtnIntf {
public:
   GeoHash(int argc, char* argv[]);

   virtual ~GeoHash();

   // build container
   duration_t create(uint32_t size) override;

   // update container
   duration_t insert(std::vector<elttype> batch) override;

   // apply function for every el<valuetype>
   duration_t scan_at_region(const region_t& region, scantype_function __apply) override;

   // apply function for every spatial area/region
   duration_t apply_at_tile(const region_t& region, applytype_function __apply) override;

   duration_t apply_at_region(const region_t& region, applytype_function __apply) override;

   duration_t topk_search(const region_t& region, std::vector<valuetype>& output, float alpha, uint64_t now, uint64_t time) override;

   inline virtual std::string name() const = 0;

protected:
#define PMA_ELT(x) ((*(uint64_t*)x))

   inline bool find_elt_pma(const uint64_t code_min, const uint64_t code_max, const uint32_t seg, uint32_t& offset) const;

   inline virtual bool search_pma(const spatial_t& el, uint32_t& seg) const {
      uint32_t offset;
      return search_pma(el, seg, offset);
   }

   virtual bool search_pma(const spatial_t& el, uint32_t& seg, uint32_t& offset) const = 0;

   // apply function for every el<valuetype>
   void scan_pma_at_region(const spatial_t& el, uint32_t& seg, const region_t& region, scantype_function __apply);

   // apply function for every spatial area/region
   void apply_pma_at_tile(const spatial_t& el, uint32_t& seg, const region_t& region, applytype_function __apply);

   void apply_pma_at_region(const spatial_t& el, uint32_t& seg, const region_t& region, applytype_function __apply);

   //void topk_pma_search()

   uint32_t count_pma(const spatial_t& el, uint32_t& seg) const;

   void scan_pma(const spatial_t& el, uint32_t& seg, scantype_function _apply) const;

   inline spatial_t get_parent_quadrant(const region_t& region) const;

   inline void get_mcode_range(const spatial_t& el, uint64_t& min, uint64_t& max, uint32_t morton_size) const;

   uint32_t seg_size;
   float tau_0, tau_h, rho_0, rho_h;

   pma_struct* _pma{nullptr};
};

inline bool GeoHash::find_elt_pma(const uint64_t code_min, const uint64_t code_max, const uint32_t seg, uint32_t& offset) const {
   if (seg >= _pma->nb_segments) return false;

   uint32_t nb_elts_per_seg = _pma->elts[seg];

   for (offset = 0; offset < nb_elts_per_seg; ++offset) {
      char* el_pt = SEGMENT_ELT(_pma, seg, offset);

      if (PMA_ELT(el_pt) > code_max) {
         return false;
      } else if (PMA_ELT(el_pt) >= code_min) {
         return true;
      }
   }

   return false;
}

inline spatial_t GeoHash::get_parent_quadrant(const region_t& region) const {
   uint64_t mask = region.code0 ^ region.code1;

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

   return spatial_t(prefix, region.z - depth_diff);
}

inline void GeoHash::get_mcode_range(const spatial_t& el, uint64_t& min, uint64_t& max, uint32_t morton_size) const {
   uint32_t diffDepth = (uint32_t)(morton_size - el.z);
   min = el.code << 2 * (diffDepth);
   max = min | ((uint64_t)~0 >> (64 - 2 * diffDepth));
}

class GeoHashSequential : public GeoHash {
public:
   GeoHashSequential(int argc, char* argv[]) : GeoHash(argc, argv) {
   };

   virtual ~GeoHashSequential() = default;

   std::string name() const override {
      static auto name_str = "GeoHashSequential";
      return name_str;
   }

protected:
   bool search_pma(const spatial_t& el, uint32_t& seg, uint32_t& offset) const override final;
};

class GeoHashBinary : public GeoHash {
public:
   GeoHashBinary(int argc, char* argv[]) : GeoHash(argc, argv) {
   };

   virtual ~GeoHashBinary() = default;

   std::string name() const override {
      static auto name_str = "GeoHashBinary";
      return name_str;
   }

protected:
   bool search_pma(const spatial_t& el, uint32_t& seg, uint32_t& offset) const override final;
};
