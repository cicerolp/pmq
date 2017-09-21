/** @file
 * Benchmark for the query time
 */

#include "stde.h"
#include "types.h"
#include "InputIntf.h"

#include "RTreeCtn.h"
#include "BTreeCtn.h"

#include "GeoHash.h"

#define PRINTBENCH(...) do { \
   std::cout << "TopkSearchBench " << container.name() << " ; ";\
   printcsv( __VA_ARGS__ ) ; \
   std::cout << std::endl ;\
} while (0)

/*#define PRINTBENCH( ... ) do { \
} while (0)*/

uint32_t g_Quadtree_Depth = 25;

struct center_t {
  center_t() = default;

  center_t(float _lat, float _lon) : lat(_lat), lon(_lon) {
  }

  float lat, lon;
};

struct bench_t {
  // topk parameters
  bool var_k, var_r, var_a;

  uint32_t def_k, min_k, max_k, inc_k;
  float def_r, min_r, max_r, inc_r;
  uint64_t def_t;
  float def_a, min_a, max_a, inc_a;

  // benchmark parameters
  uint32_t n_exp;
  uint64_t rate;
  uint64_t now;

  bool dryrun;

  topk_t reset_topk() const {
    topk_t topk_info;

    topk_info.alpha = def_a;
    topk_info.k = def_k;
    topk_info.now = now;
    topk_info.time = def_t;
    topk_info.radius = def_r;

    return topk_info;
  }

  void print() {
    PRINTOUT("parameters-> n_exp: %d, rate: %lld, now: %lld\n", n_exp, rate, now);
    PRINTOUT("t-> default: %lld\n", def_t);
    PRINTOUT("k-> enable: %s, default: %d, interval: [%d,%d], increment: %d\n",
             var_k ? "true" : "false",
             def_k,
             min_k,
             max_k,
             inc_k);
    PRINTOUT("r-> enable: %s, default: %f, interval: [%f,%f], increment: %f\n",
             var_r ? "true" : "false",
             def_r,
             min_r,
             max_r,
             inc_r);
    PRINTOUT("a-> enable: %s, default: %f, interval: [%f,%f], increment: %f\n",
             var_a ? "true" : "false",
             def_a,
             min_a,
             max_a,
             inc_a);
  }
};

template<typename T>
void inline run_queries(T &container, const center_t &center, uint32_t id, const bench_t &parameters) {
  Timer timer;

  topk_t topk_info;

  uint32_t count = 0;
  applytype_function _apply = std::bind([](uint32_t &accum, const spatial_t &, uint32_t count) {
    accum += count;
  }, std::ref(count), std::placeholders::_1, std::placeholders::_2);

  uint32_t topk_count = 0;
  scantype_function _scan_fn = std::bind([](uint32_t &accum, const valuetype &) {
    ++accum;
  }, std::ref(topk_count), std::placeholders::_1);

  if (parameters.var_k) {
    for (uint32_t k = parameters.min_k; k <= parameters.max_k; k += parameters.inc_k) {
      // warm up
      topk_info = parameters.reset_topk();
      topk_info.k = k;

      count = 0;
      if (!parameters.dryrun) container.apply_at_region(region_t(center.lat, center.lon, topk_info.radius), _apply);
      topk_count = 0;
      if (!parameters.dryrun)
        container.topk_search(region_t(center.lat, center.lon, topk_info.radius),
                              topk_info,
                              _scan_fn);

      for (uint32_t i = 0; i < parameters.n_exp; i++) {
        topk_info = parameters.reset_topk();
        topk_info.k = k;

        timer.start();
        topk_count = 0;
        if (!parameters.dryrun)
          container.topk_search(region_t(center.lat, center.lon, topk_info.radius),
                                topk_info,
                                _scan_fn);
        timer.stop();

        PRINTBENCH("topk_search_k", id, parameters.def_t, k, topk_count, count, timer.milliseconds(), "ms");
      }
    }
  }

  if (parameters.var_r) {
    for (float r = parameters.min_r; r <= parameters.max_r; r += parameters.inc_r) {
      // warm up
      topk_info = parameters.reset_topk();
      topk_info.radius = r;

      count = 0;
      if (!parameters.dryrun) container.apply_at_region(region_t(center.lat, center.lon, topk_info.radius), _apply);
      topk_count = 0;
      if (!parameters.dryrun)
        container.topk_search(region_t(center.lat, center.lon, topk_info.radius),
                              topk_info,
                              _scan_fn);

      for (uint32_t i = 0; i < parameters.n_exp; i++) {
        topk_info = parameters.reset_topk();
        topk_info.radius = r;

        timer.start();
        topk_count = 0;
        if (!parameters.dryrun)
          container.topk_search(region_t(center.lat, center.lon, topk_info.radius),
                                topk_info,
                                _scan_fn);
        timer.stop();

        PRINTBENCH("topk_search_r", id, parameters.def_t, r, topk_count, count, timer.milliseconds(), "ms");
      }
    }
  }

  if (parameters.var_a) {
    for (float a = parameters.min_a; a <= parameters.max_a; a += parameters.inc_a) {
      // warm up
      topk_info = parameters.reset_topk();
      topk_info.alpha = a;

      count = 0;
      if (!parameters.dryrun) container.apply_at_region(region_t(center.lat, center.lon, topk_info.radius), _apply);
      topk_count = 0;
      if (!parameters.dryrun)
        container.topk_search(region_t(center.lat, center.lon, topk_info.radius),
                              topk_info,
                              _scan_fn);

      for (uint32_t i = 0; i < parameters.n_exp; i++) {
        topk_info = parameters.reset_topk();
        topk_info.alpha = a;

        timer.start();
        topk_count = 0;
        if (!parameters.dryrun)
          container.topk_search(region_t(center.lat, center.lon, topk_info.radius),
                                topk_info,
                                _scan_fn);
        timer.stop();

        PRINTBENCH("topk_search_a", id, parameters.def_t, a, topk_count, count, timer.milliseconds(), "ms");
      }
    }
  }
}

