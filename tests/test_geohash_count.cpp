//
// Created by cicerolp on 9/1/17.
//

#include "stde.h"
#include "types.h"

#include "GeoHash.h"
#include "InputIntf.h"

uint32_t g_Quadtree_Depth = 25;

#define PRINT_TEST(...) do { \
   printcsv( __VA_ARGS__ ) ; \
   std::cout << std::endl ;\
} while (0)

class TEST_GeoHashBinary : public GeoHashBinary {
 public:
  TEST_GeoHashBinary(int argc, char **argv) : GeoHashBinary(argc, argv) {}

  uint32_t test_count(const region_t &region) {
    if (_pma == nullptr) return 0;

    uint32_t count = 0;

    auto pma_begin = pma_seg_it::begin(_pma);
    auto pma_end = pma_seg_it::end(_pma);

    while (pma_begin++ != pma_end) {
      count += std::count_if(pma_offset_it::begin(_pma, pma_begin), pma_offset_it::end(_pma, pma_begin),
                             [&region](void *elt) {

                               uint64_t code = PMA_ELT(elt);
                               // count how many elements in this tile are inside the region
                               uint32_t x, y;
                               mortonDecode_RAM(code, x, y);

                               return (region.x0 <= x && region.x1 >= x && region.y0 <= y && region.y1 >= y);
                             }
      );
    }

    return count;
  }

  void get_mcode_range(uint64_t code, uint32_t zoom, uint64_t &min, uint64_t &max, uint32_t mCodeSize) {
    uint32_t diffDepth = mCodeSize - zoom;
    min = code << 2 * (diffDepth);
    max = min | ((uint64_t) ~0 >> (64 - 2 * diffDepth));
  }
};

struct bench_t {
  uint64_t min_t, max_t, inc_t;
  uint64_t rate;
};

void inline count_element(uint32_t &accum, const spatial_t &, uint32_t count) {
  accum += count;
}

// reads the full element
void inline read_element(uint32_t &accum, const valuetype &el) {
  ++accum;
}

template<typename T>
void inline run_queries(T &container, const region_t &region, uint32_t id, uint64_t t, const bench_t &parameters) {

  duration_t timer;

  uint32_t count_apply_at_region = 0;
  applytype_function _apply_at_region = std::bind(count_element, std::ref(count_apply_at_region),
                                                  std::placeholders::_1, std::placeholders::_2);

  container.apply_at_region(region, _apply_at_region);

  uint32_t count_scan_at_region = 0;
  scantype_function _scan_at_region = std::bind(read_element, std::ref(count_scan_at_region),
                                                std::placeholders::_1);

  container.scan_at_region(region, _scan_at_region);

  uint32_t count_test = container.test_count(region);

  PRINT_TEST(id,
             "count",
             count_test,
             "apply_at_region",
             count_apply_at_region,
             "scan_at_region",
             count_scan_at_region);

  if (count_apply_at_region != count_test || count_scan_at_region != count_test) {
    exit(EXIT_FAILURE);
  }
}

template<typename T>
void run_bench(int argc, char *argv[], const std::vector<elttype> &input,
               const std::vector<region_t> &queries, const bench_t &parameters) {

  for (uint64_t t = parameters.min_t; t <= parameters.max_t; t += parameters.inc_t) {

    // calculates ctn size based on insertion rate and temporal window (rate * temporal_window)
    uint64_t ctn_size = std::min((uint64_t) input.size(), parameters.rate * t);

    //create container
    std::unique_ptr < T > container = std::make_unique<T>(argc, argv);
    container->create((uint32_t) ctn_size);

    std::vector<elttype> batch(input.begin(), input.begin() + ctn_size);

    // insert all elements as a single batch
    container->insert(batch);

    // perform custom queries
    for (uint32_t id = 0; id < queries.size(); id++) {
      run_queries((*container.get()), queries[id], id, t, parameters);
    }

    // delete container
    container.reset();
  }
}

/***
 * Reads a csv file containing (lat0, lon0, lat1, lon1) of a bounding box
***/
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

  cimg_usage("test_geohash_count");

  std::string fname(cimg_option("-f", "", "File with tweets (if NULL, generates Rate X T elemets "));
  const long seed(cimg_option("-seed", 123, "Random seed to generate elements"));

  int32_t n_queries(cimg_option("-n_queries", -1, "Number of queries"));
  std::string bench_file(cimg_option("-bf", "./data/benchmark_debug.csv", "Benchmark file"));

  parameters.rate = (cimg_option("-rate", 1000, "Insertion rate"));

  parameters.min_t = (cimg_option("-min_t", 10800, "T: Min"));
  parameters.max_t = (cimg_option("-max_t", 43200, "T: Max"));
  parameters.inc_t = (cimg_option("-inc_t", 10800, "T: Increment"));

  uint64_t n_elts = parameters.rate * parameters.max_t;

  const char *is_help = cimg_option("-h", (char *) 0, 0);
  if (is_help) return false;

  std::vector<elttype> input;

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

  PRINTOUT(" %d queries loaded \n", queries.size());

  run_bench<TEST_GeoHashBinary>(argc, argv, input, queries, parameters);
  //run_bench<GeoHashBinary>(argc, argv, input, queries, parameters);

  return EXIT_SUCCESS;
}
