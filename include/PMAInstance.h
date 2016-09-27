#pragma once
#include "stde.h"

#include "Query.h"
#include "Singleton.h"

#include "SpatialElement.h"

#include "ext/CImg/CImg.h"
#include "DMPLoader/dmploader.hpp"

#undef Bool

/**
 * @brief insert_batch
 * @param pma
 * @param batch
 * @param size
 *
 * Will return the windowIds of windows that were rebalanced
 */
inline void insert_batch(struct pma_struct* pma, elttype* batch, int size) {
    Timer t;
    double insertTime = 0; //time to add the batch in the pma
    double inputTime = 0; //time to prepare the batch
#ifndef NDEBUG
    static unsigned int call_counter = 0;
#endif
    t.start();
    //Inserted batch needs to be sorted already;
    std::sort( batch, batch + size , [](elttype a, elttype b) { return a < b; });
    t.stop();

    //PRINTCSVL("Batch sort", t.miliseconds(),"ms" );
    inputTime += t.milliseconds();

    /* Inserts the current batch  */
    t.start();
    pma::batch::add_array_elts(pma,(void *)batch, (void *) ((char *)batch + (size)*sizeof(elttype)),comp<uint64_t>);
    t.stop();
    insertTime += t.milliseconds();
#ifndef NDEBUG
    PRINTCSVL("Batch insert", t.milliseconds(),"ms", call_counter++);
#else
    PRINTCSVL("Batch insert", t.milliseconds(),"ms");
#endif
    return;
}

#if DEPRECATED
/**
 * @brief update_map scans the pma and update the start and end poiters for each key in the pma.
 * @param pma [IN]
 * @param range [OUT] Will be filled with the elements containing ( Key, begin_in_pma, end_in_pma) indicating where is located the range of Key in the pma array;
 * @return returns the number of keys that had their start of their range modified.
 *
 * Note this doesn't work if a key was deleted from the pma.
 */
// TODO : INPUT range with (wId);
// OUTPUT: range with seg_beg and seg_end;
// DONT return when seg_beg == seg_end;
inline int update_map(struct pma_struct* pma, map_t &range){
   int mod_ranges = 0;
   char* el = (char*) SEGMENT_START(pma,0);
   uint64_t last = *(uint64_t*) SEGMENT_START(pma,0); //mcode

   //range.emplace_back(mcode, seg_beg , seg_end);
   range.emplace_back(last, el, nullptr);   
   mod_ranges++;

   // TODO Scan the window to find mCodes in it;
   for (int s = 0 ; s < pma->nb_segments; s++){
      for (el = (char*) SEGMENT_START(pma,s) ; el < SEGMENT_ELT(pma,s,pma->elts[s]) ; el += pma->elt_size){

         if (last != (*(uint64_t*) el)) {
            range.back().end = el;

            last = *(uint64_t*) el;

            range.emplace_back(last, el, nullptr);        
            mod_ranges++;
         }
      }
   }
   
   range.back().end = (char*) SEGMENT_START(pma,pma->nb_segments - 1 ) + pma->elts[pma->nb_segments - 1] * pma->elt_size;
   
   return mod_ranges;
}
#endif
/**
 * @brief pma_diff Scans the segments modified on last rebalance operation and returns the segmentID of the elements in these windows;
 * @param pma
 * @param modified [OUT] a vector containing (KEY, START_SEG, END_SEG) where [ START_SED, END_SED [ is the interval os segments that contains key \a KEY
 *
 * for each wId modified on in pma:
 * - get wId.beg and wId.end
 * - modified <- (mcode, wId.beg, wId.end)
 *
 * @return the number of KEYS in vector.
 */
inline int pma_diff(struct pma_struct* pma, map_t &modified){

    int mod_ranges = 0;

    for (unsigned int wId : *(pma->last_rebalanced_segs) ){
        //We don't need to track rebalance on leaf segments (TODO : could remove it from the PMA rebalance function)
        //  if (wId < pma->nb_segments)
        //    continue;

        unsigned int sStart =  pma_get_window_start(pma,wId); //Get first segment of the window

        char* el_pt = (char*) SEGMENT_START(pma,sStart); //get first element of the segment
        uint64_t lastElKey = *(uint64_t*) el_pt; //mcode of the first element
        modified.emplace_back( lastElKey , sStart, sStart);  //save the start for this key. Initialy end = begin+1 (open interval)
        mod_ranges++;

        // loop over the segments of the current window
        unsigned int s ;
        for (s = sStart ; s < sStart + pma_get_window_size(pma,wId); s++){
            el_pt = (char*) SEGMENT_START(pma,s);

            //If we changed segment but the lastkey is also in first postition of this segment, we incremente its "end" index;
            if (lastElKey == (*(uint64_t*) el_pt)){
                modified.back().end ++ ;
            }

            for (el_pt ; el_pt < SEGMENT_ELT(pma,s,pma->elts[s]) ; el_pt += pma->elt_size){

                if (lastElKey != (*(uint64_t*) el_pt)) {

                    lastElKey = *(uint64_t*) el_pt;

                    modified.emplace_back(lastElKey, s, s+1);
                    mod_ranges++;
                }
            }

        }

        //if the last lastKey continues outside of this range of windows we remove it;
        //        modified.pop_back();

        //ACTUALLY I THINK WE STILL NEED IT; if the begin of the last range was modified we still need to inform to the quadtree.

        /* we still need to find the end of last element (can be out of the current window) */
        //initialize 'end' with the last segment of the pma + 1
        modified.back().end = pma->nb_segments;
        for (char* seg_pt = (char* ) SEGMENT_START(pma,s) ; seg_pt < SEGMENT_START(pma, pma->nb_segments) ; seg_pt += (pma->cap_segments * pma->elt_size)){
            // Check the first key of the following segments until we find one that differs;
            if (lastElKey != *(uint64_t*) seg_pt ){
                modified.back().end = (seg_pt - (char*) pma->array) / (pma->cap_segments*pma->elt_size); //gets they index of segment that differs;
                break;
            }
        }
    }

    return mod_ranges;
}

class PMAInstance : public Singleton<PMAInstance> {
	friend class Singleton<PMAInstance>;
public:
	bool create(int argc, char *argv[]);
   void destroy();

   /**
     * @brief query
     * @param query  "/rest/query/region/1/0/0/1/1"
     *
     * /zoom/x0/y0/x1/y1/
     *
     */
	std::string query(const Query& query);
   std::string update();

//private:
   PMAInstance() = default;
	virtual ~PMAInstance() = default;

   std::mutex mutex;
   bool up_to_date {true}; //What is this for ?

   pma_struct* pma;   
   std::unique_ptr<SpatialElement> quadtree;
};
