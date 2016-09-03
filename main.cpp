#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <vector>
#include <string.h>
#include <time.h>
#include <algorithm>


#include "PmaConfig.h"
#include "pma/pma.h"
#include "pma/pma_priv.h"
#include "pma/utils/pma_utils.h"
#include "pma/utils/bitmask.h"
#include "pma/utils/coherency.h"
#include "pma/utils/debugMacros.h"
#include "pma/utils/test_utils.h"

#include "pma/utils/benchmark_utils.h"

#include "ext/CImg/CImg.h"

#include "DMPLoader/dmploader.hpp"


#ifdef __APPLE__
#include "mac_utils.h"
#endif



#ifdef PMA_TRACE_MOVE
  extern unsigned int g_iteration_counter;
#endif


typedef tweet_t valuetype;
struct elttype{
   uint64_t key;
   valuetype value;
//Compare only KEYS?
   // Pma uses only the key to sort elements.
   // This way we can use the reference check function to compare results.
   friend inline bool operator==(const elttype& lhs, const elttype& rhs){ return (lhs.key == rhs.key); }
//   friend inline bool operator==(const elttype& lhs, const elttype& rhs){ return (lhs.key == rhs.key) && (lhs.value == rhs.value); }
   friend inline bool operator!=(const elttype& lhs, const elttype& rhs){ return !(lhs == rhs); }
   friend inline bool operator<(const elttype& lhs, const elttype& rhs){ return (lhs.key < rhs.key); }

   friend std::ostream& operator<<(std::ostream &out, const elttype& e)  { return out << e.key; } //output

};

std::ostream& operator<<(std::ostream& out, const tweet_t& e)
{
   return out << e.time ;
}

void display_help(char* exe_name)
{
   fprintf(stdout, "Usage : %s nb_elements [random_init]  [tau_0 tau_h rho_0 rho_h] [move_ratio insert_ratio del_ratio]\n",exe_name);
   fprintf(stdout, "\n");

}// display_help


// instertion sort: known to be efficient for almost sorted arrays.
void isort(const void * base, const size_t nbelements, const size_t eltsize, int (*compar)( const void *, const void * ))
{
   char  *  tmp = (char* ) malloc(eltsize);
   char * ptr_i;
   char * ptr_j;
   char * ptr_end = (char * ) ((char*)base + nbelements*eltsize);
   for(ptr_i= (char *) ((char*)base+eltsize); ptr_i < ptr_end; ptr_i+=eltsize)
   {
      memcpy(tmp,ptr_i,eltsize);
      ptr_j = ptr_i;
      while (ptr_j > (char*)base &&  compar(ptr_j-eltsize,tmp)>0)
      {
         // move hole to next smaller index
         memcpy(ptr_j, ptr_j-eltsize,eltsize);
         ptr_j -= eltsize;
      }
      // put item in the hole
      memcpy(ptr_j,tmp,eltsize);
   }
}


unsigned int sort_check(const elttype* reference, unsigned int array_size, void *p /*ma_handler*/)
{
   unsigned int i;
   uint64_t elt;
   unsigned int nb = 0;
   void *iter = NULL;
   iter = pma_create_iterator(p);
   if(iter == NULL)
   {
      fprintf(stderr,"error in pma_create_iterator\n");
      exit(EXIT_FAILURE);
   }
   for (i = 0,pma_iter_begin(p, iter); !pma_iter_end(p, iter); pma_iter_next(p, iter), i++)
   {
      elt = pma_iter_get_key(p, iter);

      if (reference[i].key != elt)
      {
         fprintf(stderr,"sort error: expected %lu, got %lu\n",
                 reference[i].key,elt);
         nb++;
      }
   }
   pma_destroy_iterator(iter);
   // pma too short: should contian more elements
   if ( i < array_size)
      nb+=(array_size-i);
   fprintf(stdout, "i = %u\n", i);
   return nb;
}// sort_check


