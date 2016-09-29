#pragma once
#include "stde.h"
#include "types.h"

#include "pma/pma.h"
#include "pma/utils/test_utils.h"
#include "pma/utils/debugMacros.h"
#include "pma/utils/benchmark_utils.h"

extern uint32_t g_Quadtree_Depth;

//Timer used for benchmarks;
using Timer = unixTimer< CLOCK_MONOTONIC >;

/**
 * @brief get_mcode_range : Computes the min and max values for a geo_hash with prefix \a mCode
 * @param mCode : The prefix of the morton code representing a quadtree node.
 * @param z : The depth of the quadtree node (corresponding to this mCode).
 * @param min [OUT]
 * @param max [OUT]
 */
inline void get_mcode_range(uint64_t mCode, int z, uint64_t& min, uint64_t& max){
    int diffDepth = g_Quadtree_Depth - z;
    min = mCode << 2*(diffDepth);
    max = min | ((uint64_t) ~0  >> (64-2*diffDepth) );
    return;
}

inline int iterate_elts_pma(const struct pma_struct* pma, unsigned int  seg_beg, unsigned int  seg_end, uint64_t mCode, int z, int (*fun)(const char*) ) {

   unsigned int cnt = 0;

   uint64_t mCodeMin, mCodeMax;
   get_mcode_range(mCode,z,mCodeMin,mCodeMax);

   //Find the first element of the first segment
   char* cur_el_pt = (char* ) SEGMENT_START(pma,seg_beg);
   while ((*(uint64_t*) cur_el_pt) < mCodeMin)
       cur_el_pt += pma->elt_size;

   //loop on the first segments (up to one before last)
   for (unsigned int s = seg_beg ; s < seg_end-1 ; ++s , cur_el_pt = (char*) SEGMENT_START(pma, s)) {

      for ( ; cur_el_pt <  (char*) SEGMENT_ELT(pma,s,pma->elts[s]) ; cur_el_pt += pma->elt_size) {
         //PRINTOUT("%llu \n", *(uint64_t* )cur_el_pt );
         fun(cur_el_pt);
         cnt++;
      }
   }

   //loop on last segment
   for ( ; cur_el_pt < (char*) SEGMENT_ELT(pma,seg_end-1,pma->elts[seg_end-1]) && *(uint64_t*)cur_el_pt <= mCodeMax ; cur_el_pt += pma->elt_size) {
      //PRINTOUT("%llu \n", *(uint64_t* )cur_el_pt );
      fun(cur_el_pt);
      cnt++;

   }

   return cnt;
}
