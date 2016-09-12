#pragma once
#include "stde.h"

#include "Query.h"
#include "Singleton.h"

#include "SpatialElement.h"

#include "ext/CImg/CImg.h"
#include "DMPLoader/dmploader.hpp"


/**
 * @brief insert_batch
 * @param pma
 * @param batch
 * @param size
 *
 * Will return the windowIds of windows that were rebalanced
 */
inline void insert_batch(struct pma_struct* pma, elttype* batch, int size) {
    simpleTimer t;
    double insertTime = 0; //time to add the batch in the pma
    double inputTime = 0; //time to prepare the batch

    t.start();
    //Inserted batch needs to be sorted already;
    std::sort( batch, batch + size , [](elttype a, elttype b) { return a < b; });
    t.stop();

    //PRINTCSVL("Batch sort", t.miliseconds(),"ms" );
    inputTime += t.miliseconds();

    /* Inserts the current batch  */
    t.start();
    add_array_elts(pma,(void *)batch, (void *) ((char *)batch + (size)*sizeof(elttype)),comp<uint64_t>);
    t.stop();
    insertTime += t.miliseconds();
    //PRINTCSVL("Batch insert", t.miliseconds(),"ms" );

    return;
}

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

class PMAInstance : public Singleton<PMAInstance> {
	friend class Singleton<PMAInstance>;
public:
	bool create(int argc, char *argv[]);
   void destroy();

	std::string query(const Query& query);

private:
   PMAInstance() = default;
	virtual ~PMAInstance() = default;

	bool _ready{ false };

   std::mutex mutex;

   pma_struct* pma;   
   std::unique_ptr<SpatialElement> quadtree;
};
