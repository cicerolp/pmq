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
#include "PMABatch.h"

#include "InputIntf.h"
#include "QuadtreeIntf.h"

#include <typeinfo>


uint32_t g_Quadtree_Depth = 25;

template<typename container_t >
void run_bench(container_t container,std::vector<elttype> & input_vec,const int batch_size){

   QuadtreeIntf quadtree(spatial_t(0,0,0));

   //create the pma
   container.create(input_vec.size());

   diff_cnt modifiedKeys;
   std::vector<elttype>::iterator it_begin = input_vec.begin();
   std::vector<elttype>::iterator it_curr = input_vec.begin();

   //Timer t;
   duration_t t;

   //Two option to get the name
   PRINTOUTF("Running Bencmark with %s  \n", typeid(container_t).name() );
// std::cout << typeid(container_t).name() << std::endl;
   std::cout << __PRETTY_FUNCTION__  << std::endl;
   std::cout << container_t::name()  << std::endl;



   while (it_begin != input_vec.end()) {
      it_curr = std::min(it_begin + batch_size, input_vec.end());

      std::vector<elttype> batch(it_begin, it_curr);

      // insert batch
      t = container.insert(batch);
      //PRINTCSVF("Insert", t.milliseconds(),"ms", modifiedKeys.size() );

      // update iterator
      it_begin = it_curr;

      // retrieve modified keys
      modifiedKeys.clear();

      // Creates a map with begin and end of each index in the container.
      t = container.diff(modifiedKeys); //Extract information of new key range boundaries inside the container
//      PRINTCSVF("ModifiedKeys", t.milliseconds(),"ms", modifiedKeys.size() );

//      t.start();
      quadtree.update(modifiedKeys.begin(), modifiedKeys.end());
  //    t.stop();
  //    PRINTCSVF("QuadtreeUpdate", t.milliseconds(),"ms");

   }
}


int main(int argc, char *argv[]) {

   cimg_usage("Benchmark inserts elements in batches.");
   const int batch_size(cimg_option("-b", 10, "Batch size used in batched insertions"));
   std::string fname ( cimg_option("-f","./data/tweet100.dat","file with tweets"));
   const unsigned int n_exp ( cimg_option("-x",1,"Number of repetitions of each experiment"));

   PMABatch pma_container(argc, argv); //read pma command line parameters

   const char* is_help = cimg_option("-h",(char*)0,0);

   if (is_help) return false;

   const uint32_t quadtree_depth = 25;

   PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());
   std::vector<elttype> input_vec = input::load(fname, quadtree_depth);
   PRINTOUT(" %d teewts loaded \n", (uint32_t)input_vec.size());

   for (int i = 0 ; i < n_exp; i++){

     run_bench<PMABatch>(pma_container,input_vec,batch_size);

#if 0
     do_bench_benderPMA(&input_vec[0],input_vec.size(),reference_array,tau_0,tau_h,rho_0,rho_h,seg_size);

     do_bench_stlsort(&input_vec[0],input_vec.size(),batch_size,reference_array);

     do_bench_qsort(&input_vec[0],input_vec.size(),batch_size,reference_array);



     do_bench_mergesort(&input_vec[0],input_vec.size(),batch_size,reference_array);

#endif

   }


}
