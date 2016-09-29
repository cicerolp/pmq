/** @file Tests the correctness of quadtree.
 *
 *
 * 1 - include a batch into the pma
 * 2 - update the quadtree
 *
 * Check:
 * - reference = traverse the whole pma.
 *
 * - traversal of root node == pma traversal
 * - traversal of level 1 == pma traversal.
 * ...
 *
 *
 */

#include "PMAInstance.h"

#include <queue>


const struct pma_struct* global_pma; //global reference to the pma, for debuging purpose

int main(int argc, char *argv[]) {

   cimg_usage("Benchmark inserts elements in batches.");
   const unsigned int seg_size ( cimg_option("-s",8,"Segment size for the pma"));
   const int batch_size ( cimg_option("-b",10,"Batch size used in batched insertions"));
   const float tau_0 ( cimg_option("-t0",0.92,"pma parameter tau_0"));
   const float tau_h ( cimg_option("-th",0.7,"pma parameter tau_h"));
   const float rho_0 ( cimg_option("-r0",0.08,"pma parameter rho_0"));
   const float rho_h ( cimg_option("-rh",0.3,"pma parameter rho_0"));
   std::string fname ( cimg_option("-f","../data/tweet100.dat","file with tweets"));

   const char* is_help = cimg_option("-h",(char*)0,0);

   if (is_help) return false;

   int errors = 0;

   PRINTOUT("Loading twitter dataset... %s \n",fname.c_str());

   // Create <key,value> elements
   std::vector<elttype> input_vec;
   loadTweetFile(input_vec, fname);

   int nb_elements = input_vec.size();

   PRINTOUT(" %d teewts loaded \n", (uint32_t)input_vec.size());

   struct pma_struct* pma = (struct pma_struct * ) pma::build_pma(nb_elements, sizeof(valuetype), tau_0, tau_h, rho_0, rho_h, seg_size);

   elttype * batch_start;
   int size = nb_elements / batch_size;
   int num_batches = 1 + (nb_elements-1)/batch_size;


   //Inserts all the batches
   for (int k = 0; k < num_batches; k++) {
      batch_start = &input_vec[k*size];

      if ((nb_elements-k*batch_size) / batch_size == 0) {
         size = nb_elements % batch_size;
      } else {
         size = batch_size;
      }

      insert_batch(pma, batch_start, size);

#ifndef NDEBUG
      PRINTOUT( "PMA WINDOWS : ") ;
      for (auto k: *(pma->last_rebalanced_segs)){
         std::cout << k << " "; //<< std::endl;
      }
#endif
      std::cout << "\n";

      // Creates a map with begin and end of each index in the pma.
      map_t modifiedKeys;
      pma_diff(pma,modifiedKeys); //Extract information of new key range boundaries inside the pma.

#ifndef NDEBUG
      PRINTOUT("ModifiedKeys %d : ",modifiedKeys.size()) ;
      for (auto& k: modifiedKeys){
         std::cout << k.key << " [" << k.begin << " " << k.end << "], ";
      }

      std::cout << "\n";

      PRINTOUT("pma keys: ");
      print_pma_keys(pma);
#endif


      std::cout << "\n";
      //TODO check that there are no modifiedKey outside of their range
      for (auto& k: modifiedKeys){
          //PMA is sorted

          if (k.begin == 0)
              continue;

          //check if key exists in previous segment ->> ERROR
          if (k.key <=  *(int64_t* ) SEGMENT_LAST(pma,k.begin-1) ){
              PRINTOUT("ERROR BEG Key: %lu (%d %d) \n", k.key, k.begin, k.end);
              errors++;
              //return EXIT_FAILURE;
           }

          if (k.end = pma->nb_segments)
              continue;

          //check if key exists in segment end ->> ERROR
          if (k.key >= *(int64_t* ) SEGMENT_START(pma,k.end)){
              PRINTOUT("ERROR END Key: %lu (%d %d) \n", k.key, k.begin, k.end);
              errors++;
              //return EXIT_FAILURE;
          }

      }

      if (errors){
          PRINTOUT("ERRORS: %d\n",errors);
          return EXIT_FAILURE;
      }

   }


   pma::destroy_pma(pma);
   return EXIT_SUCCESS;

}
