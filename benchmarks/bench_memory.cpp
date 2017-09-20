#include "stde.h"
#include "types.h"

#include "input_it.h"
#include "memory_util.h"

#include "PMQ.h"
#include "RTreeCtn.h"
#include "BTreeCtn.h"
#include "DenseCtn.h"

#define PRINTBENCH(...) do { \
   std::cout << "MemoryBench " << container.name() << " ; ";\
   printcsv( __VA_ARGS__ ) ; \
   std::cout << std::endl ;\
} while (0)

#define PRINTBENCH_PTR(...) do { \
   std::cout << "MemoryBench " << container->name() << " ; ";\
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

template<typename T, typename _It, typename _T>
void run_bench(int argc, char *argv[], _It it_begin, _It it_end, const bench_t &parameters) {
  // store temporary memory
  size_t resident = getCurrentRSS();

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

    // update iterator
    it_begin = it_curr;

    PRINTBENCH_PTR("id", id, (getCurrentRSS() - resident) / (1024), "KB");

    ++id;
  }

  PRINTBENCH_PTR("total", (getCurrentRSS() - resident) / (1024), "KB");

  container.reset();
}

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

int main(int argc, char *argv[]) {
  bench_t parameters;

  const unsigned int nb_elements(cimg_option("-n", 0, "Number of elements to read / generate randomly"));
  const long seed(cimg_option("-r", 0, "Random seed to generate elements"));

  std::string fname(cimg_option("-f", "", "File with tweets to load"));

  std::string container(cimg_option("-c", "ghb", "Container Acronym"));

  parameters.batch_size = (cimg_option("-b", 100, "Batch size used in batched insertions"));

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

    if (container == "ghb") {
      run_bench<PMQBinary<el_t>, it_t, el_t>(argc, argv, begin, end, parameters);
    } else if (container == "bt") {
      run_bench<BTreeCtn < el_t>, it_t, el_t > (argc, argv, begin, end, parameters);
    } else if (container == "rt") {
      run_bench<RTreeCtn<el_t, bgi::quadratic < 16>> , it_t, el_t>(argc, argv, begin, end, parameters);
    } else if (container == "dv") {
      run_bench<DenseCtn<el_t>, it_t, el_t>(argc, argv, begin, end, parameters);
    }

  } else {
    using el_t = GenericType;
    using it_t = input_random_it;

    PRINTOUT("Generate random keys...\n");
    auto begin = it_t::begin(seed, parameters.batch_size);
    auto end = it_t::end(seed, parameters.batch_size, nb_elements);
    PRINTOUT("%d teewts generated \n", end - begin);

    if (container == "ghb") {
      run_bench<PMQBinary<el_t>, it_t, el_t>(argc, argv, begin, end, parameters);
    } else if (container == "bt") {
      run_bench<BTreeCtn < el_t>, it_t, el_t > (argc, argv, begin, end, parameters);
    } else if (container == "rt") {
      run_bench<RTreeCtn<el_t, bgi::quadratic < 16>> , it_t, el_t>(argc, argv, begin, end, parameters);
    } else if (container == "dv") {
      run_bench<DenseCtn<el_t>, it_t, el_t>(argc, argv, begin, end, parameters);
    }
  }

  return EXIT_SUCCESS;
}

