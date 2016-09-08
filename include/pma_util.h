#pragma once

#include "pma/pma.h"
#include "pma/utils/debugMacros.h"
#include "pma/utils/test_utils.h"
#include "pma/utils/benchmark_utils.h"

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

