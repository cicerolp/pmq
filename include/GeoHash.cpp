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

   gfx::timsort(batch.begin(), batch.end());
   //std::sort(batch.begin(), batch.end());

   void* begin = (void *)(&batch[0]);
   void* end = (void *)((char *)(&batch[0]) + (batch.size()) * sizeof(elttype));

   pma::batch::add_array_elts(_pma, begin, end, comp<uint64_t>);

   // insert end
   timer.stop();

   return {duration_info("Insert", timer)};
}

duration_t GeoHash::insert_rm(std::vector<elttype> batch, std::function<int (const void*)> is_removed) {
   Timer timer;

   if (_pma == nullptr) { return {duration_info("Error", timer)}; }

   // insert start
   timer.start();

   std::sort(batch.begin(), batch.end());

   void* begin = (void *)(&batch[0]);
   void* end = (void *)((char *)(&batch[0]) + (batch.size()) * sizeof(elttype));

   pma::batch::add_rm_array_elts(_pma, begin, end, comp<uint64_t>, is_removed);

   // insert end
   timer.stop();

   return {duration_info("Insert RM", timer)};
}

/**
 * @brief GeoHash::scan_at_region applies the function __apply on the elements inside a region of the implicit quadtree.
 * @param region A square region in the implict quadtree.
 * @param __apply
 * @return
 */
duration_t GeoHash::scan_at_region(const region_t& region, scantype_function __apply) {
   Timer timer;

   if (_pma == nullptr) { return {duration_info("scan_at_region", timer)}; }

   timer.start();

   auto curr_seg = pma_seg_it::begin(_pma);
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

   auto curr_seg = pma_seg_it::begin(_pma);
   apply_pma_at_region(get_parent_quadrant(region), curr_seg, region, __apply);

   timer.stop();

   return {duration_info("apply_at_region", timer)};
}

duration_t GeoHash::topk_search(const region_t& region, const topk_t& topk, std::vector<valuetype>& output) {
   Timer timer;

   if (_pma == nullptr) { return {duration_info("topk_search", timer)}; }

   timer.start();

   // store intermediate results
   topk_cnt container;

   // ranking function
   scantype_function __apply = std::bind(
      [](const region_t& region, const topk_t& topk, topk_cnt& container, const valuetype& el) {
         // spatial distance
         static const float PI_180_INV = 180.0f / (float)M_PI;
         static const float PI_180 = (float)M_PI / 180.0f;
         static const float r_earth = 6378.f;

         float d_lat = (el.latitude - region.lat) * PI_180;
         float d_lon = (el.longitude - region.lon) * PI_180;

         float a = std::sin(d_lat / 2.0f) * std::sin(d_lat / 2.0f) +
            std::cos(region.lat * PI_180) * std::cos(el.latitude * PI_180) *
            std::sin(d_lon / 2.0f) * std::sin(d_lon / 2);

         float c = 2.0f * std::atan2(std::sqrt(a), std::sqrt(1.0f - a));

         float spatial_score = (r_earth * c) / float(topk.distance);

         if (spatial_score > 1.f) return;

         // temporal distance
         float temporal_score;
         if (topk.now - el.time > topk.time) {
            return;
         } else {
            temporal_score = (topk.now - el.time) / float(topk.time);
         }

         // ranking score
         float score = (topk.alpha * spatial_score) + ((1.f - topk.alpha) * temporal_score);

         container.insert(topk, el, score);

      }, std::ref(region), std::ref(topk), std::ref(container), std::placeholders::_1);

   auto curr_seg = pma_seg_it::begin(_pma);
   scan_pma_at_region(get_parent_quadrant(region), curr_seg, region, __apply);

   // sort elements by score
   gfx::timsort(container.ctn.begin(), container.ctn.end(),
                [](const topk_elt& lhs, const topk_elt& rhs) {
                   return lhs.score < rhs.score;
                });

   size_t size = std::min(container.ctn.size(), (size_t)topk.k);
   output.resize(size);

   // copy to output vector
   std::copy(container.ctn.begin(), container.ctn.begin() + size, output.begin());

   timer.stop();

   return {duration_info("topk_search", timer)};
}

