/** @file
 * Benchmark for the query time
 * Loads a file of queries to execute
 *
 *
 */

#include "stde.h"
#include "types.h"
#include "InputIntf.h"
#include "string_util.h"

#include "RTreeCtn.h"
#include "BTreeCtn.h"

#include "GeoHash.h"
#include "RTreeCtn.h"
#include "PMABatchCtn.h"
#include "PostGisCtn.h"
#include "SpatiaLiteCtn.h"
#include "DenseVectorCtn.h"

#define PRINTBENCH(...) do { \
   std::cout << "QueryBench " << container.name() << " ; ";\
   printcsv( __VA_ARGS__ ) ; \
   std::cout << std::endl ;\
} while (0)

/*#define PRINTBENCH( ... ) do { \
} while (0)*/

struct center_t {
  center_t() = default;
  center_t(float _lat, float _lon) : lat(_lat), lon(_lon) {
  }

  float lat, lon;
};

struct bench_t {
  float def_r, min_r, max_r, inc_r;
  uint64_t min_t, max_t, inc_t;

  // benchmark parameters
  uint32_t n_exp;
  uint64_t rate;
  bool dryrun;
};

uint32_t g_Quadtree_Depth = 25;

void inline count_element(uint32_t &accum, const spatial_t &, uint32_t count) {
  accum += count;
}

// reads the full element
void inline read_element(const valuetype &el) {
  valuetype volatile elemt = *(valuetype *) &el;
}

template<typename T>
void inline run_queries(T &container, const center_t &center, uint32_t id, uint64_t t, const bench_t &parameters) {

  Timer timer;

  uint32_t count = 0;
  applytype_function _apply = std::bind(count_element, std::ref(count),
                                        std::placeholders::_1, std::placeholders::_2);

  for (float r = parameters.min_r; r <= parameters.max_r; r += parameters.inc_r) {
    region_t region(center.lat, center.lon, r);

    // warm up
    if (!parameters.dryrun) container.scan_at_region(region, read_element);

    for (uint32_t i = 0; i < parameters.n_exp; i++) {
      timer.start();
      if (!parameters.dryrun) container.scan_at_region(region, read_element);
      timer.stop();
      PRINTBENCH("scan_at_region", id, t, r, timer.milliseconds(), "ms");
    }
  }

#if 0
  for (float r = parameters.min_r; r <= parameters.max_r; r += parameters.inc_r) {
    region_t region(center.lat, center.lon, r);

    // warm up
    if (!parameters.dryrun) container.apply_at_tile(region, _apply);

    for (uint32_t i = 0; i < parameters.n_exp; i++) {
      count = 0;
      timer.start();
      if (!parameters.dryrun) container.apply_at_tile(region, _apply);
      timer.stop();
      PRINTBENCH("apply_at_tile", id, t, r, timer.milliseconds(), "ms");
    }
  }
#endif

  for (float r = parameters.min_r; r <= parameters.max_r; r += parameters.inc_r) {
    region_t region(center.lat, center.lon, r);

    // warm up
    if (!parameters.dryrun) container.apply_at_region(region, _apply);

    for (uint32_t i = 0; i < parameters.n_exp; i++) {
      count = 0;
      timer.start();
      if (!parameters.dryrun) container.apply_at_region(region, _apply);
      timer.stop();
      PRINTBENCH("apply_at_region", id, t, r, timer.milliseconds(), "ms", count);
    }
  }
}

template<typename T>
void run_bench(int argc,
               char *argv[],
               const std::vector<elttype> &input,
               const std::vector<center_t> &queries,
               const bench_t &parameters) {

  Timer timer;

  uint32_t count = 0;
  applytype_function _apply = std::bind(count_element, std::ref(count),
                                        std::placeholders::_1, std::placeholders::_2);

  for (uint64_t t = parameters.min_t; t <= parameters.max_t; t += parameters.inc_t) {

    // calculates ctn size based on insertion rate and temporal window (rate * temporal_window)
    uint64_t ctn_size = std::min((uint64_t) input.size(), parameters.rate * t);

    //create container
    std::unique_ptr < T > container = std::make_unique<T>(argc, argv);
    container->create((uint32_t) ctn_size);

    std::vector<elttype> batch(input.begin(), input.begin() + ctn_size);

    // insert all elements as a single batch
    timer.start();
    if (!parameters.dryrun) container->insert(batch);
    timer.stop();
    std::cout << "QueryBench " << container->name() << " ; ";
    printcsv("insert", timer.milliseconds(), "ms");
    std::cout << std::endl;

    //Run a scan on the whole array
    timer.start();
    container->apply_at_region(region_t(0,0,0,0,0), _apply);
    timer.stop();
    std::cout << "QueryBench " << container->name() << " ; ";
    printcsv("Global Apply", timer.milliseconds(), "ms", count);
    std::cout << std::endl;

    // Perform custom queries
    for (uint32_t id = 0; id < queries.size(); id++) {
      run_queries((*container.get()), queries[id], id, t, parameters);
    }

    // delete container
    container.reset();
  }
}

