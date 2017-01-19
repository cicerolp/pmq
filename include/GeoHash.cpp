#include "stde.h"
#include "GeoHash.h"

GeoHash::GeoHash(int argc, char* argv[]) {
   seg_size = cimg_option("-s", 8, "GeoHash arg: segment size");
   tau_0 = cimg_option("-t0", 0.92f, "GeoHash arg: tau_0");
   tau_h = cimg_option("-th", 0.7f, "GeoHash arg: tau_h");
   rho_0 = cimg_option("-r0", 0.08f, "GeoHash arg: rho_0");
   rho_h = cimg_option("-rh", 0.3f, "GeoHash arg: rho_0");
}

GeoHash::~GeoHash() { if (_pma != nullptr) pma::destroy_pma(_pma); }

duration_t GeoHash::create(uint32_t size) {
   Timer timer;

   timer.start();
   _pma = (struct pma_struct *) pma::build_pma(size, sizeof(valuetype), tau_0, tau_h, rho_0, rho_h, seg_size);
   timer.stop();

   return {duration_info("Create", timer)};
}

duration_t GeoHash::insert(std::vector<elttype> batch) {
   Timer timer;

   if (_pma == nullptr) { return {duration_info("Error", timer)}; }

   // insert start
   timer.start();

   std::sort(batch.begin(), batch.end());

   void* begin = (void *)(&batch[0]);
   void* end = (void *)((char *)(&batch[0]) + (batch.size()) * sizeof(elttype));

   pma::batch::add_array_elts(_pma, begin, end, comp<uint64_t>);

   // insert end
   timer.stop();

   return {duration_info("Insert", timer)};
}

duration_t GeoHash::scan_at_region(const region_t& region, scantype_function __apply) {
   Timer timer;

   if (_pma == nullptr) { return {duration_info("scan_at_region", timer)}; }

   timer.start();

   uint32_t curr_seg = 0;
   scan_pma_at_region(get_parent_quadrant(region), curr_seg, region, __apply);

   timer.stop();

   return {duration_info("scan_at_region", timer)};
}

duration_t GeoHash::apply_at_tile(const region_t& region, applytype_function __apply) {
   Timer timer;

   if (_pma == nullptr) { return {duration_info("apply_at_tile", timer)}; }

   timer.start();

   auto curr_seg = pma_seg_it::begin(_pma);
   apply_pma_at_tile(get_parent_quadrant(region), curr_seg, region, __apply);

   timer.stop();

   return {duration_info("apply_at_tile", timer)};
}

duration_t GeoHash::apply_at_region(const region_t& region, applytype_function __apply) {
   Timer timer;

   if (_pma == nullptr) { return {duration_info("apply_at_region", timer)}; }

   timer.start();

   uint32_t curr_seg = 0;
   apply_pma_at_region(get_parent_quadrant(region), curr_seg, region, __apply);

   timer.stop();

   return {duration_info("apply_at_region", timer)};
}

duration_t GeoHash::topk_search(const region_t& region, std::vector<valuetype>& output, float alpha, uint64_t now, uint64_t time) {
   Timer timer;

   if (_pma == nullptr || region.z != 25) { return {duration_info("topk_search", timer)}; }

   timer.start();

   timer.stop();

   return {duration_info("topk_search", timer)};
}

void GeoHash::scan_pma_at_region(const spatial_t& el, uint32_t& seg, const region_t& region, scantype_function __apply) {
   //if (!search_pma(el, seg)) return;

   if (region.cover(el)) {
      //std::cout << el.code << ":" << el.z << std::endl;
      scan_pma(el, seg, __apply);

   } else if (el.z < region.z) {
      // break code into four
      uint64_t code = el.code << 2;

      scan_pma_at_region(spatial_t(code | 0, (uint32_t)(el.z + 1)), seg, region, __apply);
      scan_pma_at_region(spatial_t(code | 1, (uint32_t)(el.z + 1)), seg, region, __apply);
      scan_pma_at_region(spatial_t(code | 2, (uint32_t)(el.z + 1)), seg, region, __apply);
      scan_pma_at_region(spatial_t(code | 3, (uint32_t)(el.z + 1)), seg, region, __apply);
   }
}

