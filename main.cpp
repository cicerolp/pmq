#include "stde.h"

#include "pma/pma.h"
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
   nds_opts.port = 7000;
   nds_opts.cache = false;
   nds_opts.multithreading = true;

   std::cout << "Server Options:" << std::endl;
   std::cout << "\tOn/Off: " << server << std::endl;
   std::cout << "\t" << nds_opts << std::endl;

   // http server
   std::unique_ptr<std::thread> server_ptr;
   if (server) server_ptr = std::make_unique<std::thread>(Server::run, nds_opts);

   cimg_usage("Benchmark inserts elements in batches.");
   //const unsigned int nb_elements ( cimg_option("-n",100,"Number of elements to insert"));
   const unsigned int seg_size ( cimg_option("-s",8,"Segment size for the pma"));
   const int batch_size ( cimg_option("-b",10,"Batch size used in batched insertions"));
   const float tau_0 ( cimg_option("-t0",0.92,"pma parameter tau_0"));
   const float tau_h ( cimg_option("-th",0.7,"pma parameter tau_h"));
   const float rho_0 ( cimg_option("-r0",0.08,"pma parameter rho_0"));
   const float rho_h ( cimg_option("-rh",0.3,"pma parameter rho_0"));
   std::string fname ( cimg_option("-f","../../data/twitter/tweet1000.dat","file with tweets"));

   const char* is_help = cimg_option("-h",(char*)0,0);

   if (is_help) return 0;

   std::cout << "Input file: " << fname << std::endl;

   elttype *reference_array;

   std::vector<tweet_t> tweet_vec;
   loadTweetFile(tweet_vec,fname);

   // Create <key,value> elements

   std::vector<elttype> input_vec;
   input_vec.reserve(tweet_vec.size());

   //use the spatial index as key
   for (auto& tweet : tweet_vec){
       elttype e;
       e.key = spatialKey(tweet,g_Quadtree_Depth);
       e.value = tweet;
       input_vec.emplace_back(e);
   }

   reference_array = (elttype * ) malloc( (input_vec.size()) *sizeof(elttype));
   memcpy(reference_array, &input_vec[0], input_vec.size() * sizeof(elttype));
   qsort(reference_array, input_vec.size(), sizeof(elttype), comp<uint64_t>);


   int nb_elements = input_vec.size();
   PRINTOUT("Number of elements == %d \n",nb_elements);

   struct pma_struct * pma = (struct pma_struct * ) build_pma(nb_elements,sizeof(valuetype), tau_0, tau_h, rho_0, rho_h, seg_size);

   // Creates a map with begin and end of each index in the pma.
   map_t range;
   SpatialElement quadtree(spatial_t(0,0,0));

   elttype * batch_start;
   int size = nb_elements / batch_size;
   int num_batches = 1 + (nb_elements-1)/batch_size;

   for (int k = 0; k < num_batches; k++)
   {
       batch_start = &input_vec[k*size];

       if ((nb_elements-k*batch_size) / batch_size == 0){
           size = nb_elements % batch_size;
       }else{
           size = batch_size;
       }
       insert_batch(pma,batch_start,batch_size);
       int count = update_map(pma,range);
     // printf("Size of map %d ; updated %d \n",range.size(),count);


       quadtree.update(range);

       // print the updated ranges:
     //  for (int r ; r < batch_size; r++){
           //printf("%d batch_start[r].key;
      //     printf("%llu : [%p - %p] : %d \n" , e.first , e.second.first, e.second.second , (e.second.second - e.second.first) / pma->elt_size);
      // }
   }

   std::cout << " Element in the root node [" << (void*) quadtree.beg << " : " << (void*) quadtree.end << "] " << count_elts_pma(pma,quadtree.beg,quadtree.end) <<  std::endl ;

   // A stupid test :
   SpatialElement* ptr = &quadtree;
   //goes a somewhere deep in the tree
   for (int d = 0 ; d < 2 ; d++){
       int k = 0 ;
       while(ptr->_container[k] == NULL)
           k++;
       ptr = (ptr->_container[k]).get();
   }

   //print the counts on each of child quadrants
   std::cout << " Element in the parent range [" << (void*) ptr->beg << " : " << (void*) ptr->end << "] " << count_elts_pma(pma,ptr->beg,ptr->end) <<  std::endl ;
   for (int i = 0 ; i < 4 ; i++){
    if (ptr->_container[i] != NULL){
       std::cout << " Element in the quadrant " << i << " = " <<
                    (void*) (ptr->_container[i]->beg) << " : " <<
                    (void*) (ptr->_container[i]->end) << "] " <<
                    count_elts_pma(pma,ptr->_container[i]->beg,ptr->_container[i]->end) <<
                    std::endl ;
    }
   }


   if (server_ptr) {
      std::cout << "Server Running... press any key to terminate." << std::endl;
      getchar();

      Server::getInstance().stop();
      server_ptr->join();
   }


//   print_pma_keys(pma);
//   std::cout << "\n";
/*
   for (auto &e : range){
       //printf("%llu : [%p - %p] : %d \n" , e.first , e.second.first, e.second.second , (e.second.second - e.second.first) / pma->elt_size);
       std::cout << e.first << std::endl;
   }
*/
   destroy_pma(pma);
   free(reference_array);

   return 0;
}
