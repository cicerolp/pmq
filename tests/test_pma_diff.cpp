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

#include <stde.h>
#include "types.h"
#include "PMABatchCtn.h"

#include "InputIntf.h"

const struct pma_struct* global_pma; //global reference to the pma, for debuging purpose

uint32_t g_Quadtree_Depth = 25;

class  TEST_PMABatchCtn : public PMABatchCtn {

    using PMABatchCtn::PMABatchCtn;

public:
    using PMABatchCtn::diff;
    using PMABatchCtn::_pma;
    using PMABatchCtn::_quadtree;

};


int main(int argc, char* argv[]) {

   cimg_usage("Benchmark inserts elements in batches.");
   const int batch_size(cimg_option("-b", 10, "Batch size used in batched insertions"));
   std::string fname(cimg_option("-f", "../data/tweet100.dat", "file with tweets"));

   TEST_PMABatchCtn pma(argc, argv);

   const char* is_help = cimg_option("-h", (char*)0, 0);
   if (is_help) return false;

   int errors = 0;

   const uint32_t quadtree_depth = 25;

   PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());
   std::vector<elttype> input = input::load(fname, quadtree_depth);
   PRINTOUT(" %d teewts loaded \n", (uint32_t)input.size());
   
   pma.create(input.size());

   diff_cnt modifiedKeys;

   std::vector<elttype>::iterator it_begin = input.begin();
   std::vector<elttype>::iterator it_curr = input.begin();

   while (it_begin != input.end()) {
      it_curr = std::min(it_begin + batch_size, input.end());

      std::vector<elttype> batch(it_begin, it_curr);

      // insert batch
      pma.insert(batch);

      // update iterator
      it_begin = it_curr;

#ifndef NDEBUG
      PRINTOUT("PMA WINDOWS : ");
      for (auto k: *(pma._pma->last_rebalanced_segs)) {
         std::cout << k << " "; //<< std::endl;
      }
      std::cout << "\n";
#endif

      // retrieve modified keys
      modifiedKeys.clear();
      // Creates a map with begin and end of each index in the pma.
      modifiedKeys = pma.diff(); //Extract information of new key range boundaries inside the pma.

#ifndef NDEBUG
      PRINTOUT("ModifiedKeys %d : ", modifiedKeys.size());
      for (auto& k: modifiedKeys) {
         std::cout << k.key << " [" << k.begin << " " << k.end << "], ";
      }

      std::cout << "\n";

      PRINTOUT("pma keys: ");
      print_pma_keys(pma._pma);
      std::cout << "\n";
#endif

      //TODO check that there are no modifiedKey outside of their range
      for (auto& k: modifiedKeys) {
         //PMA is sorted

         if (k.begin == 0) continue;

         //check if key exists in previous segment ->> ERROR
         if (k.key <= *(int64_t*) SEGMENT_LAST(pma._pma, k.begin - 1)) {
            PRINTOUT("ERROR BEG Key: %lu (%d %d) \n", k.key, k.begin, k.end);
            errors++;
            //return EXIT_FAILURE;
         }

         if (k.end = pma._pma->nb_segments) continue;

         //check if key exists in segment end ->> ERROR
         if (k.key >= *(int64_t*) SEGMENT_START(pma._pma, k.end)) {
            PRINTOUT("ERROR END Key: %lu (%d %d) \n", k.key, k.begin, k.end);
            errors++;
            //return EXIT_FAILURE;
         }

      }

      if (errors) {
         PRINTOUT("ERRORS: %d\n", errors);
         return EXIT_FAILURE;
      }
   }

   PRINTOUT("TEST SUCCESS \n");
   return EXIT_SUCCESS;

}
