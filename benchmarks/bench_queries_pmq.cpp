/** @file
 * Benchmark for the queru time
 *
 *
 */

#include "stde.h"
#include "types.h"
#include "PMABatch.h"

#include "InputIntf.h"
#include "QuadtreeIntf.h"


uint32_t g_Quadtree_Depth = 25;


template< typename T> struct type {
   static constexpr const char* name() { return "unknown";  }  // end type< T>::name
}; // type< T>

template<> struct type<PMABatch> {
   static constexpr const char* name() { return "PMABatch";  }
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
void run_bench(container_t container, std::vector<elttype>& input_vec, const int batch_size) {
#define PRINTBENCH( ... ) do { \
   std::cout << "QueriesBench " << type<container_t>::name() << " ; ";\
   printcsv( __VA_ARGS__ ) ; \
   std::cout << std::endl ;\
} while (0)

   QuadtreeIntf quadtree(spatial_t(0, 0, 0));

   //create the pma
   container.create(input_vec.size());

   diff_cnt modifiedKeys;
   std::vector<elttype>::iterator it_begin = input_vec.begin();
   std::vector<elttype>::iterator it_curr = input_vec.begin();

   Timer t;

   // =====================================
   // Populates container and index
   // =====================================
   //Fully Populates the container
   while (it_begin != input_vec.end()) {
      it_curr = std::min(it_begin + batch_size, input_vec.end());

      std::vector<elttype> batch(it_begin, it_curr);

      // insert batch
      container.insert(batch);

      // update insert iterator
      it_begin = it_curr;
    }

    //Populates the quadtree index
    container.clear_diff();

    //retrieve modified keys
    modifiedKeys.clear();

    // Creates a map with begin and end of each index in the container.
    // Extract information of new key range boundaries inside the container
    container.diff(modifiedKeys);

    quadtree.update(modifiedKeys.begin(), modifiedKeys.end());

    // ========================================
    // Perform the queries
    // ========================================

    // 1 - Creates a region on the map to be queried
    // Region arround NY
    // http://localhost:7000/rest/query/region/14/4790/6116/4909/6204
    region_t q_region(4790,6116,4909,6204,14);

    // 2 - gets the minimum set of nodes that are inside the queried region
    // QueryRegion will traverse the tree and return the intervals to query;
    // NOTE: when comparing with the octree with pointer to elements the scan will be the traversall on the tree.
    std::vector<QuadtreeIntf*> q_nodes;
    quadtree.query_region(q_region, q_nodes);

    // 3 - access the container to count the number of elements inside the region


    for (int i = 0; i < 10; i++) {
       uint32_t count = 0;
       t.start();
       for (auto& el : q_nodes) {
          container.count(el->begin(), el->end(), el->el(), count);
       }
       t.stop();
       PRINTBENCH("Count", t.milliseconds(),"ms", q_nodes.size() );
}
    for (int i = 0; i < 10; i++) {
       // 4 - scans the container executing a function
       t.start();
       for (auto& el : q_nodes) {
          container.apply(el->begin(), el->end(), el->el(), read_key);
       }
       t.stop();
       PRINTBENCH("ReadKeys", t.milliseconds(),"ms", q_nodes.size() );
}
    for (int i = 0; i < 10; i++) {
       t.start();
       for (auto& el : q_nodes) {
          container.apply(el->begin(), el->end(), el->el(), read_element);
       }
       t.stop();
       PRINTBENCH("ReadElts", t.milliseconds(),"ms", q_nodes.size() );
    }



}

int main(int argc, char* argv[]) {

   cimg_usage("Queries Benchmark inserts elements in batches.");
   const int batch_size(cimg_option("-b", 10, "Batch size used in batched insertions"));
   std::string fname(cimg_option("-f", "./data/tweet100.dat", "file with tweets"));
   const unsigned int n_exp(cimg_option("-x", 1, "Number of repetitions of each experiment"));

   PMABatch pma_container(argc, argv); //read pma command line parameters

   const char* is_help = cimg_option("-h", (char*)0, 0);

   if (is_help) return false;

   const uint32_t quadtree_depth = 25;

   PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());
   std::vector<elttype> input_vec = input::load(fname, quadtree_depth);
   PRINTOUT(" %d teewts loaded \n", (uint32_t)input_vec.size());


   run_bench<PMABatch>(pma_container, input_vec, batch_size);


   return EXIT_SUCCESS;

}