void load_bench_file(const std::string &file, std::vector<center_t> &queries, int32_t n_queries) {
  PRINTOUT("Loading log file: %s \n", file.c_str());

  std::ifstream infile(file, std::ios::in | std::ifstream::binary);

  // number of elts - header
  uint32_t elts = 0;
  infile.read((char *) &elts, sizeof(uint32_t));
  PRINTOUT("Max queries in file: %d\n", elts);

  if (n_queries > 0) {
    elts = std::min(elts, (uint32_t) n_queries);
  }
  queries.resize(elts);

  infile.read(reinterpret_cast<char *>(&queries[0]), sizeof(center_t) * elts);

  infile.close();

  PRINTOUT(" %d queries loaded. \n", (uint32_t) queries.size());
}

int main(int argc, char *argv[]) {
  bench_t parameters;

  cimg_usage("Queries Benchmark inserts elements in batches.");

  std::string fname(cimg_option("-f", "", "File with tweets"));
  const long seed(cimg_option("-seed", 123, "Random seed to generate elements"));

  int32_t n_queries(cimg_option("-n_queries", -1, "Number of queries"));
  std::string bench_file(cimg_option("-bf", "./data/checkins_global.dat", "file with logs"));

  parameters.rate = (cimg_option("-rate", 1000, "Insertion rate"));
  parameters.n_exp = (cimg_option("-x", 1, "Number of repetitions of each experiment"));

  parameters.def_r = (cimg_option("-def_r", 30.f, "R: Default value (radius in km)"));
  parameters.min_r = (cimg_option("-min_r", 0.25f, "R: Min"));
  parameters.max_r = (cimg_option("-max_r", 400.1f, "R: Max"));
  parameters.inc_r = (cimg_option("-inc_r", 39.975f, "R: Increment"));

  parameters.min_t = (cimg_option("-min_t", 10800, "T: Min"));
  parameters.max_t = (cimg_option("-max_t", 43200, "T: Max"));
  parameters.inc_t = (cimg_option("-inc_t", 10800, "T: Increment"));

  parameters.dryrun = (cimg_option("-dry", false, "Dry run"));

  uint64_t n_elts = parameters.rate * parameters.max_t;

  const char *is_help = cimg_option("-h", (char *) 0, 0);
  if (is_help) return false;

  const uint32_t quadtree_depth = 25;

  std::vector<elttype> input;

  if (parameters.dryrun) PRINTOUT("==== DRY RUN ====\n");

  if (!fname.empty()) {
    PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());
    input = input::load(fname, quadtree_depth, parameters.rate, n_elts);
    PRINTOUT(" %d teewts loaded \n", (uint32_t) input.size());
  } else {
    PRINTOUT("Generate random keys..");
    input = input::dist_random(n_elts, seed, parameters.rate);
    PRINTOUT(" %d teewts generated \n", (uint32_t) input.size());
  }

  std::vector<center_t> queries;
  load_bench_file(bench_file, queries, n_queries);

  std::cout << "Input File" << std::endl;
  for (auto &elt : input ){
      std::cout << "[" << elt.key << "] " << elt.value << std::endl;
  }

  // run_bench<GeoHashSequential>(argc, argv, input, queries, parameters);
  run_bench<GeoHashBinary>(argc, argv, input, queries, parameters);
  run_bench<BTreeCtn>(argc, argv, input, queries, parameters);
  run_bench<RTreeCtn<bgi::rstar<16>>>(argc, argv, input, queries, parameters);
  //run_bench<RTreeCtn<bgi::quadratic<16>>>(argc, argv, input, queries, parameters);

  return EXIT_SUCCESS;
}
