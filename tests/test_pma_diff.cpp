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


int BFS(  SpatialElement* root, int (*fun)(SpatialElement *) )
{
    std::queue< SpatialElement* > Q;
    Q.push(root);

    while(!Q.empty())
    {
       SpatialElement* node = Q.front();

       Q.pop();

       if( fun(node) ){
          return 1; //found an error
       }

       for (auto& child : node->container()) {
          if (child != nullptr ){
             Q.push(child.get());
          };
       };
    }
    return 0;
}

int check_consistency(SpatialElement* node){
    return node->check_child_consistency();
}

int print_node(SpatialElement* node){
    std::cout << node->zoom() << ": " << node->code() << " [" << node->begin() <<", "<< node->end() << "] " << std::endl;
    return 0;
}

uint32_t g_Quadtree_Depth = 25;

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

   PMAInstance PMQ;

   PRINTOUT("Loading twitter dataset... %s \n",fname.c_str());

   // Create <key,value> elements
   std::vector<elttype> input_vec;
   loadTweetFile(input_vec, fname);

   int nb_elements = input_vec.size();

   PRINTOUT(" %d teewts loaded \n", (uint32_t)input_vec.size());

   PMQ.pma = (struct pma_struct * ) pma::build_pma(nb_elements, sizeof(valuetype), tau_0, tau_h, rho_0, rho_h, seg_size);

   Timer t;
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

      insert_batch(PMQ.pma, batch_start, size);

#ifndef NDEBUG
      PRINTOUT( "PMA WINDOWS : ") ;
      for (auto k: *(PMQ.pma->last_rebalanced_segs)){
         std::cout << k << " "; //<< std::endl;
      }
#endif
      std::cout << "\n";

      // Creates a map with begin and end of each index in the pma.
      map_t modifiedKeys;
      t.start();
      pma_diff(PMQ.pma,modifiedKeys); //Extract information of new key range boundaries inside the pma.
      t.stop();

      PRINTCSVL("ModifiedKeys", t.milliseconds(),"ms" );

      if (PMQ.quadtree == nullptr)
         PMQ.quadtree = std::make_unique<SpatialElement>(spatial_t(0,0,0));

#ifndef NDEBUG
      PRINTOUT("ModifiedKeys %d : ",modifiedKeys.size()) ;
      for (auto& k: modifiedKeys){
         std::cout << k.key << " " ;
      }

      std::cout << "\n";

      PRINTOUT("pma keys: ");
      print_pma_keys(PMQ.pma);
#endif
      t.start();
      PMQ.quadtree->update(PMQ.pma, modifiedKeys.begin(), modifiedKeys.end());
      if (modifiedKeys.size() != 0) PMQ.up_to_date = false;
      t.stop();
      PRINTCSVL("QuadtreeUpdate" , t.milliseconds(),"ms" , k);
      std::cout << "\n";

      //Check every level.
      // TODO FIX SEGFAULT HERE
      BFS(PMQ.quadtree.get(),print_node);


   }

   PMQ.destroy();
   return EXIT_SUCCESS;

}
