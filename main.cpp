#include "stde.h"

//#include "PmaConfig.h"
#include "pma/pma.h"
//#include "pma/pma_priv.h"
//#include "pma/utils/pma_utils.h"
//#include "pma/utils/bitmask.h"
//#include "pma/utils/coherency.h"
#include "pma/utils/debugMacros.h"
#include "pma/utils/test_utils.h"
#include "pma/utils/benchmark_utils.h"

#include "ext/CImg/CImg.h"

#include "Server.h"
#include "SpatialElement.h"
#include "DMPLoader/dmploader.hpp"


uint32_t g_Quadtree_Depth = 25;



#ifdef PMA_TRACE_MOVE
  extern unsigned int g_iteration_counter;
#endif


typedef tweet_t valuetype;
struct elttype{
   uint64_t key;
   valuetype value;
   // Pma uses only the key to sort elements.
   friend inline bool operator==(const elttype& lhs, const elttype& rhs){ return (lhs.key == rhs.key); }
   friend inline bool operator!=(const elttype& lhs, const elttype& rhs){ return !(lhs == rhs); }
   friend inline bool operator<(const elttype& lhs, const elttype& rhs){ return (lhs.key < rhs.key); }

   friend std::ostream& operator<<(std::ostream &out, const elttype& e)  { return out << e.key; } //output

};

std::ostream& operator<<(std::ostream& out, const tweet_t& e)
{
   return out << e.time ;
}

void insert_batch(struct pma_struct* pma, elttype* batch, int size)
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
uint64_t spatialKey(tweet_t& t, int depth){
  uint32_t y = mercator_util::lat2tiley(t.latitude, depth);
  uint32_t x = mercator_util::lon2tilex(t.longitude, depth);
  return mortonEncode_RAM(x,y);
}


/**
 * @brief update_map scans the pma and update the start and end poiters for each key in the pma.
 * @param pma
 * @param range The start and end o
 * @return returns the number of keys that had their start of their range modified.
 *
 * Note this doesn't work if a key was deleted from the pma.
 */
int update_map(struct pma_struct* pma, map_t &range){
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

/**
 * @brief count_elts_pma Returns the amount of valid elements between range [beg,end[
 * @param pma
 * @param beg
 * @param end
 * @return
 */
int count_elts_pma(struct pma_struct* pma, char* beg , char* end){
    /* - get the starting segment
     * - get the ending segment
     */

    unsigned int seg_beg = (beg - (char*) pma->array)/(pma->cap_segments * pma->elt_size);
    unsigned int seg_end = (end - 1 - (char* ) pma->array)/(pma->cap_segments * pma->elt_size);

    std::cout << "begin " << seg_beg << " ; end " << seg_end << std::endl;

//    _START_ beg / pma->cap_segments

    unsigned int cnt = 0;

    for (unsigned int s = seg_beg ; s <= seg_end; s ++ ){
        cnt += pma->elts[s] ;
    }

    // need to subtract the extra elements at the end a start of each segmend
    cnt -= (beg - (char*) SEGMENT_START(pma,seg_beg)) / pma->elt_size;
    cnt -= ((char*) SEGMENT_ELT(pma,seg_end,pma->elts[seg_end]) - end) / pma->elt_size;
    return cnt;
}

int main(int argc, char *argv[]) {   

   bool server = true;
   Server::server_opts nds_opts;
   nds_opts.port = 7001;
   nds_opts.cache = false;
   nds_opts.multithreading = true;

   std::cout << "Server Options:" << std::endl;
   std::cout << "\tOn/Off: " << server << std::endl;
   std::cout << "\t" << nds_opts << std::endl;

   // http server
   std::unique_ptr<std::thread> server_ptr;
   if (server) server_ptr = std::make_unique<std::thread>(Server::run, nds_opts);

   if (server_ptr) {
      std::cout << "Server Running... press any key to terminate." << std::endl;
      getchar();

      Server::getInstance().stop();
      server_ptr->join();
   }

   return 0;
}
