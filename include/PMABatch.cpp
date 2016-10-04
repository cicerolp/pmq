#include "PMABatch.h"

PMABatch::~PMABatch() {
   if (_pma != nullptr)
      pma::destroy_pma(_pma);
}

duration_t PMABatch::create(uint32_t size, int argc, char* argv[]) {
   const uint32_t seg_size(cimg_option("-s", 8, "pma::batch arg: segment size")); 
   const float tau_0(cimg_option("-t0", 0.92, "pma::batch arg: tau_0"));
   const float tau_h(cimg_option("-th", 0.7 , "pma::batch arg: tau_h"));
   const float rho_0(cimg_option("-r0", 0.08, "pma::batch arg: rho_0"));
   const float rho_h(cimg_option("-rh", 0.3 , "pma::batch arg: rho_0"));
   
   const char* is_help = cimg_option("-h", (char*)0, 0);

   std::chrono::time_point<resolution_t> t_point = resolution_t::now();

   if (is_help) return resolution_t::now() - t_point;

   _pma = (struct pma_struct *) pma::build_pma(size, sizeof(valuetype), tau_0, tau_h, rho_0, rho_h, seg_size);

   return resolution_t::now() - t_point;
}

duration_t PMABatch::insert(std::vector<elttype> batch) {
   std::chrono::time_point<resolution_t> t_point = resolution_t::now();
   if (_pma == nullptr) return resolution_t::now() - t_point;

   // sorting algorithm
   std::sort(batch.begin(), batch.end());

   void *begin = (void *)(&batch[0]);
   void *end = (void *)((char *)(&batch[0]) + (batch.size()) * sizeof(elttype));

   pma::batch::add_array_elts(_pma, begin, end, comp<uint64_t>);

   return resolution_t::now() - t_point;
}

duration_t PMABatch::diff(std::vector<elinfo_t>& keys) {
   std::chrono::time_point<resolution_t> t_point = resolution_t::now();
   if (_pma == nullptr) return resolution_t::now() - t_point;

   for (unsigned int wId : *(_pma->last_rebalanced_segs)) {
      //We don't need to track rebalance on leaf segments (TODO : could remove it from the PMA rebalance function)
      //  if (wId < pma->nb_segments)
      //    continue;

      unsigned int sStart = pma_get_window_start(_pma, wId); //Get first segment of the window

      char* el_pt = (char*)SEGMENT_START(_pma, sStart); //get first element of the segment
      uint64_t lastElKey = *(uint64_t*)el_pt; //mcode of the first element

      uint64_t fstSeg = sStart;
      // check it this key appear previously in the pma.
      while (fstSeg > 0 && (lastElKey == *(uint64_t*)SEGMENT_LAST(_pma, fstSeg - 1))) {
         fstSeg--;
      }

      keys.emplace_back(lastElKey, fstSeg, sStart); //save the start for this key. Initialy end = begin+1 (open interval)

      // loop over the segments of the current window
      unsigned int s;
      for (s = sStart; s < sStart + pma_get_window_size(_pma, wId); s++) {
         el_pt = (char*)SEGMENT_START(_pma, s);

         // if we changed segment but the lastkey is also in first postition of this segment, we incremente its "end" index;
         if (lastElKey == (*(uint64_t*)el_pt)) {
            keys.back().end++;
         }

         for (el_pt; el_pt < SEGMENT_ELT(_pma, s, _pma->elts[s]); el_pt += _pma->elt_size) {

            if (lastElKey != (*(uint64_t*)el_pt)) {
               lastElKey = *(uint64_t*)el_pt;
               keys.emplace_back(lastElKey, s, s + 1);
            }
         }
      }

      // if the last lastKey continues outside of this range of windows we remove it;
      // modified.pop_back();

      // ACTUALLY I THINK WE STILL NEED IT; 
      // if the begin of the last range was modified we still need to inform to the quadtree.

      // we still need to find the end of last element (can be out of the current window)
      // initialize 'end' with the last segment of the pma + 1
      keys.back().end = _pma->nb_segments;

      for (char* seg_pt = (char*)SEGMENT_START(_pma, s); seg_pt < SEGMENT_START(_pma, _pma->nb_segments); seg_pt += (_pma->cap_segments * _pma->elt_size)) {
         // Check the first key of the following segments until we find one that differs;
         if (lastElKey != *(uint64_t*)seg_pt) {
            keys.back().end = (seg_pt - (char*)_pma->array) / (_pma->cap_segments * _pma->elt_size); //gets they index of segment that differs;
            break;
         }
      }
   }

   return resolution_t::now() - t_point;
}

