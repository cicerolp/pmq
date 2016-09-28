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
inline int count_elts_pma(const struct pma_struct* pma, unsigned int  seg_beg , unsigned int  seg_end, uint64_t mCode, int z){

   unsigned int cnt = 0;
   uint64_t mCodeMin = 0;
   uint64_t mCodeMax = 0;
   get_mcode_range(mCode,z,mCodeMin,mCodeMax);

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

/**
 * @brief elts_pma Gets the elements in the \a pma between segments [seg_beg , seg_end[ with prefix equal to \a mCode.
 * @param pma
 * @param seg_beg
 * @param seg_end
 * @param mCode The MortonCode prefix the function will be looking for
 * @param z The depth in the quadtree used to compute the prefix.
 * @param writer Will be filled with the elements retreived
 * @return the amount elements written.
 */
inline int elts_pma(struct pma_struct* pma, unsigned int  seg_beg, unsigned int  seg_end, uint64_t mCode, int z, json_writer& writer, uint32_t &max_cnt) {
   
   unsigned int cnt = 0;

   if (max_cnt == 0) return cnt;

   uint64_t mCodeMin, mCodeMax;
   get_mcode_range(mCode,z,mCodeMin,mCodeMax);

   //Find the first element of the first segment
   char* cur_el_pt = (char* ) SEGMENT_START(pma,seg_beg);
   while ((*(uint64_t*) cur_el_pt) < mCodeMin)
       cur_el_pt += pma->elt_size;

   //loop on the first segments (up to one before last)
   for (unsigned int s = seg_beg ; s < seg_end-1 ; ++s , cur_el_pt = (char*) SEGMENT_START(pma, s)) {

      for ( ; cur_el_pt <  (char*) SEGMENT_ELT(pma,s,pma->elts[s]) ; cur_el_pt += pma->elt_size) {

         if (max_cnt == 0) return cnt;

         //PRINTOUT("%llu \n", *(uint64_t* )cur_el_pt );
         valuetype tweet = *(valuetype*) ELT_TO_CONTENT(cur_el_pt);

         writer.StartArray();
         writer.Uint(tweet.time);
         writer.Uint(tweet.language);
         writer.Uint(tweet.device);
         writer.Uint(tweet.app);
         writer.EndArray();
         cnt++;

         max_cnt--;
      }
   }

   //loop on last segment
   for ( ; cur_el_pt < (char*) SEGMENT_ELT(pma,seg_end-1,pma->elts[seg_end-1]) && *(uint64_t*)cur_el_pt <= mCodeMax ; cur_el_pt += pma->elt_size) {
      
      if (max_cnt == 0) return cnt;
      
      //PRINTOUT("%llu \n", *(uint64_t* )cur_el_pt );
      valuetype tweet = *(valuetype*) ELT_TO_CONTENT(cur_el_pt);

      writer.StartArray();
      writer.Uint(tweet.time);
      writer.Uint(tweet.language);
      writer.Uint(tweet.device);
      writer.Uint(tweet.app);
      writer.EndArray();
      cnt++;

      max_cnt--;
   }

   return cnt;
}