template<typename T>
void run_bench(int argc,
               char *argv[],
               const std::vector<elttype> &input,
               const std::vector<center_t> &queries,
               const bench_t &parameters) {
  // calculates ctn size based on insertion rate and temporal window (rate * temporal_window)
  uint64_t ctn_size = std::min((uint64_t) input.size(), parameters.rate * parameters.def_t);

  //create container
  std::unique_ptr < T > container = std::make_unique<T>(argc, argv);
  container->create((uint32_t) ctn_size);

  std::vector<elttype> batch(input.begin(), input.begin() + ctn_size);

  // insert batch
  if (!parameters.dryrun) container->insert(batch);

  for (uint32_t id = 0; id < queries.size(); id++) {
    run_queries((*container.get()), queries[id], id, parameters);
  }

  // delete container
  container.reset();
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

  cimg_usage("Topk Search Benchmark.");

  std::string fname(cimg_option("-f", "", "file with tweets"));
  const long seed(cimg_option("-r", 123, "Random seed to generate elements"));

  int32_t n_queries(cimg_option("-n_queries", -1, "Number of queries"));
  std::string bench_file(cimg_option("-bf", "./data/checkins_global.dat", "file with logs"));

  parameters.rate = (cimg_option("-rate", 1000, "Insertion rate"));
  parameters.n_exp = (cimg_option("-x", 1, "Number of repetitions of each experiment"));

  parameters.var_k = (cimg_option("-var_k", false, "K: Enable benchmark"));
  parameters.def_k = (cimg_option("-def_k", 100, "K: Default value"));
  parameters.min_k = (cimg_option("-min_k", 10, "K: Min "));
  parameters.max_k = (cimg_option("-max_k", 100, "K: Max "));
  parameters.inc_k = (cimg_option("-inc_k", 10, "K: Increment"));

  parameters.var_r = (cimg_option("-var_r", false, "r: Enable benchmark"));
  parameters.def_r = (cimg_option("-def_r", 30.f, "R: Default value (radius in km)"));
  parameters.min_r = (cimg_option("-min_r", 0.25f, "R: Min"));
  parameters.max_r = (cimg_option("-max_r", 400.1f, "R: Max"));
  parameters.inc_r = (cimg_option("-inc_r", 39.975f, "R: Increment"));

  parameters.def_t = (cimg_option("-def_t", 21600, "T: Default value (in seconds)"));

  parameters.var_a = (cimg_option("-var_a", false, "a: Enable benchmark"));
  parameters.def_a = (cimg_option("-def_a", 0.2f, "a: Default value"));
  parameters.min_a = (cimg_option("-min_a", 0.f, "a: Min"));
  parameters.max_a = (cimg_option("-max_a", 1.f, "a: Max"));
  parameters.inc_a = (cimg_option("-inc_a", 0.2f, "a: Increment"));

  parameters.dryrun = (cimg_option("-dry", false, "Dry run"));

  uint64_t n_elts = parameters.rate * parameters.def_t;

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

  // set now to last valid time
  parameters.now = input.back().value.time;

  parameters.print();

  std::vector<center_t> queries;
  load_bench_file(bench_file, queries, n_queries);

  run_bench<GeoHashSequential>(argc, argv, input, queries, parameters);
  run_bench<GeoHashBinary>(argc, argv, input, queries, parameters);

  run_bench<BTreeCtn>(argc, argv, input, queries, parameters);

  run_bench<RTreeCtn<bgi::rstar < 16>> > (argc, argv, input, queries, parameters);

  return EXIT_SUCCESS;
}
