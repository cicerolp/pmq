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

   uint32_t curr_seg = 0;
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

void GeoHash::scan_pma_at_region(const spatial_t& el, uint32_t& seg, const region_t& region, scantype_function __apply) {
   if (!search_pma(el, seg)) return;

   if (region.cover(el)) {
      apply_pma(el, seg, __apply);

   } else if (el.z < region.z) {
      // break code into four
      uint64_t code = el.code << 2;

      scan_pma_at_region(spatial_t(code | 0, (uint32_t)(el.z + 1)), seg, region, __apply);
      scan_pma_at_region(spatial_t(code | 1, (uint32_t)(el.z + 1)), seg, region, __apply);
      scan_pma_at_region(spatial_t(code | 2, (uint32_t)(el.z + 1)), seg, region, __apply);
      scan_pma_at_region(spatial_t(code | 3, (uint32_t)(el.z + 1)), seg, region, __apply);
   }
}

void GeoHash::apply_pma_at_tile(const spatial_t& el, uint32_t& seg, const region_t& region, applytype_function __apply) {
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
   if (!search_pma(el, seg)) return;

   if (region.cover(el)) {
      __apply(el, count_pma(el, seg));

   } else if (el.z < region.z) {
      // break morton code into four
      uint64_t code = el.code << 2;

      apply_pma_at_region(spatial_t(code | 0, (uint32_t)(el.z + 1)), seg, region, __apply);
      apply_pma_at_region(spatial_t(code | 1, (uint32_t)(el.z + 1)), seg, region, __apply);
      apply_pma_at_region(spatial_t(code | 2, (uint32_t)(el.z + 1)), seg, region, __apply);
      apply_pma_at_region(spatial_t(code | 3, (uint32_t)(el.z + 1)), seg, region, __apply);
   }
}

uint32_t GeoHash::count_pma(const spatial_t & el, uint32_t& seg) const {
   if (_pma == nullptr) return 0;

   uint64_t code_min, code_max;
   get_mcode_range(el, code_min, code_max, 25);

   uint32_t count = 0;
   uint32_t prev_seg = seg;
   
   while (seg < _pma->nb_segments && PMA_ELT(SEGMENT_START(_pma, seg)) <= code_max) {
      count += _pma->elts[seg];
      seg++;
   }
   seg--;

   uint32_t offset = 0;
   //subtract extra elements for first segment
   while (PMA_ELT(SEGMENT_ELT(_pma, prev_seg, offset)) < code_min) {
      offset++;
      count--;
   }

   offset = _pma->elts[seg] - 1;
   //subtract extra elements for the last segment
   while (PMA_ELT(SEGMENT_ELT(_pma, seg, offset)) > code_max) {
      offset--;
      count--;
   }

   return count;
}

void GeoHash::apply_pma(const spatial_t & el, uint32_t& seg, scantype_function _apply) const {
   if (_pma == nullptr) return;
   
   uint64_t code_min, code_max;
   get_mcode_range(el, code_min, code_max, 25);
   
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

bool GeoHashSequential::search_pma(const spatial_t& el, uint32_t& seg) const {
   if (_pma == nullptr || seg >= _pma->nb_segments) return false;

   uint64_t code_min, code_max;
   get_mcode_range(el, code_min, code_max, 25);

   for (; seg < _pma->nb_segments; ++seg) {
      if (PMA_ELT(SEGMENT_LAST(_pma, seg)) < code_min) {
         // next segment
         continue;
      } else {
         uint32_t nb_elts_per_seg = _pma->elts[seg];

         for (uint32_t offset = 0; offset < nb_elts_per_seg; ++offset) {
            char* el_pt = (char*)SEGMENT_ELT(_pma, seg, offset);

            if (PMA_ELT(el_pt) > code_max) {
               return false;
            } else if (PMA_ELT(el_pt) >= code_min) {
               return true;
            }
         }
      }
   }

   return false;
}

bool GeoHashBinary::search_pma(const spatial_t& el, uint32_t& seg) const {
   if (_pma == nullptr || seg >= _pma->nb_segments) return false;

   uint64_t code_min, code_max;
   get_mcode_range(el, code_min, code_max, 25);

   if (PMA_ELT(SEGMENT_START(_pma, seg)) > code_max) {
      return false;

   } else if (PMA_ELT(SEGMENT_START(_pma, seg)) < code_min) {
      //binary search
      uint32_t it, first;
      first = seg;

      int32_t count, step;
      count = _pma->nb_segments - seg;

      while (count > 0) {
         it = first;
         step = count / 2;
         it += step;
         if (PMA_ELT(SEGMENT_LAST(_pma, it)) < code_min) {
            first = ++it;
            count -= step + 1;
         } else {
            count = step;
         }
      }

      seg = first;

      if (PMA_ELT(SEGMENT_START(_pma, seg)) <= code_max) {
         uint32_t nb_elts_per_seg = _pma->elts[seg];

         for (uint32_t offset = 0; offset < nb_elts_per_seg; ++offset) {
            char* el_pt = SEGMENT_ELT(_pma, seg, offset);

            if (PMA_ELT(el_pt) > code_max) {
               return false;
            } else if (PMA_ELT(el_pt) >= code_min) {
               return true;
            }
         }
         return false;
      } else {
         return false;
      }
   } else {
      return true;
   }
}
