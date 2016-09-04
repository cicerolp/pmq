#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <vector>
#include <string.h>
#include <time.h>
#include <algorithm>
#include <map>

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

#include "include/DMPLoader/dmploader.hpp"
#include "include/mercator_util.h"
#include "include/morton.h"

#ifdef __APPLE__
#include "mac_utils.h"
#endif



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

uint64_t spatialKey(tweet_t& t){
  uint32_t y = mercator_util::lat2tiley(t.latitude, 25);
  uint32_t x = mercator_util::lon2tilex(t.longitude, 25);
  return mortonEncode_RAM(x,y);
}


int main(int argc, char *argv[])
{

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

   /* Create <key,value> elements */

   std::vector<elttype> input_vec;
   input_vec.reserve(tweet_vec.size());

   //use the spatial index as key
   for (auto& tweet : tweet_vec){
       elttype e;
       e.key = spatialKey(tweet);
       e.value = tweet;
       input_vec.emplace_back(e);
   }

   reference_array = (elttype * ) malloc( (input_vec.size()) *sizeof(elttype));
   memcpy(reference_array, &input_vec[0], input_vec.size() * sizeof(elttype));
   qsort(reference_array, input_vec.size(), sizeof(elttype), comp<uint64_t>);


   int nb_elements = input_vec.size();
   PRINTOUT("Number of elements == %d \n",nb_elements);

   struct pma_struct * pma = (struct pma_struct * ) build_pma(nb_elements,sizeof(valuetype), tau_0, tau_h, rho_0, rho_h, seg_size);

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
   }

//   print_pma_keys(pma);
//   std::cout << "\n";

   /* Creates a map with begin and end of each index in the pma. */ .
   std::map<int64_t,std::pair<char*,char*> > range;
   uint64_t last = *(uint64_t*) SEGMENT_START(pma,0);
   range[last].first = (char*) SEGMENT_START(pma,0);
   for (int s = 0 ; s < pma->nb_segments; s++){
       for (char* el = (char*) SEGMENT_START(pma,s) ; el < SEGMENT_ELT(pma,s,pma->elts[s]) ; el += pma->elt_size){
           if (last != *(uint64_t*) el){
               range[last].second = el;
               last = *(uint64_t*) el;
               range[last].first = el;
           }
       }
   }
   range[last].second = (char*) SEGMENT_START(pma,pma->nb_segments - 1 ) + pma->elts[pma->nb_segments - 1] * pma->elt_size;

   printf("Size of map %d \n",range.size());

   for (auto &e : range){
       printf("%llu : [%p - %p] : %d \n" , e.first , e.second.first, e.second.second , (e.second.second - e.second.first) / pma->elt_size);
   }

   destroy_pma(pma);
   free(reference_array);

   return 0;
}
