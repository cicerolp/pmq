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


uint32_t g_Quadtree_Depth = 25;

// reads the full element
void inline read_element(const valuetype& el) {
   valuetype volatile elemt = *(valuetype*)&el;
}


template <typename container_t>
void fill_container(container_t& container, std::vector<elttype>& input_vec, const int batch_size, uint64_t rm_time_limit ) {
   //create container
   container.create((uint32_t)input_vec.size());

   std::vector<elttype>::iterator it_begin = input_vec.begin();
   std::vector<elttype>::iterator it_curr = input_vec.begin();

   uint64_t t = 0;
   uint64_t oldest_time  = 0;
   while (it_begin != input_vec.end()) {
      it_curr = std::min(it_begin + batch_size, input_vec.end());

      std::vector<elttype> batch(it_begin, it_curr);


      // insert batch
      if (t > rm_time_limit){
         oldest_time++;
      }

      PRINTOUT("Removing with oldest time %u\n",oldest_time);

      container.insert_rm(batch, [ oldest_time ]( const void* el) {
               return ((elttype*)el)->value.time < oldest_time;
           });

      it_begin = it_curr;
      t++ ;
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

   GeoHashSequential container(argc, argv);

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

   ////////////////////////////////////////////////////////////////////



   fill_container(container, input_vec, batch_size,rm_time);

   container.scan_at_region( region_t(0, 0, 0, 0, 0) , read_element);

   //container.scan_at_region( region_t(139, 330, 341, 446, 10) , read_element);
  // container.scan_at_region( region_t(557, 1320, 1366, 1786, 12) , read_element);
  // container.scan_at_region( region_t(2231, 5280, 5464, 7144, 14) , read_element);


   return EXIT_SUCCESS;

}
