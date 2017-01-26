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
#include "string_util.h"

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

#define PRINTBENCH_PTR( ... ) do { \
   std::cout << "InsertionBench " << container->name() << " ; ";\
   printcsv( __VA_ARGS__ ) ; \
   std::cout << std::endl ;\
} while (0)

/*#define PRINTBENCH( ... ) do { \
} while (0)*/

struct bench_t {
   // benchmark parameters
   uint32_t n_exp;
   uint32_t batch_size;
};

uint32_t g_Quadtree_Depth = 25;

// reads the full element
void inline read_element(const valuetype& el) {
   valuetype volatile elemt = *(valuetype*)&el;
}

template <typename T>
void inline run_queries(T& container, const region_t& region, uint32_t id, const bench_t& parameters) {

   Timer timer;

   // 1 - gets the minimum set of nodes that are inside the queried region
   // QueryRegion will traverse the tree and return the intervals to query;
   // NOTE: when comparing with the quadtree with pointer to elements the scan will be the traversall on the tree.

   // warm up
   container.scan_at_region(region, read_element);

   // access the container to count the number of elements inside the region
   for (uint32_t i = 0; i < parameters.n_exp; i++) {
      timer.start();
      container.scan_at_region(region, read_element);
      timer.stop();

      PRINTBENCH("ReadElts", id, timer.milliseconds(), "ms");
   }
}

template <typename T>
void run_bench(int argc, char* argv[], const std::vector<elttype>& input, const bench_t& parameters) {
   //create container
   std::unique_ptr<T> container = std::make_unique<T>(argc, argv);
   container->create((uint32_t)input.size());

   std::vector<elttype>::const_iterator it_begin = input.begin();
   std::vector<elttype>::const_iterator it_curr = input.begin();

   duration_t timer;

   int id = 0;
   while (it_begin != input.end()) {
      it_curr = std::min(it_begin + parameters.batch_size, input.end());

      std::vector<elttype> batch(it_begin, it_curr);

      // insert batch
      timer = container->insert(batch);

      for (auto& info : timer) {
         PRINTBENCH_PTR(info.name, id, info.duration, "ms");
      }

      // update iterator
      it_begin = it_curr;

      // ========================================
      // Performs global scan
      // ========================================

      //Run a scan on the whole array
      run_queries((*container.get()), region_t(0, 0, 0, 0, 0), id, parameters);

      ++id;
   }
}

int main(int argc, char* argv[]) {
   bench_t parameters;

   cimg_usage("Queries Benchmark inserts elements in batches.");

   const unsigned int nb_elements(cimg_option("-n", 0, "Number of elements to generate randomly"));
   const long seed(cimg_option("-r", 0, "Random seed to generate elements"));

   std::string fname(cimg_option("-f", "./data/tweet100.dat", "file with tweets to load"));

   parameters.batch_size = (cimg_option("-b", 100, "Batch size used in batched insertions"));
   parameters.n_exp = (cimg_option("-x", 1, "Number of repetitions of each experiment"));

   const char* is_help = cimg_option("-h", (char*)0, 0);
   if (is_help) return false;

   const uint32_t quadtree_depth = 25;

   std::vector<elttype> input;

   if (nb_elements == 0) {
      PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());
      input = input::load(fname, quadtree_depth);
      PRINTOUT("%d teewts loaded \n", (uint32_t)input.size());
   } else {
      PRINTOUT("Generate random keys...\n");
      //Use the batch id as timestamp
      input = input::dist_random(nb_elements, seed, parameters.batch_size);
      PRINTOUT("%d teewts generated \n", (uint32_t)input.size());
   }
#ifndef NDEBUG
   for (elttype& e : input) {
      std::cout << "[" << e.key << "," << e.value.time << "] \n";
   }
#endif

   run_bench<PMABatchCtn>(argc, argv, input, parameters);
   run_bench<GeoHashSequential>(argc, argv, input, parameters);
   run_bench<GeoHashBinary>(argc, argv, input, parameters);
   run_bench<DenseCtnStdSort>(argc, argv, input, parameters);
   run_bench<DenseCtnTimSort>(argc, argv, input, parameters);
   run_bench<SpatiaLiteCtn>(argc, argv, input, parameters);
   run_bench<PostGisCtn>(argc, argv, input, parameters);

   return EXIT_SUCCESS;

}
