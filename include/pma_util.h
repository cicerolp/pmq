#pragma once

#include "pma/pma.h"
#include "pma/utils/debugMacros.h"
#include "pma/utils/test_utils.h"
#include "pma/utils/benchmark_utils.h"


extern uint32_t g_Quadtree_Depth;

/**
 * @brief count_elts_pma Returns the amount of valid elements between range [beg,end[
 * @param pma A pma data structure
 * @param beg The pointer where want to start counting elements.
 * @param end
 *
 * @note This function was made to be used by the quadtree to query the amount of elements in a range.
 *
 * usually *beg will point to the position of first occurence of a Key in the pma and *end will point to the position of its last occurence.
 *
 * @return
 */
#if DEPRECATED
inline int count_elts_pma(struct pma_struct* pma, char* beg , char* end){
   unsigned int seg_beg = (beg - (char*) pma->array)/(pma->cap_segments * pma->elt_size);
   unsigned int seg_end = (end - 1 - (char* ) pma->array)/(pma->cap_segments * pma->elt_size);

   unsigned int cnt = 0;

   for (unsigned int s = seg_beg ; s <= seg_end; s ++ ){
      cnt += pma->elts[s] ;
   }

   // need to subtract the extra elements at the end a start of each segmend
   cnt -= (beg - (char*) SEGMENT_START(pma,seg_beg)) / pma->elt_size;
   cnt -= ((char*) SEGMENT_ELT(pma,seg_end,pma->elts[seg_end]) - end) / pma->elt_size;
   return cnt;
}
#endif
/**
 * @brief count_elts_pma find in [sbeg, send[ all the elements starting with mcode at level z.
 * @param pma
 * @param sbeg
 * @param send
 * @param mcode the morton code of a quadtree at level z
 * @param z
 *
 * Counts the number elements in the PMA that are contained in the subtree (quad-tree) of depth z and morton code \a mCode;
 *
 * @return
 */
inline int count_elts_pma(struct pma_struct* pma, unsigned int  seg_beg , unsigned int  seg_end, uint64_t mCode, int z){

   unsigned int cnt = 0;
   int diffDepth = g_Quadtree_Depth - z;
   uint64_t mCodeMin = mCode << 2*(diffDepth);
   uint64_t mCodeMax = mCodeMin | ((uint64_t) ~0  >> (64-2*diffDepth) );

   for (unsigned int s = seg_beg ; s < seg_end; s ++ ){
      cnt += pma->elts[s] ;
   }

   //subtract extra elements for first segment
   for (char* el_pt = (char* ) SEGMENT_START(pma,seg_beg) ; (*(uint64_t*) el_pt) < mCodeMin  ; el_pt += pma->elt_size){
       cnt--;
   }

   //subtract extra elements for the last segment
   for (char* el_pt = (char* ) SEGMENT_ELT(pma,seg_end-1,pma->elts[seg_end-1]-1) ; (*(uint64_t*) el_pt) > mCodeMax ; el_pt -= pma->elt_size){
       cnt--;
   }


   return cnt;
}

