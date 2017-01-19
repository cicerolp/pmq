/** @file
 * Benchmark for the query time
 *
 * - Measures the time for inserting each batch
 * - After each batch insertion measures the time to scan the whole array
 *
 */

#include "stde.h"
#include "types.h"

#include "InputIntf.h"

#include "GeoCtnIntf.h"

#include "GeoHash.h"
#include "PMABatchCtn.h"
#include "PostGisCtn.h"
#include "SpatiaLiteCtn.h"
#include "DenseVectorCtn.h"

#define PRINTBENCH( ... ) do { \
   std::cout << "InsertionBench " << container.name() << " ; ";\
   printcsv( __VA_ARGS__ ) ; \
   std::cout << std::endl ;\
} while (0)

uint32_t g_Quadtree_Depth = 25;

// reads the full element
void inline read_element(const valuetype& el) {
   valuetype volatile elemt = *(valuetype*)&el;
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
      container.scan_at_region(region, read_element);
      timer.stop();

      PRINTBENCH("ReadElts", id, timer.milliseconds(), "ms");
   }
}

template <typename container_t>
void run_bench(container_t& container, std::vector<elttype>& input_vec, const int batch_size, const int n_exp) {
   //create container
   container.create((uint32_t)input_vec.size());

   std::vector<elttype>::iterator it_begin = input_vec.begin();
   std::vector<elttype>::iterator it_curr = input_vec.begin();

   duration_t timer;

   int id = 0;
   while (it_begin != input_vec.end()) {
      it_curr = std::min(it_begin + batch_size, input_vec.end());

      std::vector<elttype> batch(it_begin, it_curr);

      // insert batch
      timer = container.insert(batch);

      for (auto& info : timer) {
         PRINTBENCH(info.name, id, info.duration, "ms");
      }

      // update iterator
      it_begin = it_curr;

      // ========================================
      // Performs global scan
      // ========================================

      //Run a scan on the whole array
      run_queries(container, region_t(0, 0, 0, 0, 0), id, n_exp);

      id++;
   }
}

int main(int argc, char* argv[]) {

   cimg_usage("Queries Benchmark inserts elements in batches.");
   const unsigned int nb_elements(cimg_option("-n", 0, "Number of elements to generate randomly"));
   const long seed(cimg_option("-r", 0, "Random seed to generate elements"));
   const int batch_size(cimg_option("-b", 10, "Batch size used in batched insertions"));
   std::string fname(cimg_option("-f", "./data/tweet100.dat", "file with tweets to load"));
   const unsigned int n_exp(cimg_option("-x", 1, "Number of repetitions of each experiment"));

   PMABatchCtn container0(argc, argv);

   /*DenseCtnStdSort container1;
   DenseCtnTimSort container2;
   SpatiaLiteCtn container3;
   PostGisCtn container4;*/

   GeoHashSequential container5(argc, argv);
   GeoHashBinary container6(argc, argv);

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


   run_bench(container0, input_vec, batch_size, n_exp);
   run_bench(container5, input_vec, batch_size, n_exp);
   run_bench(container6, input_vec, batch_size, n_exp);

   /*run_bench(container1, input_vec, batch_size, n_exp);
   run_bench(container2, input_vec, batch_size, n_exp);
   run_bench(container3, input_vec, batch_size, n_exp);
   run_bench(container4, input_vec, batch_size, n_exp);*/

   return EXIT_SUCCESS;

}
