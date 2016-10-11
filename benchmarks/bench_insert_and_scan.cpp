/** @file
 * Benchmark for the queru time
 *
 *
 */

#include "stde.h"
#include "types.h"
#include "PMABatch.h"
#include "DenseVector.h"

#include "InputIntf.h"
#include "QuadtreeIntf.h"


#define PRINTBENCH( ... ) do { \
   std::cout << type<container_t>::name() << " ; ";\
   printcsv( __VA_ARGS__ ) ; \
   std::cout << std::endl ;\
} while (0)

uint32_t g_Quadtree_Depth = 25;

unsigned int g_it_id = 0;

template< typename T> struct type {
   static constexpr const char* name() { return "unknown";  }  // end type< T>::name
}; // type< T>

template<> struct type<PMABatch> {
   static constexpr const char* name() { return "PMABatch";  }
};
template<> struct type<DenseVectorStdSort> {
   static constexpr const char* name() { return "StdDense";  }
};
template<> struct type<DenseVectorTimSort> {
   static constexpr const char* name() { return "TimDense";  }
};

//Reads the key only
void inline read_key(const void* el) {
   uint64_t volatile key = *(uint64_t*)el;
}

//Reads the full element
void inline read_element(const void* el) {
   elttype volatile elemt = *(elttype*)el;
}


template <typename container_t>
void inline run_queries(const container_t & container, QuadtreeIntf& quadtree, region_t q_region, int n_exp ){

   Timer t;

   // 1 - gets the minimum set of nodes that are inside the queried region
   // QueryRegion will traverse the tree and return the intervals to query;
   // NOTE: when comparing with the octree with pointer to elements the scan will be the traversall on the tree.
   std::vector<QuadtreeIntf*> q_nodes;
   quadtree.query_region(q_region, q_nodes);

   //WARMUP
   for (auto& el : q_nodes) {
      container.apply(el->begin(), el->end(), el->el(), read_element);
   }

   // access the container to count the number of elements inside the region
   for (int i = 0; i < n_exp; i++) {
      t.start();
      for (auto& el : q_nodes) {
         container.apply(el->begin(), el->end(), el->el(), read_element);
      }
      t.stop();
      PRINTBENCH("ReadElts",g_it_id,t.milliseconds(),"ms");
   }

}

template <typename container_t>
void run_bench(container_t container, std::vector<elttype>& input_vec, const int batch_size, int n_exp) {

   QuadtreeIntf quadtree(spatial_t(0, 0, 0));

   //create the pma
   container.create(input_vec.size());

   diff_cnt modifiedKeys;

   std::vector<elttype>::iterator it_begin = input_vec.begin();
   std::vector<elttype>::iterator it_curr = input_vec.begin();

   Timer t;
   //std::cout << typeid(Timer).name() << std::endl;

   g_it_id = 0;
   while (it_begin != input_vec.end()) {
      it_curr = std::min(it_begin + batch_size, input_vec.end());

      std::vector<elttype> batch(it_begin, it_curr);

      // insert batch
      t = container.insert(batch);
      PRINTBENCH("Insert", g_it_id ,t.milliseconds(),"ms", modifiedKeys.size() );

      // update iterator
      it_begin = it_curr;

      // retrieve modified keys
      modifiedKeys.clear();

      // Creates a map with begin and end of each index in the container.
      t = container.diff(modifiedKeys); //Extract information of new key range boundaries inside the container
      PRINTBENCH("ModifiedKeys", g_it_id, t.milliseconds(),"ms", modifiedKeys.size() );

      t.start();
      quadtree.update(modifiedKeys.begin(), modifiedKeys.end());
      t.stop();
      PRINTBENCH("QuadtreeUpdate", g_it_id, t.milliseconds(),"ms");


      // ========================================
      // Performs global scan
      // ========================================

      //Run a scan on the whole array
      run_queries(container, quadtree, region_t(0,0,1,1,0), n_exp);

      g_it_id++;
   }


}

int main(int argc, char* argv[]) {

   cimg_usage("Queries Benchmark inserts elements in batches.");
   const int batch_size(cimg_option("-b", 10, "Batch size used in batched insertions"));
   std::string fname(cimg_option("-f", "./data/tweet100.dat", "file with tweets"));
   const unsigned int n_exp(cimg_option("-x", 1, "Number of repetitions of each experiment"));

   PMABatch pma_container(argc, argv); //read pma command line parameters

//   DenseVectorStdSort vec_cont;
   DenseVectorTimSort tim_cont;

   const char* is_help = cimg_option("-h", (char*)0, 0);

   if (is_help) return false;

   const uint32_t quadtree_depth = 25;

   PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());
   std::vector<elttype> input_vec = input::load(fname, quadtree_depth);
   PRINTOUT(" %d teewts loaded \n", (uint32_t)input_vec.size());


   run_bench(pma_container, input_vec, batch_size, n_exp);

   //don't need to insert by batch for the dense vector case

  // run_bench(vec_cont, input_vec,  input_vec.size(), n_exp);
   run_bench(tim_cont, input_vec,  batch_size, n_exp);


   return EXIT_SUCCESS;

}