duration_t PMABatch::count(const uint32_t& begin, const uint32_t& end, const spatial_t& el, uint32_t& count) const {
   std::chrono::time_point<resolution_t> t_point = resolution_t::now();
   if (_pma == nullptr) return resolution_t::now() - t_point;

   uint64_t mCodeMin = 0;
   uint64_t mCodeMax = 0;
   get_mcode_range(el.code, el.z, mCodeMin, mCodeMax,25);

   for (unsigned int s = begin; s < end; s++) {
      count += _pma->elts[s];
   }

   //subtract extra elements for first segment
   for (char* el_pt = (char*)SEGMENT_START(_pma, begin); (*(uint64_t*)el_pt) < mCodeMin; el_pt += _pma->elt_size) {
      count--;
   }

   //subtract extra elements for the last segment
   for (char* el_pt = (char*)SEGMENT_ELT(_pma, end - 1, _pma->elts[end - 1] - 1); (*(uint64_t*)el_pt) > mCodeMax; el_pt -= _pma->elt_size) {
      count--;
   }
   
   return resolution_t::now() - t_point;
}

duration_t PMABatch::apply(const uint32_t& begin, const uint32_t& end, const spatial_t& el, elttype_function _apply) const {
   std::chrono::time_point<resolution_t> t_point = resolution_t::now();
   if (_pma == nullptr ) return resolution_t::now() - t_point;

   uint64_t mCodeMin, mCodeMax;
   get_mcode_range(el.code, el.z, mCodeMin, mCodeMax,25);

   //Find the first element of the first segment
   char* cur_el_pt = (char*)SEGMENT_START(_pma, begin);
   while ((*(uint64_t*)cur_el_pt) < mCodeMin)
      cur_el_pt += _pma->elt_size;

   //loop on the first segments (up to one before last)
   for (unsigned int s = begin; s < end - 1; ++s, cur_el_pt = (char*)SEGMENT_START(_pma, s)) {

      for (; cur_el_pt < (char*)SEGMENT_ELT(_pma, s, _pma->elts[s]); cur_el_pt += _pma->elt_size) {
         _apply((void*)cur_el_pt);

      }
   }

   //loop on last segment
   for (; cur_el_pt < (char*)SEGMENT_ELT(_pma, end - 1, _pma->elts[end - 1]) && *(uint64_t*)cur_el_pt <= mCodeMax; cur_el_pt += _pma->elt_size) {
      _apply((void*)cur_el_pt);

   }

   return resolution_t::now() - t_point;
}

duration_t PMABatch::apply(const uint32_t& begin, const uint32_t& end, const spatial_t& el, uint32_t& count, uint32_t max, valuetype_function _apply) const {
   std::chrono::time_point<resolution_t> t_point = resolution_t::now();
   if (_pma == nullptr || count >= max) return resolution_t::now() - t_point;

   uint64_t mCodeMin, mCodeMax;
   get_mcode_range(el.code, el.z, mCodeMin, mCodeMax,25);

   //Find the first element of the first segment
   char* cur_el_pt = (char*)SEGMENT_START(_pma, begin);
   while ((*(uint64_t*)cur_el_pt) < mCodeMin)
      cur_el_pt += _pma->elt_size;

   //loop on the first segments (up to one before last)
   for (unsigned int s = begin; s < end - 1; ++s, cur_el_pt = (char*)SEGMENT_START(_pma, s)) {

      for (; cur_el_pt < (char*)SEGMENT_ELT(_pma, s, _pma->elts[s]); cur_el_pt += _pma->elt_size) {
         _apply(*(valuetype*)ELT_TO_CONTENT(cur_el_pt));
         count++;

         if (count >= max) return resolution_t::now() - t_point;
      }
   }

   //loop on last segment
   for (; cur_el_pt < (char*)SEGMENT_ELT(_pma, end - 1, _pma->elts[end - 1]) && *(uint64_t*)cur_el_pt <= mCodeMax; cur_el_pt += _pma->elt_size) {
      _apply(*(valuetype*)ELT_TO_CONTENT(cur_el_pt));
      count++;

      if (count >= max) return resolution_t::now() - t_point;
   }

   return resolution_t::now() - t_point;
}
