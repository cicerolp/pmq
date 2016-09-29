/** @file
 * Micro Benchmark for PMQ operations.
 *
 * - Insertion of a batch
 * - DIFF of modified Keys
 * - UPDATE of the Quadtree.
 *
 */

#include "PMAInstance.h"
#include <queue>


uint32_t g_Quadtree_Depth = 25;
const struct pma_struct* global_pma; //global reference to the pma, for debuging purpose

int main(int argc, char *argv[]) {

   cimg_usage("Benchmark inserts elements in batches.");
   const unsigned int seg_size ( cimg_option("-s",8,"Segment size for the pma"));
   const int batch_size ( cimg_option("-b",10,"Batch size used in batched insertions"));
   const float tau_0 ( cimg_option("-t0",0.92,"pma parameter tau_0"));
   const float tau_h ( cimg_option("-th",0.7,"pma parameter tau_h"));
   const float rho_0 ( cimg_option("-r0",0.08,"pma parameter rho_0"));
   const float rho_h ( cimg_option("-rh",0.3,"pma parameter rho_0"));
   std::string fname ( cimg_option("-f","./data/tweet100.dat","file with tweets"));

   const char* is_help = cimg_option("-h",(char*)0,0);

   if (is_help) return false;

   PMAInstance PMQ;
   Timer t;

   PRINTOUT("Loading twitter dataset... %s \n",fname.c_str());

   // Create <key,value> elements
   std::vector<elttype> input_vec;
   loadTweetFile(input_vec, fname);

   int nb_elements = input_vec.size();

   PRINTOUT(" %d teewts loaded \n", (uint32_t)input_vec.size());

   t.start();
   PMQ.pma = (struct pma_struct * ) pma::build_pma(nb_elements, sizeof(valuetype), tau_0, tau_h, rho_0, rho_h, seg_size);
   t.stop();
   PRINTCSVL("build_pma", t.milliseconds(),"ms" );

   global_pma = PMQ.pma;

   if (PMQ.quadtree == nullptr)
      PMQ.quadtree = std::make_unique<SpatialElement>(spatial_t(0,0,0));


   elttype * batch_start;
   int size = nb_elements / batch_size;
   int num_batches = 1 + (nb_elements-1)/batch_size;

   //Inserts all the batches:
   for (int k = 0; k < num_batches; k++) {
      PRINTOUT("BATCH %d / %d\n", k , num_batches);
      batch_start = &input_vec[k*size];

      if ((nb_elements-k*batch_size) / batch_size == 0) {
         size = nb_elements % batch_size;
      } else {
         size = batch_size;
      }

      insert_batch(PMQ.pma, batch_start, size);

      // Creates a map with begin and end of each index in the pma.
      map_t modifiedKeys;
      t.start();
      pma_diff(PMQ.pma,modifiedKeys); //Extract information of new key range boundaries inside the pma.
      t.stop();
      PRINTCSVL("ModifiedKeys", t.milliseconds(),"ms", modifiedKeys.size() );

      t.start();
      PMQ.quadtree->update(PMQ.pma, modifiedKeys.begin(), modifiedKeys.end());
      t.stop();
      PRINTCSVL("Quadtree Update", t.milliseconds(),"ms");

   }

   pma::destroy_pma(PMQ.pma);
   return EXIT_SUCCESS;

}
