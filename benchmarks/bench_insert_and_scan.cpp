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

#include "PMQ.h"
#include "RTreeCtn.h"
#include "BTreeCtn.h"
#include "DenseCtn.h"

#include "benchmarkconfig.h"

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
    run_queries<T, _T>((*container.get()), region_t(+90.f, -180.f, -90.f, +180.f), id, parameters);

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

  std::string fname(cimg_option("-f", "", "File with tweets to load (.dat): with metadata."));

  std::string fname_dmp(cimg_option("-d", "", "File with tweets to load (.dmp): with tweet text."));

  parameters.batch_size = (cimg_option("-b", 100, "Batch size used in batched insertions"));
  parameters.n_exp = (cimg_option("-x", 1, "Number of repetitions of each experiment"));

  const char *is_help = cimg_option("-h", (char *) 0, 0);
  if (is_help) return false;

  if (!fname.empty()) {
    using el_t = TweetDatType;
    using it_t = input_file_it<el_t>;

    PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());
    std::shared_ptr < std::ifstream > file_ptr = std::make_shared<std::ifstream>(fname, std::ios::binary);
    auto begin = it_t::begin(file_ptr);
    auto end = nb_elements == 0 ? it_t::end(file_ptr) : it_t::end(file_ptr, nb_elements);
    PRINTOUT("%d tweets loaded \n", end - begin);

#ifdef BENCH_PMQ
    run_bench<PMQBinary<el_t>, it_t, el_t>(argc, argv, begin, end, parameters);
#endif
#ifdef BENCH_BTREE
    run_bench<BTreeCtn<el_t>, it_t, el_t>(argc, argv, begin, end, parameters);
#endif
#ifdef BENCH_RTREE
    run_bench<RTreeCtn<el_t, bgi::quadratic < 16>> , it_t, el_t>(argc, argv, begin, end, parameters);
#endif
#ifdef BENCH_DENSE
    run_bench<DenseCtn<el_t>, it_t, el_t>(argc, argv, begin, end, parameters);
#endif

  } else if (!fname_dmp.empty()) {
    using el_t = TweetDmpType;
    using it_t = input_file_it<el_t>;

    PRINTOUT("Loading twitter dataset... %s \n", fname_dmp.c_str());
    std::shared_ptr < std::ifstream > file_ptr = std::make_shared<std::ifstream>(fname_dmp, std::ios::binary);
    auto begin = it_t::begin(file_ptr);
    auto end = nb_elements == 0 ? it_t::end(file_ptr) : it_t::end(file_ptr, nb_elements);
    PRINTOUT("%d tweets loaded \n", end - begin);

#ifdef BENCH_PMQ
    run_bench<PMQBinary<el_t>, it_t, el_t>(argc, argv, begin, end, parameters);
#endif
#ifdef BENCH_BTREE
    run_bench<BTreeCtn<el_t>, it_t, el_t>(argc, argv, begin, end, parameters);
#endif
#ifdef BENCH_RTREE
    run_bench<RTreeCtn<el_t, bgi::quadratic < 16>> , it_t, el_t>(argc, argv, begin, end, parameters);
#endif
#ifdef BENCH_DENSE
    run_bench<DenseCtn<el_t>, it_t, el_t>(argc, argv, begin, end, parameters);
#endif

  } else {
    // 16 bytes + N
    static const size_t N = 0;
    using el_t = GenericType<N>;
    using it_t = input_random_it<N>;

    PRINTOUT("Generate random keys...\n");
    auto begin = it_t::begin(seed, parameters.batch_size);
    auto end = it_t::end(seed, parameters.batch_size, nb_elements);
    PRINTOUT("%d tweets generated \n", end - begin);

#ifdef BENCH_PMQ
    run_bench<PMQBinary<el_t>, it_t, el_t>(argc, argv, begin, end, parameters);
#endif
#ifdef BENCH_BTREE
    run_bench<BTreeCtn<el_t>, it_t, el_t>(argc, argv, begin, end, parameters);
#endif
#ifdef BENCH_RTREE
    run_bench<RTreeCtn<el_t, bgi::quadratic < 16>> , it_t, el_t>(argc, argv, begin, end, parameters);
#endif
#ifdef BENCH_DENSE
    run_bench<DenseCtn<el_t>, it_t, el_t>(argc, argv, begin, end, parameters);
#endif
  }

  return EXIT_SUCCESS;
}
