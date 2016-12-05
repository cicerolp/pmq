#include "stde.h"
#include "GeoHash.h"

GeoHash::GeoHash(int argc, char* argv[]) {
	seg_size = cimg_option("-s", 8, "pma::batch arg: segment size");
	tau_0 = cimg_option("-t0", 0.92, "pma::batch arg: tau_0");
	tau_h = cimg_option("-th", 0.7, "pma::batch arg: tau_h");
	rho_0 = cimg_option("-r0", 0.08, "pma::batch arg: rho_0");
	rho_h = cimg_option("-rh", 0.3, "pma::batch arg: rho_0");
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
   while ((mask & 3) == 0) {
      depth_diff++;
      mask = mask >> 2;      
   }

   prefix = prefix >> (depth_diff * 2);

	std::vector<spatial_t> u_codes; // unrefined codes
   std::vector<spatial_t> t_codes; // temporary codes

	u_codes.emplace_back(prefix, region.z - depth_diff);

	while (!u_codes.empty()) {

      // clear temporary storage
      t_codes.clear();

		for(auto& el : u_codes) {

         if (!naive_search_pma(el)) continue;

         if (region.cover(el)) {
            scan_pma(el, __apply);

			} else if (el.z < region.z) {
            // break code into four
            uint64_t code = el.code << 2;
				
            t_codes.emplace_back(code | 0, el.z + 1);
            t_codes.emplace_back(code | 1, el.z + 1);
            t_codes.emplace_back(code | 2, el.z + 1);
            t_codes.emplace_back(code | 3, el.z + 1);
			}
		}
		u_codes.swap(t_codes);
	}

	timer.stop();

	return {duration_info("scan_at_region", timer)};
}

duration_t GeoHash::apply_at_tile(const region_t& region, applytype_function __apply) {
	Timer timer;

	if (_pma == nullptr) { return {duration_info("apply_at_tile", timer)}; }

	timer.start();

	timer.stop();

	return {duration_info("apply_at_tile", timer)};
}

duration_t GeoHash::apply_at_region(const region_t& region, applytype_function __apply) {
	Timer timer;

	if (_pma == nullptr) { return {duration_info("apply_at_region", timer)}; }

   timer.start();

   timer.stop();

	return {duration_info("apply_at_region", timer)};
}

bool GeoHash::naive_search_pma(const spatial_t& el) const {
   if (_pma == nullptr) return false;

   uint64_t code_min = 0;
   uint64_t code_max = 0;
   get_mcode_range(el, code_min, code_max, 25);

   for (uint32_t seg = 0; seg < _pma->nb_segments; ++seg) {

      if (PMA_SEG(SEGMENT_LAST(_pma, seg)) < code_min) {
         // next segment
         continue;
      } else {
         uint32_t nb_elts_per_seg = _pma->elts[seg];

         for (uint32_t offset = 0; offset < nb_elts_per_seg; ++offset) {
            char* el_pt = (char*)SEGMENT_ELT(_pma, seg, offset);

            if (PMA_SEG(el_pt) > code_max) {
               return false;
            } else if (PMA_SEG(el_pt) >= code_min) {
               return true;
            }
         }
      }
   }
}

uint32_t GeoHash::count_pma(const spatial_t & el) const {
   if (_pma == nullptr) return 0;

   uint32_t count = 0;

   uint64_t code_min = 0;
   uint64_t code_max = 0;
   get_mcode_range(el, code_min, code_max, 25);

   for (uint32_t seg = 0; seg < _pma->nb_segments; ++seg) {

      if (PMA_SEG(SEGMENT_LAST(_pma, seg)) < code_min) {
         // next segment
         continue;
      } else {
         uint32_t nb_elts_per_seg = _pma->elts[seg];

         for (uint32_t offset = 0; offset < nb_elts_per_seg; ++offset) {
            char* el_pt = (char*)SEGMENT_ELT(_pma, seg, offset);

            if (PMA_SEG(el_pt) > code_max) {
               return count;
            } else if (PMA_SEG(el_pt) >= code_min) {

               if (PMA_SEG(SEGMENT_LAST(_pma, seg)) <= code_max) {
                  count += nb_elts_per_seg - offset;
                  break;
               } else {
                  count++;
               }               
            }
         }
      }
   }

   return count;
}

void GeoHash::scan_pma(const spatial_t & el, scantype_function _apply) const {
   if (_pma == nullptr) return;

   uint64_t code_min = 0;
   uint64_t code_max = 0;
   get_mcode_range(el, code_min, code_max, 25);

   for (uint32_t seg = 0; seg < _pma->nb_segments; ++seg) {

      if (PMA_SEG(SEGMENT_LAST(_pma, seg)) < code_min) {
         // next segment
         continue;
      } else {
         uint32_t nb_elts_per_seg = _pma->elts[seg];

         for (uint32_t offset = 0; offset < nb_elts_per_seg; ++offset) {
            char* el_pt = (char*)SEGMENT_ELT(_pma, seg, offset);

            if (PMA_SEG(el_pt) > code_max) {
               return;
            } else if (PMA_SEG(el_pt) >= code_min) {
               _apply(*(valuetype*)ELT_TO_CONTENT(el_pt));
            }
         }
      }
   }
}
