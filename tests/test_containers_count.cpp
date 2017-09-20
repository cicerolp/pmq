//
// Created by cicerolp on 9/1/17.
//

#include "stde.h"
#include "types.h"
#include "input_it.h"

#include "PMQ.h"
//#include "RTreeCtn.h"
//#include "BTreeCtn.h"

//#include "ImplicitDenseVectorCtn.h"

#define PRINT_TEST(...) do { \
   printcsv( __VA_ARGS__ ) ; \
   std::cout << std::endl ;\
} while (0)

template<typename T>
class TEST_PMQBinary : public PMQBinary<T> {
 public:
  TEST_PMQBinary(int argc, char **argv) : PMQBinary<T>(argc, argv) {}

  uint32_t test_count(const region_t &region) {
    if (this->_pma == nullptr) return 0;

    uint32_t count = 0;

    auto pma_begin = pma_seg_it::begin(this->_pma);
    auto pma_end = pma_seg_it::end(this->_pma);

    while (pma_begin++ != pma_end) {
      count += std::count_if(pma_offset_it::begin(this->_pma, pma_begin), pma_offset_it::end(this->_pma, pma_begin),
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
  uint64_t t;
  uint64_t rate;
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

template<typename T, typename _It, typename _T>
std::vector<uint32_t> run_test_bench(int argc, char *argv[], _It it_begin, _It it_end,
                                     const std::vector<region_t> &queries, const bench_t &parameters) {

  std::vector<uint32_t> test_count;

// calculates ctn size based on insertion rate and temporal window (rate * temporal_window)
  size_t ctn_size = std::min((size_t) (it_end - it_begin), parameters.rate * parameters.t);

  //create container
  std::unique_ptr < T > container = std::make_unique<T>(argc, argv);
  container->create(ctn_size);

  std::vector<_T> batch(it_begin, it_begin + ctn_size);

  // insert all elements as a single batch
  container->insert(batch);

  // perform custom queries
  for (uint32_t id = 0; id < queries.size(); id++) {
    test_count.emplace_back(container->test_count(queries[id]));
  }

  // delete container
  container.reset();

  return test_count;
}

template<typename T, typename Tp>
void inline run_queries(T &container, const region_t &region, uint32_t id, const std::vector<uint32_t> &test_count) {
  uint32_t count_apply_at_region = 0;
  uint32_t count_scan_at_region = 0;

  typename GeoCtnIntf<Tp>::scantype_function
      _scan_at_region = std::bind([](uint32_t &accum, const Tp &el) {
    accum++;
  }, std::ref(count_scan_at_region), std::placeholders::_1);

  typename GeoCtnIntf<Tp>::applytype_function
      _apply_at_region = std::bind([](uint32_t &accum, const spatial_t &, uint32_t count) {
    accum += count;
  }, std::ref(count_apply_at_region), std::placeholders::_1, std::placeholders::_2);

  container.apply_at_region(region, _apply_at_region);

  container.scan_at_region(region, _scan_at_region);

  PRINT_TEST(id,
             "count",
             test_count[id],
             "apply_at_region",
             count_apply_at_region,
             "scan_at_region",
             count_scan_at_region);

  if (count_apply_at_region != test_count[id] || count_scan_at_region != test_count[id]) {
    exit(EXIT_FAILURE);
  }
}

template<typename T, typename _It, typename _T>
void run_bench(int argc, char *argv[], _It it_begin, _It it_end, const std::vector<region_t> &queries,
               const bench_t &parameters, const std::vector<uint32_t> &test_count) {
  size_t id = 0;

  // calculates ctn size based on insertion rate and temporal window (rate * temporal_window)
  size_t ctn_size = std::min((size_t) (it_end - it_begin), parameters.rate * parameters.t);

  //create container
  std::unique_ptr < T > container = std::make_unique<T>(argc, argv);
  container->create(ctn_size);

  std::vector<_T> batch(it_begin, it_begin + ctn_size);

  // insert all elements as a single batch
  container->insert(batch);

  // perform custom queries
  for (const auto &q : queries) {
    run_queries<T, _T>((*container.get()), q, id++, test_count);
  }

  // delete container
  container.reset();
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

  cimg_usage("test_geohash_count");

  std::string fname(cimg_option("-f", "", "File with tweets (if NULL, generates Rate X T elemets "));
  const long seed(cimg_option("-seed", 123, "Random seed to generate elements"));

  int32_t n_queries(cimg_option("-n_queries", -1, "Number of queries"));
  std::string bench_file(cimg_option("-bf", "./data/benchmark_debug.csv", "Benchmark file"));

  parameters.rate = (cimg_option("-rate", 1000, "Insertion rate"));

  parameters.t = (cimg_option("-t", 10800, "T: Max"));

  uint64_t n_elts = parameters.rate * parameters.t;

  const char *is_help = cimg_option("-h", (char *) 0, 0);
  if (is_help) return false;

  std::vector<region_t> queries;
  load_bench_file(bench_file, queries, n_queries);

  PRINTOUT(" %d queries loaded \n", queries.size());

  std::vector<uint32_t> test_count;

  if (!fname.empty()) {
    using el_t = TweetDatType;
    using it_t = input_file_it<el_t>;

    PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());
    std::shared_ptr < std::ifstream > file_ptr = std::make_shared<std::ifstream>(fname, std::ios::binary);
    auto begin = it_t::begin(file_ptr);
    auto end = it_t::end(file_ptr);
    PRINTOUT("%d teewts loaded \n", end - begin);

    test_count = run_test_bench<TEST_PMQBinary<el_t>, it_t, el_t>(argc, argv, begin, end, queries, parameters);

    run_bench<PMQBinary<el_t>, it_t, el_t>(argc, argv, begin, end, queries, parameters, test_count);

  } else {
    using el_t = GenericType;
    using it_t = input_random_it;

    PRINTOUT("Generate random keys...\n");
    auto begin = it_t::begin(seed, parameters.rate);
    auto end = it_t::end(seed, parameters.rate, n_elts);
    PRINTOUT("%d teewts generated \n", end - begin);

    test_count = run_test_bench<TEST_PMQBinary<el_t>, it_t, el_t>(argc, argv, begin, end, queries, parameters);

    run_bench<PMQBinary<el_t>, it_t, el_t>(argc, argv, begin, end, queries, parameters, test_count);
  }

  return EXIT_SUCCESS;
}
