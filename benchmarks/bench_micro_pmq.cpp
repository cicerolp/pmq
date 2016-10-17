/** @file
 * Micro Benchmark for PMQ operations.
 *
 * - Insertion of a batch
 * - DIFF of modified Keys
 * - UPDATE of the Quadtree.
 *
 */

#include <typeinfo>

#include "stde.h"
#include "types.h"

#include "InputIntf.h"

#include "GeoCtnIntf.h"

#include "PMABatchCtn.h"
#include "PostGisCtn.h"
#include "SpatiaLiteCtn.h"
#include "DenseVectorCtn.h"

uint32_t g_Quadtree_Depth = 25;

template <typename T>
struct type {
   static constexpr const char* name() { return "unknown"; } // end type< T>::name
}; // type< T>

template <>
struct type<PMABatchCtn> {
   static constexpr const char* name() { return "PMABatch"; }
};

template <>
struct type<SpatiaLiteCtn> {
   static constexpr const char* name() { return "StdDense"; }
};

template <>
struct type<DenseVectorCtn> {
   static constexpr const char* name() { return "TimDense"; }
};

template <typename container_t>
void run_bench(container_t& container, std::vector<elttype>& input_vec, const int batch_size) {
#define PRINTBENCH( ... ) do { \
   std::cout << "InsertionBench " << type<container_t>::name() << " ; ";\
   printcsv( __VA_ARGS__ ) ; \
   std::cout << std::endl ;\
} while (0)

   //create the pma
   container.create(input_vec.size());

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
   DenseCtnStdSort container1;
   DenseCtnTimSort container2;

   const char* is_help = cimg_option("-h", (char*)0, 0);
   if (is_help) return false;

   const uint32_t quadtree_depth = 25;

   PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());
   std::vector<elttype> input_vec = input::load(fname, quadtree_depth);
   PRINTOUT(" %d teewts loaded \n", (uint32_t)input_vec.size());

   for (int i = 0; i < n_exp; i++) {
      run_bench(container0, input_vec, batch_size);
      run_bench(container1, input_vec, batch_size);
      run_bench(container2, input_vec, batch_size);
   }

   /*PMABatch pma_container(argc, argv); //read pma command line parameters
   DenseVectorStdSort vec_cont;
   DenseVectorTimSort tim_cont;

   const char* is_help = cimg_option("-h", (char*)0, 0);

   if (is_help) return false;

   const uint32_t quadtree_depth = 25;

   PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());
   std::vector<elttype> input_vec = input::load(fname, quadtree_depth);
   PRINTOUT(" %d teewts loaded \n", (uint32_t)input_vec.size());

   for (int i = 0; i < n_exp; i++) {
      run_bench(pma_container, input_vec, batch_size);
      run_bench(vec_cont, input_vec, batch_size);
      run_bench(tim_cont, input_vec, batch_size);
#if 0
     do_bench_benderPMA(&input_vec[0],input_vec.size(),reference_array,tau_0,tau_h,rho_0,rho_h,seg_size);
     do_bench_stlsort(&input_vec[0],input_vec.size(),batch_size,reference_array);
     do_bench_qsort(&input_vec[0],input_vec.size(),batch_size,reference_array);
     do_bench_mergesort(&input_vec[0],input_vec.size(),batch_size,reference_array);
#endif
   }*/
}