/**
 * @brief GeoHash::scan_pma_at_region
 * @param el : a prefix mortonCode on the implicit quadtree and is detph (z)
 * @param seg : a lower bound on the segment that contains the elements inside region;
 * @param region : a square region on implicit quadtree.
 * @param __apply
 *
 * Applies function __apply in every element inside region r;
 *
 */

void GeoHash::scan_pma_at_region(const spatial_t& el, pma_seg_it& seg, const region_t& region, scantype_function __apply) {
   // search in the PMA if there is an element starting with prefix el ; (starts the search at segment seg )
   if (search_pma(el, seg) == pma_seg_it::end(_pma)) return;

   if (region.cover(el)) {
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
   if (search_pma(el, seg) == pma_seg_it::end(_pma)) return;

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

void GeoHash::apply_pma_at_region(const spatial_t& el, pma_seg_it& seg, const region_t& region, applytype_function __apply) {
   if (search_pma(el, seg) == pma_seg_it::end(_pma)) return;

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

uint32_t GeoHash::count_pma(const spatial_t& el, pma_seg_it& seg) const {
   uint64_t code_min, code_max;
   get_mcode_range(el, code_min, code_max, 25);

   size_t count = 0;
   auto prev_seg = seg;

   do {
      count += seg.size();
   } while (++seg != pma_seg_it::end(_pma) && PMA_ELT(*seg) <= code_max);

   assert(count != 0);

   pma_offset_it off_begin, off_end;

   if (seg != pma_seg_it::end(_pma)) {
      count += seg.size();

      //subtract extra elements for last segment
      off_end = pma_offset_it::end(_pma, seg);
      off_begin = std::lower_bound(pma_offset_it::begin(_pma, seg), pma_offset_it::end(_pma, seg), code_max + 1,
                                   [](void* elt, uint64_t value) {
                                      return PMA_ELT(elt) < value;
                                   });
      count -= seg.size() - (off_end - off_begin);

      assert(count != 0);
   }

   //subtract extra elements for first segment
   off_begin = pma_offset_it::begin(_pma, prev_seg);
   off_end = std::lower_bound(off_begin, pma_offset_it::end(_pma, prev_seg), code_min,
                              [](void* elt, uint64_t value) {
                                 return PMA_ELT(elt) < value;
                              });
   count -= (off_end - off_begin);

   assert(count != 0);

   return (uint32_t)count;
}

void GeoHash::scan_pma(const spatial_t& el, pma_seg_it& seg, scantype_function _apply) const {
   uint64_t code_min, code_max;
   get_mcode_range(el, code_min, code_max, 25);

   auto seg_end = pma_seg_it::end(_pma);

   while (seg != seg_end) {
      auto off_it = pma_offset_it::begin(_pma, seg);
      auto off_end = pma_offset_it::end(_pma, seg);

      while (off_it != off_end) {
         if (PMA_ELT(*off_it) > code_max) {
            return;
         } else if (PMA_ELT(*off_it) >= code_min) {
            _apply(*(valuetype*)ELT_TO_CONTENT(*off_it));
         }
         ++off_it;
      }
      ++seg;
   }
}

/**
 * @brief GeoHashSequential::search_pma
 * Sequential search on the pma.
 */
pma_seg_it GeoHashSequential::search_pma(const spatial_t& el, pma_seg_it& seg) const {
   uint64_t code_min, code_max;
   get_mcode_range(el, code_min, code_max, 25);

   auto end = pma_seg_it::end(_pma);

   while (seg != end) {
      if (PMA_ELT(*seg) >= code_min) break;
      ++seg;
   }

   if (seg != end) return find_elt_pma(code_min, code_max, seg);

   return end;
}

pma_seg_it GeoHashBinary::search_pma(const spatial_t& el, pma_seg_it& seg) const {
   uint64_t code_min, code_max;
   get_mcode_range(el, code_min, code_max, 25);

   // current segment
   if (PMA_ELT(*seg) >= code_min) {
      return find_elt_pma(code_min, code_max, seg);
   }

   auto end = pma_seg_it::end(_pma);

   seg = std::lower_bound(seg, end, code_min,
                          [](void* elt, uint64_t value) {
                             return PMA_ELT(elt) < value;
                          });

   if (seg != end) return find_elt_pma(code_min, code_max, seg);

   return end;
}