void do_bench_qsort(elttype* input_array, int nb_elements, int batch_size, elttype* reference_array)
{
  simpleTimer t;
  double sortTime = 0;

  elttype *qsort_array = (elttype*) malloc( (nb_elements) *sizeof(elttype));
  elttype *qsort_array_end = qsort_array;

  DBG_PRINTOUT("QSORT INSERT BY BATCHES SIZE = %d\n",batch_size);
  elttype * batch_start;
  int size = nb_elements / batch_size;
  int num_batches = 1 + (nb_elements-1)/batch_size;

  for (int k = 0; k < num_batches; k++){
    batch_start = input_array + (k*size);

    if ((nb_elements-k*batch_size) / batch_size == 0){
      size = nb_elements % batch_size;
    }else{
      size = batch_size;
    }

    t.start();
    //Copy batch to end of the array
    memcpy( qsort_array_end, batch_start, sizeof(elttype) * size);
    qsort_array_end += size;

    //Resort the whole array.
    qsort( qsort_array, qsort_array_end - qsort_array,sizeof(elttype), comp<uint64_t>);
    t.stop();
    PRINTCSVL("Qsort insert", k , t.miliseconds(),"ms" );

    sortTime += t.miliseconds();
  }

  //Check reference
  //int res = memcmp(qsort_array,reference_array,nb_elements*sizeof(elttype));
  bool res = std::equal(reference_array,reference_array+nb_elements,qsort_array);

  if (res){
    PRINTERR("QSORT SUCCESS !\n");
    PRINTERR("QSORT TOTAL: %f ms\n",sortTime);
#if 0
    std::cout << " QSORT array: " << std::endl;
    for ( int i = 0; i < nb_elements ; i++ )
      std::cout << "[" << qsort_array[i].key << ", " << qsort_array[i].value << "] " ;
#endif

  }else{
    PRINTERR("QSORT ERROR !\n");

    std::cout << " QSORT array: " << std::endl;
    for ( int i = 0; i < nb_elements ; i++ )
      std::cout << "[" << qsort_array[i].key << ", " << qsort_array[i].value << "] " ;

    std::cout << "\n Reference " << std::endl;
    for ( int i = 0; i < nb_elements ; i++ )
      std::cout << "[" << reference_array[i].key << ", " << reference_array[i].value << "] " ;
    std::cout << std::endl;
  }

  free(qsort_array);
  return;
}


void do_bench_stlsort(elttype* input_array, int nb_elements, int batch_size, elttype* reference_array)
{
  simpleTimer t;
  double stlSortTime = 0 ;

  //* We test the execution time of sorting the vector after a batch insertion.
  std::vector<elttype> vecResult;
  vecResult.reserve(nb_elements);

  DBG_PRINTOUT("STD SORT INSERT BY BATCHES SIZE = %d\n",batch_size);
  elttype * batch_start;
  int size = nb_elements / batch_size;
  int num_batches = 1 + (nb_elements-1)/batch_size;

  for (int k = 0; k < num_batches; k++){
    batch_start = input_array + (k*size);

    if ((nb_elements-k*batch_size) / batch_size == 0){
      size = nb_elements % batch_size;
    }else{
      size = batch_size;
    }

    t.start();
    //Copy batch to end of the array
    vecResult.insert(vecResult.end(),batch_start,batch_start+size);

    //Resort the whole array.
    //std::stable_sort( vecResult.begin(), vecResult.end(), [](elttype a, elttype b) { return a < b; });
    std::sort( vecResult.begin(), vecResult.end(), [](elttype a, elttype b) { return a < b; });
    t.stop();
    PRINTCSVL("std sort insert", k , t.miliseconds(),"ms" );

    stlSortTime += t.miliseconds();
  }

  //Check reference
  //     int res = memcmp(&vecResult[0],reference_array,nb_elements*sizeof(elttype));
  bool res = std::equal(reference_array,reference_array+nb_elements,vecResult.begin());
  if ( res ){
    PRINTERR("STL SORT SUCCESS !\n");
    PRINTERR("STL TOTAL: %f ms\n",stlSortTime);
#if 0
    std::cout << " STL vector: " << std::endl;
    for ( auto e : vecResult)
      std::cout << "[" << e.key << ", " << e.value << "] " ;
#endif
  }
  else{
    PRINTERR("STL SORT ERROR !\n");
    std::cout << " STL vector: " << std::endl;
    for ( auto e : vecResult)
      std::cout << "[" << e.key << ", " << e.value << "] " ;

    std::cout << "\n Reference " << std::endl;
    for ( int i = 0; i < nb_elements ; i++ )
      std::cout << "[" << reference_array[i].key << ", " << reference_array[i].value << "] " ;
    std::cout << std::endl;
  }

}


