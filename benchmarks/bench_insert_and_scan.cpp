/** @file
 * Benchmark for the query time
 *
 * - Measures the time for inserting each batch
 * - After each batch insertion measures the time to scan the whole array
 *
 */

#include "stde.h"
#include "types.h"
#include "input_it.h"

//#include "RTreeCtn.h"
//#include "BTreeCtn.h"

#include "PMQ.h"
//#include "ImplicitDenseVectorCtn.h"

#define PRINTBENCH(...) do { \
   std::cout << "InsertionBench " << container.name() << " ; ";\
   printcsv( __VA_ARGS__ ) ; \
   std::cout << std::endl ;\
} while (0)

#define PRINTBENCH_PTR(...) do { \
   std::cout << "InsertionBench " << container->name() << " ; ";\
   printcsv( __VA_ARGS__ ) ; \
   std::cout << std::endl ;\
} while (0)

struct bench_t {
  // benchmark parameters
  uint32_t n_exp;
  uint32_t batch_size;
};

uint32_t g_Quadtree_Depth = 25;

// reads the full element
template<typename T>
void inline read_element(const T &el) {
  T volatile elemt = *(T *) &el;
}

template<typename T>
void inline count_element(uint32_t &accum, const spatial_t &, uint32_t count) {
  accum += count;
}

template<typename T, typename Tp>
void inline run_queries(T &container, const region_t &region, uint32_t id, const bench_t &parameters) {

  uint32_t count = 0;
  typename GeoCtnIntf<Tp>::scantype_function
      _scan = std::bind([](uint32_t &accum, const Tp &el) {
    accum++;
  }, std::ref(count), std::placeholders::_1);

  typename GeoCtnIntf<Tp>::applytype_function
      _apply = std::bind([](uint32_t &accum, const spatial_t &, uint32_t count) {
    accum += count;
  }, std::ref(count), std::placeholders::_1, std::placeholders::_2);

  duration_t timer;

  // 1 - gets the minimum set of nodes that are inside the queried region
  // QueryRegion will traverse the tree and return the intervals to query;
  // NOTE: when comparing with the quadtree with pointer to elements the scan will be the traversall on the tree.

  // warm up
  container.scan_at_region(region, _scan);

  // access the container to count the number of elements inside the region
  for (uint32_t i = 0; i < parameters.n_exp; i++) {
    count = 0;
    timer = container.scan_at_region(region, _scan);

    PRINTBENCH(id, timer, "count", count);
  }

  count = 0;
  timer = container.apply_at_region(region, _apply);
  PRINTBENCH(id, timer, "count", count);
}

template<typename T, typename _It, typename _T>
void run_bench(int argc, char *argv[], _It it_begin, _It it_end, const bench_t &parameters) {
  //create container
  std::unique_ptr < T > container = std::make_unique<T>(argc, argv);
  container->create(it_end - it_begin);

  auto it_curr = it_begin;

  duration_t timer;

  int id = 0;
  while (it_begin != it_end) {
    it_curr = std::min(it_begin + parameters.batch_size, it_end);

    std::vector<_T> batch(it_begin, it_curr);

    // insert batch
    timer = container->insert(batch);

    PRINTBENCH_PTR(id, timer);

    // update iterator
    it_begin = it_curr;

    // ========================================
    // Performs global scan
    // ========================================

    //Run a scan on the whole array
    run_queries<T, _T>((*container.get()), region_t(0, 0, 0, 0, 0), id, parameters);

    ++id;
  }
}

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

int main(int argc, char *argv[]) {
  bench_t parameters;

  cimg_usage("Queries Benchmark inserts elements in batches.");

  const unsigned int nb_elements(cimg_option("-n", 0, "Number of elements to read / generate randomly"));
  const long seed(cimg_option("-r", 0, "Random seed to generate elements"));

  std::string fname(cimg_option("-f", "", "File with tweets to load"));

  parameters.batch_size = (cimg_option("-b", 100, "Batch size used in batched insertions"));
  parameters.n_exp = (cimg_option("-x", 1, "Number of repetitions of each experiment"));

  const char *is_help = cimg_option("-h", (char *) 0, 0);
  if (is_help) return false;

  const uint32_t quadtree_depth = 25;

  //std::unique_ptr < input_random_it > begin, end;

  if (!fname.empty()) {
    /*PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());

    std::shared_ptr<std::ifstream> file_ptr = std::make_shared<std::ifstream>(fname, std::ios::binary);

    begin = std::make_unique<input_tweet_it>(file_ptr, false);
    end = std::make_unique<input_tweet_it>(file_ptr, true);

    PRINTOUT("%d teewts loaded \n", (uint32_t) begin->size());*/

  } else {
    PRINTOUT("Generate random keys...\n");

    auto begin = input_random_it::begin(seed, parameters.batch_size);
    auto end = input_random_it::end(seed, parameters.batch_size, nb_elements);

    PRINTOUT("%d teewts generated \n", end - begin);

    run_bench<PMQBinary<GenericType>, input_random_it, GenericType>(argc, argv, begin, end, parameters);
  }


  // run_bench<GeoHashSequential>(argc, argv, input, parameters);
//  run_bench<PMQBinary<GenericType>, input_random_it, GenericType>(argc, argv, (*begin), (*end), parameters);

  //run_bench<ImplicitDenseVectorCtn>(argc, argv, (*begin), (*end), parameters);

  //run_bench<BTreeCtn>(argc, argv, (*begin), (*end), parameters);

  //run_bench<RTreeCtn<bgi::quadratic < 16>> > (argc, argv, (*begin), (*end), parameters);

  return EXIT_SUCCESS;
}
