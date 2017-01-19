/** @file
 * 1: Inserts a batch and deletes with a remove function
 * 2: Scan the whole pma.
 *
 */

#include "stde.h"
#include "types.h"

#include "InputIntf.h"
#include "GeoCtnIntf.h"

#include "GeoHash.h"
#include "PMABatchCtn.h"
#include "DenseVectorCtn.h"

#define PRINTBENCH( ... ) do { \
   std::cout << "InsertionRemoveBench " << container.name() << " ; ";\
   printcsv( __VA_ARGS__ ) ; \
   std::cout << std::endl ;\
} while (0)

uint32_t g_Quadtree_Depth = 25;

// reads the full element
void inline read_element(const valuetype& el) {
   valuetype volatile elemt = *(valuetype*)&el;
}


void inline print_values(const valuetype& el) {
   valuetype volatile elemt = *(valuetype*)&el;
   std::cout << elemt.time << " " ;
}

template <typename container_t>
void inline run_queries(container_t& container, const region_t& region, const int id, const int n_exp) {

   Timer timer;

   // 1 - gets the minimum set of nodes that are inside the queried region
   // QueryRegion will traverse the tree and return the intervals to query;
   // NOTE: when comparing with the quadtree with pointer to elements the scan will be the traversall on the tree.

   // warm up
   container.scan_at_region(region, read_element);

   // access the container to count the number of elements inside the region
   for (int i = 0; i < n_exp; i++) {
      timer.start();
      container.scan_at_region(region, print_values);
      timer.stop();

      std::cout << "\n";

      PRINTBENCH("ReadElts", id, timer.milliseconds(), "ms");
   }
}

template <typename container_t>
void run_bench(container_t& container, std::vector<elttype>& input_vec, const int batch_size, const int n_exp, uint64_t rm_time_limit) {
   //create container
   container.create((uint32_t)input_vec.size());

   std::vector<elttype>::iterator it_begin = input_vec.begin();
   std::vector<elttype>::iterator it_curr = input_vec.begin();

   duration_t timer;

   uint64_t t = 0;
   uint64_t oldest_time  = 0;
   while (it_begin != input_vec.end()) {
      it_curr = std::min(it_begin + batch_size, input_vec.end());

      std::vector<elttype> batch(it_begin, it_curr);

      // insert batch
      if (t > rm_time_limit){
         oldest_time++;
      }

      DBG_PRINTOUT("Removing with oldest time %u\n",oldest_time);

      timer = container.insert_rm(batch, [ oldest_time ]( const void* el) {
          return ((elttype*)el)->value.time < oldest_time;
      });

      for (auto& info : timer) {
         PRINTBENCH(info.name, t, info.duration, "ms");
      }

      // update iterator
      it_begin = it_curr;

      // ========================================
      // Performs global scan
      // ========================================

      //Run a scan on the whole array
      DBG_PRINTOUT("Scanning container:\n");
      run_queries(container, region_t(0, 0, 0, 0, 0), t, n_exp);

      DBG_PRINTOUT("\n\n===================================================\n \n");
      t++;
   }
}

int main(int argc, char* argv[]) {

   cimg_usage("Queries Benchmark inserts elements in batches.");
   const unsigned int nb_elements(cimg_option("-n", 0, "Number of elements to generate randomly"));
   const long seed(cimg_option("-r", 0, "Random seed to generate elements"));
   const int batch_size(cimg_option("-b", 10, "Batch size used in batched insertions"));
   const int rm_time(cimg_option("-rm", 10, "The 'time' difference used to delete tweets"));
   std::string fname(cimg_option("-f", "./data/tweet100.dat", "file with tweets to load"));
   const unsigned int n_exp(cimg_option("-x", 1, "Number of repetitions of each experiment"));

   GeoHashSequential container5(argc, argv);

   const char* is_help = cimg_option("-h", (char*)0, 0);
   if (is_help) return false;

   const uint32_t quadtree_depth = 25;

   std::vector<elttype> input_vec;

   if (nb_elements == 0) {
      PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());
      input_vec = input::load(fname, quadtree_depth);
      PRINTOUT("%d teewts loaded \n", (uint32_t)input_vec.size());
   } else {
      PRINTOUT("Generate random keys...\n");
      //Use the batch id as timestamp
      input_vec = input::dist_random(nb_elements, seed, batch_size);
      PRINTOUT("%d teewts generated \n", (uint32_t)input_vec.size());
   }
#ifndef NDEBUG
   for (elttype & e : input_vec){
       std::cout << "[" << e.key << "," << e.value.time << "] \n" ;
   }

#endif


   run_bench(container5, input_vec, batch_size, n_exp, rm_time );

   return EXIT_SUCCESS;

}
