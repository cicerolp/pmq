#pragma once
#include "stde.h"

#include "Query.h"
#include "Singleton.h"

#include "SpatialElement.h"

#include "ext/CImg/CImg.h"
#include "DMPLoader/dmploader.hpp"


typedef tweet_t valuetype;

struct elttype {
   uint64_t key;
   valuetype value;
   // Pma uses only the key to sort elements.
   friend inline bool operator==(const elttype& lhs, const elttype& rhs) { 
      return (lhs.key == rhs.key); 
   }
   friend inline bool operator!=(const elttype& lhs, const elttype& rhs) { 
      return !(lhs == rhs); 
   }
   friend inline bool operator<(const elttype& lhs, const elttype& rhs) { 
      return (lhs.key < rhs.key); 
   }
   friend inline std::ostream& operator<<(std::ostream &out, const elttype& e) {
      return out << e.key; 
   }
};

inline void insert_batch(struct pma_struct* pma, elttype* batch, int size)
{
    simpleTimer t;
    double insertTime = 0; //time to add the batch in the pma
    double inputTime = 0; //time to prepare the batch

    t.start();
    //Inserted batch needs to be sorted already;
    std::sort( batch, batch + size , [](elttype a, elttype b) { return a < b; });
    t.stop();

    PRINTCSVL("Batch sort", t.miliseconds(),"ms" );
    inputTime += t.miliseconds();

    /* Inserts the current batch  */
    t.start();
    add_array_elts(pma,(void *)batch, (void *) ((char *)batch + (size)*sizeof(elttype)),comp<uint64_t>);
    t.stop();
    insertTime += t.miliseconds();
    PRINTCSVL("Batch insert", t.miliseconds(),"ms" );

    return;
}

/**
 * @brief spatialKey Computes the the morton-index using the tweets coordinates on quadtree
 * @param t The tweet data structure.
 * @param depth Depth of refinement of the quadtree
 * @return
 */
inline uint64_t spatialKey(tweet_t& t, int depth){
  uint32_t y = mercator_util::lat2tiley(t.latitude, depth);
  uint32_t x = mercator_util::lon2tilex(t.longitude, depth);
  return mortonEncode_RAM(x,y);
}


/**
 * @brief update_map scans the pma and update the start and end poiters for each key in the pma.
 * @param pma [IN]
 * @param range [OUT] Will be filled with the elements containing ( Key, begin_in_pma, end_in_pma) indicating where is located the range of Key in the pma array;
 * @return returns the number of keys that had their start of their range modified.
 *
 * Note this doesn't work if a key was deleted from the pma.
 */
inline int update_map(struct pma_struct* pma, map_t &range){
   uint64_t last = *(uint64_t*) SEGMENT_START(pma,0);

   int mod_ranges = 0;

   char* el = (char*) SEGMENT_START(pma,0);

   if (range[last].first!=el){
     mod_ranges++;
     range[last].first = el;
   }

   for (int s = 0 ; s < pma->nb_segments; s++){
       for (char* el = (char*) SEGMENT_START(pma,s) ; el < SEGMENT_ELT(pma,s,pma->elts[s]) ; el += pma->elt_size){
           if (last != *(uint64_t*) el){
               range[last].second = el;
               last = *(uint64_t*) el;

               if (range[last].first!=el){
                   mod_ranges++;
                   range[last].first = el;
               }
           }
       }
   }
   range[last].second = (char*) SEGMENT_START(pma,pma->nb_segments - 1 ) + pma->elts[pma->nb_segments - 1] * pma->elt_size;

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

   pma_struct* pma;
   elttype* reference_array;
   
   std::unique_ptr<SpatialElement> quadtree;
};