void do_bench_benderPMA(elttype* input_array, int nb_elements, elttype* reference_array,
                        float tau_0, float tau_h, float rho_0, float rho_h, unsigned int seg_size )
{
  simpleTimer t;
  double benderTime = 0;

  /* Init pma and allocate memory for storage of nb_elements */
  t.start();
  void* pma_handler = build_pma(nb_elements,sizeof(valuetype), tau_0, tau_h, rho_0, rho_h, seg_size);
  t.stop();
  if (pma_handler == NULL)
  {
    fprintf(stderr, "Problem with pma build\n");
    exit(EXIT_FAILURE);
  }
  PRINTCSVL("Build PMA", 0 , t.miliseconds(),"ms" );

#ifdef DO_PMA_STATS
  pma_stats_reset(pma_handler);
#endif// DO_PMA_STATS

  struct pma_struct *pma = (struct pma_struct*)pma_handler;

#ifdef PMA_TRACE_MOVE
  int dbg_nb_segments = pma->nb_segments;
#endif

  unsigned int trace_seg_level; // trace the number of segments that were touched during the add_elt function.

  /* Fill pma in traditional bender way one by one */
  // We disable this code because it too slow and seems to be throwing a segfault.
  if (nb_elements <= 1000000) {
    for(int i = 0; i < nb_elements; i++) {

#ifdef PMA_TRACE_MOVE
      g_iteration_counter = i;
#endif
      t.start();
      trace_seg_level = add_elt(pma_handler, input_array[i].key, (void*)&(input_array[i].value));
      t.stop();

      PRINTCSVL("Bender insert", i , t.miliseconds(),"ms", trace_seg_level );
#ifdef PMA_TRACE_MOVE
      sprintf(str,"Bender ; %d",i);
      pma_print_moves(pma_handler,stdout,str);
      pma_stats_reset(pma_handler);

      if (dbg_nb_segments < pma->nb_segments){
        dbg_nb_segments = pma->nb_segments;
        print_pma_levels(pma_handler);
      }
#endif
      benderTime += t.miliseconds();
    }

    assert(pma->elts[ELTS_ROOTINDEX] == nb_elements);


    // Verify result
    int errors = reference_check(reference_array, pma_handler);
    if (!errors) {
      PRINTERR("BENDER SUCCESS !\n");
      PRINTERR("BENDER TOTAL: %f ms\n",benderTime);
    }
    else {
      PRINTERR("%d ERRORS \n", errors);
      PRINTOUT("PMA DUMP: ");
      dump_pma_array((struct pma_struct*) pma_handler);
      //   print_pma(pma,0,pma->nb_segments);
      PRINTOUT("REFERENCE: ");
      for (int i = 0; i < nb_elements; i++){
        printf("[%lu:%d] ", reference_array[i].key, reference_array[i].value );
      }
      exit(EXIT_FAILURE);
    }
  }else{
    PRINTCSVL("Bender insert", "Disabled" );
  }

#ifdef DO_PMA_STATS
  pma_stats_print(pma_handler,stdout);
  pma_stats_reset(pma_handler);
#endif// DO_PMA_STATS

  destroy_pma(pma_handler);
  return;
}

