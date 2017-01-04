/** @file
 * Micro Benchmark for PMQ operations.
 *
 * - Insertion of a batch
 * - DIFF of modified Keys
 * - UPDATE of the Quadtree.
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

template <typename container_t>
void run_bench(container_t& container, std::vector<elttype>& input_vec, const int batch_size) {
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

      id++;
   }
}

int main(int argc, char* argv[]) {

   cimg_usage("Benchmark inserts elements in batches.");
   const int batch_size(cimg_option("-b", 10, "Batch size used in batched insertions"));
   std::string fname(cimg_option("-f", "./data/tweet100.dat", "file with tweets"));
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

   PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());
   std::vector<elttype> input_vec = input::load(fname, quadtree_depth);
   PRINTOUT(" %d teewts loaded \n", (uint32_t)input_vec.size());

   for (uint32_t i = 0; i < n_exp; i++) {
      run_bench(container0, input_vec, batch_size);
      run_bench(container5, input_vec, batch_size);
      run_bench(container6, input_vec, batch_size);

      /*run_bench(container1, input_vec, batch_size);
      run_bench(container2, input_vec, batch_size);
      run_bench(container3, input_vec, batch_size);
      run_bench(container4, input_vec, batch_size);*/
   }
}
