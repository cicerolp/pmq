/** @file
 * 1: Inserts a batch and deletes with a remove function
 * 2: Scan the whole pma.
 *
 */

#include "stde.h"
#include "types.h"
#include "InputIntf.h"
#include "string_util.h"

#include "RTreeCtn.h"
#include "BTreeCtn.h"

#include "GeoHash.h"
#include "PMABatchCtn.h"
#include "PostGisCtn.h"
#include "SpatiaLiteCtn.h"
#include "DenseVectorCtn.h"

#define PRINTBENCH(...) do { \
   std::cout << "InsertionRemoveBench " << container.name() << " ; ";\
   printcsv( __VA_ARGS__ ) ; \
   std::cout << std::endl ;\
} while (0)

#define PRINTBENCH_PTR(...) do { \
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
void inline read_element(const valuetype &el) {
  valuetype volatile elemt = *(valuetype *) &el;
}

// counts the amount of elements
void inline count_element(uint32_t &accum, const spatial_t &, uint32_t count) {
  accum += count;
}

template<typename T>
void run_bench(int argc, char *argv[], const std::vector<elttype> &input, const bench_t &parameters) {

  for (uint64_t temp_window = parameters.min_t; temp_window <= parameters.max_t; temp_window += parameters.inc_t) {



    // calculates ctn size based on insertion rate and temporal window (rate * temporal_window)
    uint64_t ctn_size = parameters.rate * temp_window;


    //create container with the size of temporal window
    std::unique_ptr < T > container = std::make_unique<T>(argc, argv);
    container->create((uint32_t) ctn_size);

    //We insert all the elements of the dataset, the container should delete the oldest to keep place for everything
    std::vector<elttype>::const_iterator it_begin = input.begin();
    std::vector<elttype>::const_iterator it_curr = input.begin();
    std::vector<elttype>::const_iterator it_end = input.end();

    duration_t timer;

    uint64_t t_now = temp_window; //current time counter
    uint64_t oldest_time = 0;

    // INITIALIZE THE CONTAINER TO FILL IT UNTIL THE STEADY STATE
    // Fills the time window
    it_curr = std::min(it_begin + ctn_size, it_end);
    std::vector<elttype> initBatch(it_begin, it_curr);
    container->insert_rm(initBatch, [oldest_time](const void *el) {
      return ((elttype *) el)->value.time < oldest_time;
    });

    it_begin = it_curr;
    t_now++;

    // Continues inserting by batches.
    while (it_begin != it_end) {
      it_curr = std::min(it_begin + parameters.rate, it_end);

      std::vector<elttype> batch(it_begin, it_curr);

      if (t_now > temp_window) {
        oldest_time++;
      }

      DBG_PRINTOUT("Removing with oldest time %u\n", oldest_time);

      if (!parameters.dryrun) {
        timer = container->insert_rm(batch, [oldest_time](const void *el) {
          return ((elttype *) el)->value.time < oldest_time;
        });
      }

      // ========================================
      // Count elements on the container
      if (!parameters.dryrun) {
        for (auto &info : timer) {
          PRINTBENCH_PTR(info.name, parameters.rate, temp_window, t_now, info.duration, "ms", container->size());
        }
      } else {
        PRINTBENCH_PTR("dryrun", parameters.rate, temp_window, t_now, 0, "ms", container->size());
      }

      // update iterator
      it_begin = it_curr;

      ++t_now;
    }
  }
}

int main(int argc, char *argv[]) {
  bench_t parameters;

  cimg_usage("Queries Benchmark inserts elements in batches.");

  const unsigned int nb_elements(cimg_option("-n", 0, "Number of elements to insert"));
  const long seed(cimg_option("-r", 0, "Random seed to generate elements"));

  std::string fname(cimg_option("-f", "", "file with tweets to load"));


//   parameters.n_exp = (cimg_option("-x", 1, "Number of repetitions of each experiment"));

  parameters.rate = (cimg_option("-rate", 1000, "Rate (elements per batch) for insertions"));

  parameters.min_t = (cimg_option("-min_t", 10800, "Temporal window: Min"));
  parameters.max_t = (cimg_option("-max_t", 43200, "T: Max"));
  parameters.inc_t = (cimg_option("-inc_t", 10800, "T: Increment"));

  parameters.dryrun = (cimg_option("-dry", false, "Dry run"));

  const char *is_help = cimg_option("-h", (char *) 0, 0);
  if (is_help) return false;

  const uint32_t quadtree_depth = 25;

//   uint64_t n_elts = parameters.rate * parameters.max_t;
// We want to load everything, not just the timewindow.

  std::vector<elttype> input;

  if (fname != "") {
    PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());
    if (nb_elements != 0)
      input = input::load(fname, quadtree_depth, parameters.rate, nb_elements);
    else
      input = input::load(fname, quadtree_depth, parameters.rate);
    PRINTOUT("%d tweets loaded \n", (uint32_t) input.size());
  } else {
    PRINTOUT("Generate random keys...\n");
    //Use the batch id as timestamp
    input = input::dist_random(nb_elements, seed, parameters.rate);
    PRINTOUT("%d teewts generated \n", (uint32_t) input.size());
  }
#ifndef NDEBUG
  for (elttype &e : input) {
    std::cout << "[" << e.key << "," << e.value.time << "] \n";
  }
#endif

  run_bench<GeoHashSequential>(argc, argv, input, parameters);
  run_bench<GeoHashBinary>(argc, argv, input, parameters);

  run_bench<BTreeCtn>(argc, argv, input, parameters);

  run_bench<RTreeCtn<bgi::rstar<16>>>(argc, argv, input, parameters);

  return EXIT_SUCCESS;
}
