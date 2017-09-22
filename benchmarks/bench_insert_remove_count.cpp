/** @file
 * 1: Inserts a batch and deletes with a remove function
 * 2: Scan the whole pma.
 *
 */

#include "stde.h"
#include "types.h"
#include "input_it.h"

#include "PMQ.h"
#include "RTreeCtn.h"
#include "BTreeCtn.h"
#include "DenseCtn.h"

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

struct bench_t {
  // benchmark parameters
  uint32_t n_exp;
  uint32_t rate;

  uint64_t t_win, ctn_size, max_tree_size;

  bool dryrun;

  friend inline std::ostream &operator<<(std::ostream &out, const bench_t &p) {

    out << "Benchmark Parmeters: \n";
    out << " n_exp: " << p.n_exp << "\n";
    out << " rate: " << p.rate << "\n";
    out << " t_win: " << p.t_win << "\n";
    out << " ctn_size: " << p.ctn_size << "\n";
    out << " max_tree_size: " << p.max_tree_size << "\n";
    return out;
  }
};

using remove_function = std::function<int(const void *, uint64_t)>;
using remove_apply_function = std::function<int(const void *)>;

template<typename _Key, typename _Value>
int remove_elttype(const void *el, uint64_t oldest_time) {
  return ((std::pair<_Key, _Value> *) el)->second.getTime() < oldest_time;
}

template<typename T>
int remove_valuetype(const void *el, uint64_t oldest_time) {
  return ((T *) el)->getTime() < oldest_time;
}

// reads the full element
template<typename T>
void inline read_element(const T &el) {
  T volatile elemt = *(T *) &el;
}

template<typename T>
void inline count_element(uint32_t &accum, const spatial_t &, uint32_t count) {
  accum += count;
}

template<typename T, typename _It, typename _T>
void run_bench(int argc, char *argv[], _It it_begin, _It it_end,
               const bench_t &parameters, remove_function is_removed) {

  uint64_t temp_window = parameters.t_win;

  // calculates the inital ize based on insertion rate and temporal window (rate * temporal_window)
  uint64_t init_size = parameters.rate * parameters.t_win;

  // create container with the appropriate size
  std::unique_ptr < T > container = std::make_unique<T>(argc, argv);
  container->create((uint32_t) parameters.ctn_size);

  // we insert all the elements of the dataset, the container should delete the oldest to keep place for everything
  auto it_curr = it_begin;

  duration_t timer;

  //current time counter
  uint64_t t_now = temp_window;
  uint64_t oldest_time = 0;

  remove_apply_function _apply = std::bind(is_removed, std::placeholders::_1, std::ref(oldest_time));

  // INITIALIZE THE CONTAINER TO FILL IT UNTIL THE STEADY STATE
  it_curr = std::min(it_begin + init_size, it_end);

  // fills the time window
  std::vector<_T> initBatch(it_begin, it_curr);
  container->insert(initBatch);

  DBG_PRINTOUT("INIT container with %d elements\n", init_size);

  it_begin = it_curr;

  // Continues inserting by batches.
  while (it_begin != it_end) {
    it_curr = std::min(it_begin + parameters.rate, it_end);

    std::vector<_T> batch(it_begin, it_curr);

    if (t_now >= temp_window) {
      oldest_time++;
    }

    DBG_PRINTOUT("Removing elements with timestamp < %u and insert one batch\n", oldest_time);

    if (!parameters.dryrun) {
      timer = container->insert_rm(batch, _apply);
    }

    // ========================================
    // Count elements on the container
    if (!parameters.dryrun) {
      PRINTBENCH_PTR(temp_window, t_now, "count", container->size(), timer);
    } else {
      PRINTBENCH_PTR("dryrun", temp_window, t_now, "count", container->size(), 0);
    }

    // update iterator
    it_begin = it_curr;

    t_now++;
  }
}

int main(int argc, char *argv[]) {
  bench_t parameters;

  cimg_usage("Queries Benchmark inserts elements in batches.");

  const unsigned int nb_elements(cimg_option("-n", 0, "Number of elements to insert"));
  const long seed(cimg_option("-r", 0, "Random seed to generate elements"));

  std::string fname(cimg_option("-f", "", "file with tweets to load"));

  parameters.rate = (cimg_option("-rate", 1000, "Rate (elements per batch) for insertions"));
  parameters.t_win = (cimg_option("-T", 10800, "Temporal window Size"));
  parameters.max_tree_size = (cimg_option("-tSize", 10800000, "Max size for the Btree and Rtree"));

  parameters.dryrun = (cimg_option("-dry", false, "Dry run"));

  const char *is_help = cimg_option("-h", (char *) 0, 0);
  if (is_help) return false;

  if (!fname.empty()) {
    using el_t = TweetDatType;
    using it_t = input_file_it<el_t>;

    PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());
    std::shared_ptr < std::ifstream > file_ptr = std::make_shared<std::ifstream>(fname, std::ios::binary);
    auto begin = it_t::begin(file_ptr);
    auto end = it_t::end(file_ptr);
    PRINTOUT("%d teewts loaded \n", end - begin);

    // parameters setup
    parameters.ctn_size = parameters.rate * parameters.t_win;
    run_bench<PMQBinary<el_t>, it_t, el_t>(argc, argv, begin, end, parameters, remove_elttype<uint64_t, el_t>);

    // parameters setup
    parameters.ctn_size = parameters.max_tree_size;
    run_bench<BTreeCtn<el_t>, it_t, el_t>(argc, argv, begin, end, parameters, remove_valuetype<el_t>);
    run_bench<RTreeCtn<el_t, bgi::quadratic < 16>>, it_t, el_t>(argc, argv, begin, end, parameters, remove_valuetype<el_t>);
    run_bench<DenseCtn<el_t>, it_t, el_t>(argc, argv, begin, end, parameters, remove_elttype<uint64_t, el_t>);

  } else {
    // 16 bytes + N
    static const size_t N = 0;
    using el_t = GenericType<N>;
    using it_t = input_random_it<N>;

    PRINTOUT("Generate random keys...\n");
    auto begin = it_t::begin(seed, parameters.rate);
    auto end = it_t::end(seed, parameters.rate, nb_elements);
    PRINTOUT("%d teewts generated \n", end - begin);

    // parameters setup
    parameters.ctn_size = parameters.rate * parameters.t_win;
    run_bench<PMQBinary<el_t>, it_t, el_t>(argc, argv, begin, end, parameters, remove_elttype<uint64_t, el_t>);

    // parameters setup
    parameters.ctn_size = parameters.max_tree_size;
    run_bench<BTreeCtn<el_t>, it_t, el_t>(argc, argv, begin, end, parameters, remove_valuetype<el_t>);
    run_bench<RTreeCtn<el_t, bgi::quadratic < 16>>, it_t, el_t>(argc, argv, begin, end, parameters, remove_valuetype<el_t>);
    run_bench<DenseCtn<el_t>, it_t, el_t>(argc, argv, begin, end, parameters, remove_elttype<uint64_t, el_t>);
  }

  return EXIT_SUCCESS;
}
