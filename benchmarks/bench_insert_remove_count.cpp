/** @file
 * 1: Inserts a batch and deletes with a remove function
 * 2: Scan the whole pma.
 *
 */

#include "stde.h"
#include "types.h"
#include "InputIntf.h"
#include "string_util.h"

#include "GeoHash.h"
#include "PMABatchCtn.h"
#include "PostGisCtn.h"
#include "SpatiaLiteCtn.h"
#include "DenseVectorCtn.h"

#define PRINTBENCH( ... ) do { \
   std::cout << "InsertionRemoveBench " << container.name() << " ; ";\
   printcsv( __VA_ARGS__ ) ; \
   std::cout << std::endl ;\
} while (0)

#define PRINTBENCH_PTR( ... ) do { \
   std::cout << "InsertionRemoveBench " << container->name() << " ; ";\
   printcsv( __VA_ARGS__ ) ; \
   std::cout << std::endl ;\
} while (0)

/*#define PRINTBENCH( ... ) do { \
} while (0)*/

struct bench_t {
   // benchmark parameters
   uint32_t n_exp;
   uint32_t rate;
   uint64_t def_t, min_t, max_t, inc_t;

   bool dryrun;

};

uint32_t g_Quadtree_Depth = 25;

// reads the full element
void inline read_element(const valuetype& el) {
   valuetype volatile elemt = *(valuetype*)&el;
}

// counts the amount of elements
void inline count_element(uint32_t& accum, const spatial_t&, uint32_t count) {
   accum += count;
}

template <typename T>
void run_bench(int argc, char* argv[], const std::vector<elttype>& input, const bench_t& parameters) {

    for (uint64_t temp_window = parameters.min_t; temp_window <= parameters.max_t; temp_window += parameters.inc_t) {


       // calculates ctn size based on insertion rate and temporal window (rate * temporal_window)
       uint64_t ctn_size = std::min((uint64_t)input.size(), parameters.rate * temp_window);


       //create container
       std::unique_ptr<T> container = std::make_unique<T>(argc, argv);
       container->create((uint32_t)ctn_size);

       std::vector<elttype>::const_iterator it_begin = input.begin();
       std::vector<elttype>::const_iterator it_curr = input.begin();
       std::vector<elttype>::const_iterator it_end = input.begin() + ctn_size;

       duration_t timer;

       uint64_t t_now = 0; //current time counter
       uint64_t oldest_time = 0;
       while (it_begin != it_end) {
          it_curr = std::min(it_begin + parameters.rate, it_end);

          std::vector<elttype> batch(it_begin, it_curr);

          if (t_now > temp_window) {
             oldest_time++;
          }

          DBG_PRINTOUT("Removing with oldest time %u\n", oldest_time);

          if (!parameters.dryrun) {
             timer = container->insert_rm(batch, [ oldest_time ]( const void* el) {
                return ((elttype*)el)->value.time < oldest_time;
             });
          }

          // ========================================
          // Count elements on the container
          uint32_t count = 0;
          applytype_function count_element_wrapper = std::bind(count_element, std::ref(count),
                                                               std::placeholders::_1, std::placeholders::_2);

          if (!parameters.dryrun) {
             container->apply_at_region( region_t(0, 0, 0, 0, 0) , count_element_wrapper);

             for (auto& info : timer) {
                PRINTBENCH_PTR(info.name, parameters.rate, temp_window, t_now, info.duration, "ms", count);
             }
          }else{
                PRINTBENCH_PTR("dryrun", parameters.rate, temp_window, t_now, 0, "ms", count);
          }

          // update iterator
          it_begin = it_curr;

          ++t_now;
       }
    }
}

int main(int argc, char* argv[]) {
   bench_t parameters;

   cimg_usage("Queries Benchmark inserts elements in batches.");

   const unsigned int nb_elements(cimg_option("-n", 0, "Number of elements to generate randomly"));
   const long seed(cimg_option("-r", 0, "Random seed to generate elements"));

   std::string fname(cimg_option("-f", "./data/tweet100.dat", "file with tweets to load"));


//   parameters.n_exp = (cimg_option("-x", 1, "Number of repetitions of each experiment"));

   parameters.rate = (cimg_option("-rate", 1000, "Rate (elements per batch) for insertions"));

   parameters.min_t = (cimg_option("-min_t", 10800, "Temporal window: Min"));
   parameters.max_t = (cimg_option("-max_t", 43200, "T: Max"));
   parameters.inc_t = (cimg_option("-inc_t", 10800, "T: Increment"));

   parameters.dryrun = (cimg_option("-dry", false, "Dry run"));

   const char* is_help = cimg_option("-h", (char*)0, 0);
   if (is_help) return false;

   const uint32_t quadtree_depth = 25;

   uint64_t n_elts = parameters.rate * parameters.max_t;

   std::vector<elttype> input;

   if (nb_elements == 0) {
      PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());
      input = input::load(fname, quadtree_depth,parameters.rate,n_elts);
      PRINTOUT("%d teewts loaded \n", (uint32_t)input.size());
   } else {
      PRINTOUT("Generate random keys...\n");
      //Use the batch id as timestamp
      input = input::dist_random(nb_elements, seed, parameters.rate);
      PRINTOUT("%d teewts generated \n", (uint32_t)input.size());
   }
#ifndef NDEBUG
   for (elttype& e : input) {
      std::cout << "[" << e.key << "," << e.value.time << "] \n";
   }
#endif

   //run_bench<PMABatchCtn>(argc, argv, input, parameters);
   //run_bench<GeoHashSequential>(argc, argv, input, parameters);
   run_bench<GeoHashBinary>(argc, argv, input, parameters);
   //run_bench<DenseCtnStdSort>(argc, argv, input, parameters);
   //run_bench<DenseCtnTimSort>(argc, argv, input, parameters);
   //run_bench<SpatiaLiteCtn>(argc, argv, input, parameters);
   //run_bench<PostGisCtn>(argc, argv, input, parameters);

   return EXIT_SUCCESS;
}