void do_bench_batchPMA(elttype* input_array, int nb_elements, int batch_size, elttype* reference_array,
                        float tau_0, float tau_h, float rho_0, float rho_h, unsigned int seg_size )
{
  simpleTimer t;
  double insertTime = 0; //time to add the batch in the pma
  double inputTime = 0; //time to prepare the batch

  /* Fill PMA by batches of size */
  void * pma_handler = build_pma(nb_elements,sizeof(valuetype), tau_0, tau_h, rho_0, rho_h, seg_size);
  struct pma_struct *pma = (struct pma_struct*)pma_handler;

#ifdef PMA_TRACE_MOVE
  int dbg_nb_segments = pma->nb_segments;
#endif

  DBG_PRINTOUT("INSERT BY BATCHES SIZE = %d\n",batch_size);

  elttype * batch_start;
  int size = nb_elements / batch_size;
  int num_batches = 1 + (nb_elements-1)/batch_size;

  for (int k = 0; k < num_batches; k++)
  {
#ifdef PMA_TRACE_MOVE
    g_iteration_counter = k;
#endif
    batch_start = input_array + (k*size);

    if ((nb_elements-k*batch_size) / batch_size == 0){
      size = nb_elements % batch_size;
    }else{
      size = batch_size;
    }

    DBG_PRINTOUT("Insert batch with size %d\n",size);

    t.start();
    //Inserted batch needs to be sorted already;
    //qsort( batch_start, size,sizeof(elttype), comp<uint64_t>);
    std::sort( batch_start, batch_start + size , [](elttype a, elttype b) { return a < b; });
    t.stop();
    PRINTCSVL("Batch sort", k , t.miliseconds(),"ms" );
    inputTime += t.miliseconds();

    /* Inserts the current batch  */
    t.start();
    add_array_elts(pma_handler,(void *)batch_start, (void *) ((char *)batch_start + (size)*sizeof(elttype)),comp<uint64_t>);
    t.stop();
    insertTime += t.miliseconds();
    PRINTCSVL("Batch insert", k , t.miliseconds(),"ms" );

#ifdef PMA_TRACE_MOVE
    sprintf(str,"Batch ; %d",k);
    pma_print_moves(pma_handler,stdout,str);
    pma_stats_reset(pma_handler);

    if (dbg_nb_segments < pma->nb_segments){
      dbg_nb_segments = pma->nb_segments;
      print_pma_levels(pma_handler);
    }
#endif
  }

  assert(pma->elts[ELTS_ROOTINDEX] == nb_elements);
  int errors = reference_check(reference_array, pma_handler);
  if (!errors)
  {
    PRINTERR("BATCH SUCCESS !\n");

    PRINTERR("BATCH TOTAL : %f ms\n",insertTime);
    PRINTERR("BATCH TOTAL+: %f ms\n",insertTime+inputTime);

    //    dump_pma_array((struct pma_struct*) pma_handler);
    /*
      std::cout << "\n Reference " << std::endl;
      for ( int i = 0; i < nb_elements ; i++ )
        std::cout << "[" << reference_array[i].key << ", " << reference_array[i].value << "] " ;
      std::cout << std::endl;
      */
  }
  else
  {
    PRINTERR("%d ERRORS \n", errors);
    PRINTOUT("PMA DUMP PRINT : ");
    dump_pma_array((struct pma_struct*) pma_handler);
    //    print_pma(pma,0,pma->nb_segments);
    exit(EXIT_FAILURE);
  }

#ifdef DO_PMA_STATS
  pma_stats_print(pma_handler,stdout);
#endif// DO_PMA_STATS
  destroy_pma(pma_handler);
  return ;
}

