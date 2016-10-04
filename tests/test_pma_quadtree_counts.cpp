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

#include "stde.h"
#include "types.h"
#include "PMABatch.h"

#include "InputIntf.h"
#include "QuadtreeIntf.h"

#include <queue>

uint32_t g_Quadtree_Depth = 25;
const PMABatch* global_pma; //global reference to the pma, for debuging purpose

int BFS(QuadtreeIntf * root, int(*fun)(QuadtreeIntf * )) {
   std::queue<QuadtreeIntf*> Q;
   Q.push(root);

   while (!Q.empty()) {
      QuadtreeIntf* node = Q.front();

      Q.pop();

      if (fun(node)) {
         return 1; //found an error
      }

      for (auto& child : node->container()) {
         if (child != nullptr) {
            Q.push(child.get());
         };
      };
   }
   return 0;
}

int check_consistency(QuadtreeIntf* node) {
   return node->check_child_consistency();
}

int check_count(QuadtreeIntf* node) {
   return node->check_count(*global_pma);
}

int print_node(QuadtreeIntf* node) {
   static unsigned int level = 0;

   if (node == nullptr) return 0;

   if (level != node->el().z) {
      level = node->el().z;
      printf("\n %02d :", level);
   }

   // printf("%012lx [%d %d] ", node->code(), node->begin() , node->end() );

   unsigned int count = 0;
   global_pma->count(node->begin(), node->end(), node->el(), count);
   printf("%u [%d %d] ", count, node->begin(), node->end());

   return 0;
}

int print_node_range(QuadtreeIntf* node) {
   static unsigned int level = 0;

   if (node == nullptr) return 0;

   if (level != node->el().z) {
      level = node->el().z;
      printf("\n %02d :", level);
   }
   uint64_t min = 0, max = 0;
   PMABatch::get_mcode_range(node->el().code, node->el().z, min, max, 25);

   unsigned int count = 0;
   global_pma->count(node->begin(), node->end(), node->el(), count);
   printf("%u [%d %d] (%lu %lu) ", count, node->begin(), node->end(), min, max);

   return 0;
}

void static print_pma_el_key(const void* el) {

   printf("%lu ", *(uint64_t*)el);
}

int main(int argc, char* argv[]) {

   cimg_usage("Benchmark inserts elements in batches.");

   const int batch_size(cimg_option("-b", 10, "Batch size used in batched insertions"));
   std::string fname(cimg_option("-f", "../data/tweet100.dat", "file with tweets"));

   const char* is_help = cimg_option("-h", (char*)0, 0);

   if (is_help) return false;

   int errors = 0;

   const uint32_t quadtree_depth = 25;

   PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());
   std::vector<elttype> input = input::load_input(fname, quadtree_depth);
   PRINTOUT(" %d teewts loaded \n", (uint32_t)input.size());

   //QuadtreeIntf quadtree(spatial_t(0,0,0));
   std::shared_ptr<QuadtreeIntf> quadtree = std::make_shared<QuadtreeIntf>(spatial_t(0, 0, 0));

   PMABatch pma;
   global_pma = &pma;

   pma.create(input.size(), argc, argv);

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
      for (auto k: *(pma.get_container()->last_rebalanced_segs)) {
         std::cout << k << " "; //<< std::endl;
      }
      std::cout << "\n";
#endif

      // retrieve modified keys
      modifiedKeys.clear();
      // Creates a map with begin and end of each index in the pma.
      pma.diff(modifiedKeys); //Extract information of new key range boundaries inside the pma.

#ifndef NDEBUG
      PRINTOUT("ModifiedKeys %d : ", modifiedKeys.size());
      for (auto& k: modifiedKeys) {
         std::cout << k.key << " [" << k.begin << " " << k.end << "], ";
      }

      std::cout << "\n";

      PRINTOUT("pma keys: ");
      print_pma_keys(pma.get_container());
#endif

      quadtree->update(modifiedKeys.begin(), modifiedKeys.end());

      //Check every level.

      //BFS(PMQ.quadtree.get(),check_consistency);
      //BFS(PMQ.quadtree.get(),[](QuadtreeIntf* node){ print_node(node); return check_consistency(node);});
      //int ret = BFS(PMQ.quadtree.get(),[](QuadtreeIntf* node){ print_node(node); return node->check_count(global_pma);});

#ifndef NDEBUG
      PRINTOUT("QUADTREE DUMP:");
      BFS(quadtree.get(), print_node); // prints without any check
      std::cout << "\n";
#endif

      //traverese the tree checking counts
      int ret = BFS(quadtree.get(), [](QuadtreeIntf* node) {

                       if (node->check_count(*global_pma)) {

                          PRINTOUT("ERROR NODES : count [seg_b seg_e] (code_min code_max)");

                          //print problematic node
                          print_node_range(node);
                          for (auto& e : node->container()) print_node_range(e.get());

                          std::cout << "\n";

                          std::cout << "PMA RANGE: \n";
                          std::cout << "P> ";
                          global_pma->apply(node->begin(), node->end(), node->el(), print_pma_el_key);
                          std::cout << "\n";

                          std::cout << "C> ";
                          for (auto& e : node->container()) {
                             if (e != nullptr) {
                                global_pma->apply(e->begin(), e->end(), e->el(), print_pma_el_key);
                                printf(" | ");
                             }
                          }

                          return 1;
                       }

                       return 0;
                    });

      if (ret) {
         return EXIT_FAILURE;
      }
      //std::cout << "\n";

   }

   return EXIT_SUCCESS;

}
