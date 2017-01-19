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

duration_t GeoHash::insert_rm(std::vector<elttype> batch,  std::function< int (const void*) > is_removed) {
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

duration_t GeoHash::topk_search(const region_t& region, std::vector<valuetype>& output, float alpha, uint64_t now, uint64_t time) {
   Timer timer;

   if (_pma == nullptr || region.z != 25) { return{ duration_info("topk_search", timer) }; }

   timer.start();

   struct cell_t {
      cell_t(pma_struct* pma, uint32_t _seg, uint32_t _offset, float alpha, float _spatial_distance, uint64_t now, uint64_t time) {
         spatial_distance = _spatial_distance;

         seg = _seg;
         offset = _offset;

         update_score(pma, alpha, now, time);
      }

      inline void update_score(pma_struct* pma, float alpha, uint64_t now, uint64_t time) {
         uint64_t m_time = (*(valuetype*)ELT_TO_CONTENT(SEGMENT_ELT(pma, seg, offset))).time;

         if (now - m_time > time) {
            score = 1.0;
         } else {
            float temporal_distance = float((now - m_time) / time);
            score = (alpha * spatial_distance) + ((1.f - alpha) * temporal_distance);
         }
      }

      inline static float get_spatial_distance(float lat0, float lon0, float lat1, float lon1) {
         static const float PI_180_INV = 180.0f / (float)M_PI;
         static const float PI_180 = (float)M_PI / 180.0f;
         static const float r_earth = 6378.f;

         float d_lat = (lat1 - lat0) * PI_180;
         float d_lon = (lon1 - lon0) * PI_180;

         float a = std::sin(d_lat / 2.0f) * std::sin(d_lat / 2.0f) +
            std::cos(lat0 * PI_180) * std::cos(lat1 * PI_180) *
            std::sin(d_lon / 2.0f) * std::sin(d_lon / 2);

         float c = 2.0f * std::atan2(std::sqrt(a), std::sqrt(1.0f - a));

         return r_earth * c;
      }

      inline bool operator>(const cell_t& e) const {
         return score > e.score;
      }

      inline bool operator<(const cell_t& e) const {
         return score < e.score;
      }

      inline valuetype elt(pma_struct* pma) {
         return (*(valuetype*)ELT_TO_CONTENT(SEGMENT_ELT(pma, seg, offset)));
      }

      inline void next(pma_struct* pma, float alpha, uint64_t now, uint64_t time) {
         uint64_t prev_code = PMA_ELT(SEGMENT_ELT(pma, seg, offset));

         if (++offset >= pma->elts[seg]) {
            offset = 0;
            ++seg;
         }

         if (seg < pma->nb_segments && prev_code == PMA_ELT(SEGMENT_ELT(pma, seg, offset))) {            
            update_score(pma, alpha, now, time);
         } else {
            score = 1.0;
         }         
      }
      
      uint32_t seg, offset;
      float score, spatial_distance;
   };

   std::priority_queue<cell_t, std::vector<cell_t>, std::greater<cell_t>> heap;

   uint32_t curr_seg = 0;
   for (uint32_t x = region.x0; x <= region.x1; ++x) {
      for (uint32_t y = region.y0; y < region.y1; ++y) {
         uint32_t offset = 0;
         if (search_pma(spatial_t(mortonEncode_RAM(x, y), 25), curr_seg, offset)) {
            //std::cout << ".";
            float m_lat = mercator_util::tiley2lat(y + 0.5, 25);
            float m_lon = mercator_util::tilex2lon(x + 0.5, 25);
            
            float spatial_distance = cell_t::get_spatial_distance(region.lat, region.lon, m_lat, m_lon);
            heap.push(cell_t(_pma, curr_seg, offset, alpha, spatial_distance, now, time));
         }
      }
   }
   
   static const float max_score = 1.0;
   while (!heap.empty()) {
      cell_t cell = heap.top();
      heap.pop();

      if (cell.score > max_score) {
         break;
      }

      while (cell.score < max_score && (heap.empty() || cell.score <= heap.top().score)) {
         output.emplace_back(cell.elt(_pma));
         cell.next(_pma, alpha, now, time);
      }

      if (cell.score < max_score) {
         heap.push(cell);
      }
   }

   timer.stop();

   return{ duration_info("topk_search", timer) };
}

void GeoHash::scan_pma_at_region(const spatial_t& el, uint32_t& seg, const region_t& region, scantype_function __apply) {
   if (!search_pma(el, seg)) return;

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
      //std::cout << el.code << ":" << el.z << std::endl;
      __apply(el, count_pma(el, seg));
   }
}

void GeoHash::apply_pma_at_region(const spatial_t& el, uint32_t& seg, const region_t& region, applytype_function __apply) {
   if (!search_pma(el, seg)) return;

   if (region.cover(el)) {
      //std::cout << el.code << ":" << el.z << std::endl;
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

uint32_t GeoHash::count_pma(const spatial_t& el, uint32_t& seg) const {
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

void GeoHash::scan_pma(const spatial_t& el, uint32_t& seg, scantype_function _apply) const {
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

bool GeoHashSequential::search_pma(const spatial_t& el, uint32_t& seg, uint32_t& offset) const {
   if (_pma == nullptr || seg >= _pma->nb_segments) return false;

   uint64_t code_min, code_max;
   get_mcode_range(el, code_min, code_max, 25);

   for (; seg < _pma->nb_segments; ++seg) {
      if (PMA_ELT(SEGMENT_LAST(_pma, seg)) < code_min) {
         // next segment
         continue;
      } else {
         return find_elt_pma(code_min, code_max, seg, offset);
      }
   }

   return false;
}

bool GeoHashBinary::search_pma(const spatial_t& el, uint32_t& seg, uint32_t& offset) const {
   if (_pma == nullptr || seg >= _pma->nb_segments) return false;

   uint64_t code_min, code_max;
   get_mcode_range(el, code_min, code_max, 25);

   // current segment
   if (PMA_ELT(SEGMENT_LAST(_pma, seg)) >= code_min) {
      return find_elt_pma(code_min, code_max, seg, offset);
   }

   // next segment
   if (++seg >= _pma->nb_segments) return false;

   //binary search
   if (PMA_ELT(SEGMENT_LAST(_pma, seg)) < code_min) {
      uint32_t it;
      int32_t count, step;
      count = (int32_t)_pma->nb_segments - (int32_t)seg;

      while (count > 0) {
         it = seg;
         step = count / 2;
         it += step;
         if (PMA_ELT(SEGMENT_LAST(_pma, it)) < code_min) {
            seg = ++it;
            count -= step + 1;
         } else {
            count = step;
         }
      }
   }

   // find at segment
   return find_elt_pma(code_min, code_max, seg, offset);
}