void do_bench_mergesort(elttype* input_array, int nb_elements, int batch_size, elttype* reference_array)
{
  simpleTimer t;
  double stlSortTime = 0 ;

  //* We test the execution time of sorting the vector after a batch insertion.
  std::vector<elttype> vecResult;
  vecResult.reserve(nb_elements);

  DBG_PRINTOUT("STD MERGE SORT INSERT BY BATCHES SIZE = %d\n",batch_size);
  elttype * batch_start;
  int size = nb_elements / batch_size;
  int num_batches = 1 + (nb_elements-1)/batch_size;

  for (int k = 0; k < num_batches; k++){
    batch_start = input_array + (k*size);

    if ((nb_elements-k*batch_size) / batch_size == 0){
      size = nb_elements % batch_size;
    }else{
      size = batch_size;
    }

    t.start();

    std::sort( batch_start, batch_start + size , [](elttype a, elttype b) { return a < b; });
    //Copy batch to end of the array
    vecResult.insert(vecResult.end(),batch_start,batch_start+size);
    std::inplace_merge(vecResult.begin(), vecResult.end() - size, vecResult.end(), [](elttype a, elttype b) { return a < b; });

    t.stop();
    PRINTCSVL(__FUNCTION__, k , t.miliseconds(),"ms" );

    stlSortTime += t.miliseconds();
  }

  //Check reference
  //     int res = memcmp(&vecResult[0],reference_array,nb_elements*sizeof(elttype));
  bool res = std::equal(reference_array,reference_array+nb_elements,vecResult.begin());
  if ( res ){
    PRINTERR("%s SUCCESS !\n",__FUNCTION__);
    PRINTERR("%s TOTAL: %f ms\n",__FUNCTION__,stlSortTime);
#if 0
    std::cout << " STL vector: " << std::endl;
    for ( auto e : vecResult)
      std::cout << "[" << e.key << ", " << e.value << "] " ;
#endif
  }
  else{
    PRINTERR("%s ERROR !\n",__FUNCTION__);
    std::cout << "Merged vector: " << std::endl;
    for ( auto e : vecResult)
      std::cout << "[" << e.key << ", " << e.value << "] " ;

    std::cout << "\n Reference " << std::endl;
    for ( int i = 0; i < nb_elements ; i++ )
      std::cout << "[" << reference_array[i].key << ", " << reference_array[i].value << "] " ;
    std::cout << std::endl;
  }

}

int main(int argc, char *argv[])
{


    // READ INPUT PARAMETERS

   cimg_usage("Benchmark inserts elements in batches.");
   //const unsigned int nb_elements ( cimg_option("-n",100,"Number of elements to insert"));
   const unsigned int seg_size ( cimg_option("-s",8,"Segment size for the pma"));
   const int batch_size ( cimg_option("-b",10,"Batch size used in batched insertions"));
   const float tau_0 ( cimg_option("-t0",0.92,"pma parameter tau_0"));
   const float tau_h ( cimg_option("-th",0.7,"pma parameter tau_h"));
   const float rho_0 ( cimg_option("-r0",0.08,"pma parameter rho_0"));
   const float rho_h ( cimg_option("-rh",0.3,"pma parameter rho_0"));
   const unsigned int n_exp ( cimg_option("-x",1,"Number of repetitions of each experiment"));
   std::string fname ( cimg_option("-f","../../data/twitter/tweet1000.dat","file with tweets"));

   const char* is_help = cimg_option("-h",(char*)0,0);

   if (is_help) return 0;

   std::cout << "Input file: " << fname << std::endl;

   elttype *reference_array;

//   std::string fname = "/home/julio/Projects/Twitter-data/tweet1000";
   std::vector<tweet_t> tweet_vec;
   loadTweetFile(tweet_vec,fname);

   /* Create <key,value> elements */

   std::vector<elttype> input_vec;
   input_vec.reserve(tweet_vec.size());

   //use the language as key
   for (auto& tweet : tweet_vec){
       elttype e;
       e.key = tweet.language;
       e.value = tweet;
       input_vec.emplace_back(e);
   }

   reference_array = (elttype * ) malloc( (input_vec.size()) *sizeof(elttype));
   memcpy(reference_array, &input_vec[0], input_vec.size() * sizeof(elttype));
   qsort(reference_array, input_vec.size(), sizeof(elttype), comp<uint64_t>);


   PRINTOUT("Number of elements == %d \n",input_vec.size());

   /**** BECHMARKS  ****/
   for (int i = 0 ; i < n_exp; i++){
#if 0
     do_bench_benderPMA(&input_vec[0],input_vec.size(),reference_array,tau_0,tau_h,rho_0,rho_h,seg_size);

     do_bench_stlsort(&input_vec[0],input_vec.size(),batch_size,reference_array);

     do_bench_qsort(&input_vec[0],input_vec.size(),batch_size,reference_array);

     do_bench_batchPMA(&input_vec[0],input_vec.size(),batch_size,reference_array,tau_0,tau_h,rho_0,rho_h,seg_size);

#endif
     do_bench_mergesort(&input_vec[0],input_vec.size(),batch_size,reference_array);

   }
   free(reference_array);

   return 0;
}
