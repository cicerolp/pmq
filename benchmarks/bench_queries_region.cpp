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

#define PRINTBENCH_PTR(...) do { \
   std::cout << "QueryBench " << container->name() << " ; ";\
   printcsv( __VA_ARGS__ ) ; \
   std::cout << std::endl ;\
} while (0)

struct bench_t {
  uint64_t min_t, max_t, inc_t;

  // benchmark parameters
  uint32_t n_exp;
  uint64_t rate;
  bool dryrun;

  int refLevel;

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
void inline run_queries(T &container, const region_t &region, uint32_t id, uint64_t t, const bench_t &parameters) {

  duration_t timer;

  uint32_t count = 0;
  applytype_function _apply = std::bind(count_element, std::ref(count),
                                        std::placeholders::_1, std::placeholders::_2);

  if (!parameters.dryrun) {
    // scan_at_region

    // warm up
    container.scan_at_region(region, read_element);

    for (uint32_t i = 0; i < parameters.n_exp; i++) {
      timer = container.scan_at_region(region, read_element);
      PRINTBENCH("query", id, timer);
    }

    // apply_at_region

    // warm up
    container.apply_at_region(region, _apply);

    for (uint32_t i = 0; i < parameters.n_exp; i++) {
      count = 0;
      timer = container.apply_at_region(region, _apply);
      PRINTBENCH("query", id, timer, "count", count);
    }
  }
}

template<typename T>
void run_bench(int argc,
               char *argv[],
               const std::vector<elttype> &input,
               const std::vector<region_t> &queries,
               const bench_t &parameters) {

  duration_t timer;

  uint32_t count = 0;
  applytype_function _apply = std::bind(count_element, std::ref(count),
                                        std::placeholders::_1, std::placeholders::_2);

  for (uint64_t t = parameters.min_t; t <= parameters.max_t; t += parameters.inc_t) {

    // calculates ctn size based on insertion rate and temporal window (rate * temporal_window)
    uint64_t ctn_size = std::min((uint64_t) input.size(), parameters.rate * t);

    //create container
    std::unique_ptr < T > container = std::make_unique<T>(argc, argv,parameters.refLevel);
    timer = container->create((uint32_t) ctn_size);

    PRINTBENCH_PTR("init", timer);

    std::vector<elttype> batch(input.begin(), input.begin() + ctn_size);

    // insert all elements as a single batch
    if (!parameters.dryrun) {
      timer = container->insert(batch);
      PRINTBENCH_PTR("init", timer);

      // run a count on the whole array
      timer = container->apply_at_region(region_t(0, 0, 0, 0, 0), _apply);
      PRINTBENCH_PTR("init", timer, "count", count);
    }

    // perform custom queries
    for (uint32_t id = 0; id < queries.size(); id++) {
      run_queries((*container.get()), queries[id], id, t, parameters);
    }

    // delete container
    container.reset();
  }
}

// reads a csv file containing (lat0, lon0, lat1, lon1) of a bounding box
void load_bench_file(const std::string &file, std::vector<region_t> &queries,
                     int32_t n_queries = std::numeric_limits<uint32_t>::max()) {
  PRINTOUT("Loading log file: %s \n", file.c_str());

  std::ifstream in(file.c_str());
  if (!in.is_open()) {
    return;
  }

  typedef boost::tokenizer<boost::escaped_list_separator<char>> Tokenizer;

  std::string line;

  while (n_queries--) {
    std::getline(in, line);

    if (in.eof()) break;

    Tokenizer tok(line);
    auto it = tok.begin();

    float lat0 = std::stof(*(it++));
    float lon0 = std::stof(*(it++));
    float lat1 = std::stof(*(it++));
    float lon1 = std::stof(*(it++));

    queries.emplace_back(lat0, lon0, lat1, lon1);
  }

  in.close();

  PRINTOUT("%d queries loaded.\n", (uint32_t) queries.size());
}

int main(int argc, char *argv[]) {
  bench_t parameters;

  cimg_usage("bench_queries_region");

  std::string fname(cimg_option("-f", "", "File with tweets (if NULL, generates Rate X T elemets "));
  const long seed(cimg_option("-seed", 123, "Random seed to generate elements"));

  int32_t n_queries(cimg_option("-n_queries", -1, "Number of queries"));
  std::string bench_file(cimg_option("-bf", "./data/benchmark_debug.csv", "Benchmark file"));

  parameters.rate = (cimg_option("-rate", 1000, "Insertion rate"));
  parameters.n_exp = (cimg_option("-x", 1, "Number of repetitions of each experiment"));

  parameters.min_t = (cimg_option("-min_t", 10800, "T: Min"));
  parameters.max_t = (cimg_option("-max_t", 43200, "T: Max"));
  parameters.inc_t = (cimg_option("-inc_t", 10800, "T: Increment"));

  parameters.refLevel = (cimg_option("-ref", 8, "Number of quadtree refinements used in the queries"));
  parameters.dryrun = (cimg_option("-dry", false, "Dry run"));

  uint64_t n_elts = parameters.rate * parameters.max_t;

  const char *is_help = cimg_option("-h", (char *) 0, 0);
  if (is_help) return false;

  std::vector<elttype> input;

  if (parameters.dryrun) {
    PRINTOUT("==== DRY RUN ====\n");
  }

  if (!fname.empty()) {
    PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());
    input = input::load(fname, g_Quadtree_Depth, parameters.rate, n_elts);
    PRINTOUT(" %d teewts loaded \n", (uint32_t) input.size());
  } else {
    PRINTOUT("Generate random keys..");
    input = input::dist_random(n_elts, seed, parameters.rate);
    PRINTOUT(" %d teewts generated \n", (uint32_t) input.size());
  }

  std::vector<region_t> queries;
  load_bench_file(bench_file, queries, n_queries);

  PRINTOUT("%d queries loaded \n", queries.size());

  PRINTOUT("Refinement level = %d\n", parameters.refLevel);
  //run_bench<GeoHashSequential>(argc, argv, input, queries, parameters);
  run_bench<GeoHashBinary>(argc, argv, input, queries, parameters);
  run_bench<BTreeCtn>(argc, argv, input, queries, parameters);
  //run_bench<RTreeCtn<bgi::rstar < 16>> > (argc, argv, input, queries, parameters);
  //run_bench<RTreeCtn<bgi::quadratic < 16>> > (argc, argv, input, queries, parameters);

  return EXIT_SUCCESS;
}