void GeoHash::apply_pma_at_tile(const spatial_t& el, pma_seg_it& seg, const region_t& region, applytype_function __apply) {
   if (!search_pma(el, seg)) return;

   if ((el.z < 25) && ((int)el.z - (int)region.z) < 8) {
      // break morton code into four
      uint64_t code = el.code << 2;

      apply_pma_at_tile(spatial_t(code | 0, (uint32_t)(el.z + 1)), seg, region, __apply);
      apply_pma_at_tile(spatial_t(code | 1, (uint32_t)(el.z + 1)), seg, region, __apply);
      apply_pma_at_tile(spatial_t(code | 2, (uint32_t)(el.z + 1)), seg, region, __apply);
      apply_pma_at_tile(spatial_t(code | 3, (uint32_t)(el.z + 1)), seg, region, __apply);

   } else if (region.cover(el)) {
      __apply(el, count_pma(el, seg));
   }
}

void GeoHash::apply_pma_at_region(const spatial_t& el, uint32_t& seg, const region_t& region, applytype_function __apply) {
   //if (!search_pma(el, seg)) return;

   if (region.cover(el)) {
      //__apply(el, count_pma(el, seg));

   } else if (el.z < region.z) {
      // break morton code into four
      uint64_t code = el.code << 2;

      apply_pma_at_region(spatial_t(code | 0, (uint32_t)(el.z + 1)), seg, region, __apply);
      apply_pma_at_region(spatial_t(code | 1, (uint32_t)(el.z + 1)), seg, region, __apply);
      apply_pma_at_region(spatial_t(code | 2, (uint32_t)(el.z + 1)), seg, region, __apply);
      apply_pma_at_region(spatial_t(code | 3, (uint32_t)(el.z + 1)), seg, region, __apply);
   }
}

uint32_t GeoHash::count_pma(const spatial_t& el, pma_seg_it& seg) const {
   if (_pma == nullptr) return 0;

   uint64_t code_min, code_max;
   get_mcode_range(el, code_min, code_max, 25);

   uint32_t count = 0;
   auto prev_seg = seg;

   while (seg != pma_seg_it::end(_pma) && PMA_ELT(*seg) <= code_max) {
      ++seg;
   }

   count += prev_seg - seg;

   auto off_begin = pma_offset_it::begin(_pma, prev_seg);
   auto off_end = pma_offset_it::end(_pma, prev_seg);

   //subtract extra elements for first segment
   while (off_begin != off_end && PMA_ELT(*off_begin) < code_min) {
      ++off_begin;
      --count;
   }

   off_begin = pma_offset_it::begin(_pma, seg);
   off_end = pma_offset_it::end(_pma, seg);

   //subtract extra elements for first segment
   while (off_begin != off_end) {
      if (PMA_ELT(*off_begin) > code_max) --count;
      ++off_begin;
   }

   return count;
}

void GeoHash::scan_pma(const spatial_t& el, uint32_t& seg, scantype_function _apply) const {
   if (_pma == nullptr) return;

   uint64_t code_min, code_max;
   get_mcode_range(el, code_min, code_max, 25);

   /*auto seg_begin = pma_seg_it::begin(_pma) += seg;
   auto seg_end = pma_seg_it::end(_pma);

   for (auto it = seg_begin; it != seg_end; ++it) {
      
   }*/

   for (; seg < _pma->nb_segments; ++seg) {

      uint32_t nb_elts_per_seg = _pma->elts[seg];

      for (uint32_t offset = 0; offset < nb_elts_per_seg; ++offset) {
         char* el_pt = (char*)SEGMENT_ELT(_pma, seg, offset);

         if (PMA_ELT(el_pt) > code_max) {
            return;
         } else if (PMA_ELT(el_pt) >= code_min) {
            _apply(*(valuetype*)ELT_TO_CONTENT(el_pt));
         }
      }
   }
}

bool GeoHashSequential::search_pma(const spatial_t& el, pma_seg_it& seg, uint32_t& offset) const {
   if (_pma == nullptr) return false;

   uint64_t code_min, code_max;
   get_mcode_range(el, code_min, code_max, 25);

   auto end = pma_seg_it::end(_pma);

   while (seg != end) {
      if (PMA_ELT(*seg) >= code_min) break;;
      ++seg;
   }

   if (seg != end) return find_elt_pma(code_min, code_max, seg, offset);

   return false;
}

bool GeoHashBinary::search_pma(const spatial_t& el, pma_seg_it& seg, uint32_t& offset) const {
   if (_pma == nullptr) return false;

   uint64_t code_min, code_max;
   get_mcode_range(el, code_min, code_max, 25);

   auto end = pma_seg_it::end(_pma);

   seg = std::lower_bound(seg, end, code_min,
                          [](void* elt, uint64_t value) {
                             return PMA_ELT(elt) < value;
                          });

   if (seg != end) return find_elt_pma(code_min, code_max, seg, offset);

   return false;
}
