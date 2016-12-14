/** @file
 * Benchmark for the queru time
 *
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

void inline count_element(uint32_t& accum, const spatial_t&, uint32_t count) {
   accum += count;
}

// reads the full element
void inline read_element(const valuetype& el) {
   valuetype volatile elemt = *(valuetype*)&el;
}

template <typename container_t>
void inline run_queries(container_t& container, region_t region, const int n_exp) {

   Timer timer;

   char qname[50];
   snprintf(qname, sizeof(qname), "%d-%d-%d-%d-%d", region.z, region.x0, region.y0, region.x1, region.y1);

   // 1 - gets the minimum set of nodes that are inside the queried region
   // QueryRegion will traverse the tree and return the intervals to query;
   // NOTE: when comparing with the octree with pointer to elements the scan will be the traversall on the tree.

   // access the container to count the number of elements inside the region

   uint32_t count = 0;
   applytype_function _apply = std::bind(count_element, std::ref(count),
                                         std::placeholders::_1, std::placeholders::_2);

   for (int i = 0; i < n_exp; i++) {
      count = 0;
      timer.start();
      container.apply_at_region(region, _apply);
      timer.stop();
      PRINTBENCH(qname, "Count", timer.milliseconds(),"ms");
   }

   // scans the container executing a function
   /*for (int i = 0; i < n_exp; i++) {
      timer.start();
      container.scan_at_region(region, read_keys);
      timer.stop();
      PRINTBENCH(qname, "ReadKeys", timer.milliseconds(), "ms");
   }*/   

   for (int i = 0; i < n_exp; i++) {
      timer.start();
      container.scan_at_region(region, read_element);
      timer.stop();
      PRINTBENCH(qname, "ReadElts", timer.milliseconds(), "ms");
   }
}

template <typename container_t>
void run_bench(container_t& container, std::vector<elttype>& input_vec, const int batch_size, const int n_exp) {

   //create container
   container.create((uint32_t)input_vec.size());

   // =====================================
   // Populates container and index
   // =====================================
   //Fully Populates the container

   if (dynamic_cast<PMABatchCtn*>(&container)) {
      std::vector<elttype>::iterator it_begin = input_vec.begin();
      std::vector<elttype>::iterator it_curr = input_vec.begin();
      std::cout << "Populates PMA" << std::endl;
      while (it_begin != input_vec.end()) {

         it_curr = std::min(it_begin + batch_size, input_vec.end());

         std::vector<elttype> batch(it_begin, it_curr);

         // insert batch
         container.insert(batch);

         // update insert iterator
         it_begin = it_curr;
      }
   } else {
      std::cout << "Populates Dense Vector" << std::endl;
      container.insert(input_vec);
   }

   // ========================================
   // Perform the queries
   // ========================================

   // 1 - Creates a region on the map to be queried
   // Region arround NY
   // http://localhost:7000/rest/query/region/14/4790/6116/4909/6204
   // x0 , y0 , x1, y1 , z

   run_queries(container, region_t(4790, 6116, 4909, 6204, 14), n_exp);
   run_queries(container, region_t(598, 1361, 1346, 1796, 12), n_exp);

   //The same query at different zoom levels (over US)  --> ARE THESE ALL THE SAME QUERIES ?
   // http://localhost:7000/rest/query/region/10/139/330/341/446
   // http://localhost:7000/rest/query/region/12/557/1320/1366/1786
   // http://localhost:7000/rest/query/region/14/2231/5280/5464/7144
   run_queries(container, region_t(139, 330, 341, 446, 10), n_exp);
   run_queries(container, region_t(557, 1320, 1366, 1786, 12), n_exp);
   run_queries(container, region_t(2231, 5280, 5464, 7144, 14), n_exp);

   run_queries(container, region_t(1249, 3071, 2501, 3226, 13), n_exp);

   run_queries(container, region_t(868, 1357, 1039, 1784, 12), n_exp);
   run_queries(container, region_t(0, 0, 0, 0, 0), n_exp);
}

int main(int argc, char* argv[]) {

   cimg_usage("Queries Benchmark inserts elements in batches.");
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

   run_bench(container0, input_vec, batch_size, n_exp);
   run_bench(container5, input_vec, batch_size, n_exp);
   run_bench(container6, input_vec, batch_size, n_exp);

   // don't need to insert by batch for the dense vector case
   /*run_bench(container1, input_vec, input_vec.size(), n_exp);
   run_bench(container2, input_vec, input_vec.size(), n_exp);
   run_bench(container3, input_vec, input_vec.size(), n_exp);
   run_bench(container4, input_vec, input_vec.size(), n_exp);*/
   

   return EXIT_SUCCESS;
}
