#include "stde.h"
#include "types.h"
#include "InputIntf.h"
#include "string_util.h"

#include "memory_util.h"

#include "RTreeCtn.h"
#include "BTreeCtn.h"

#include "GeoHash.h"
#include "PMABatchCtn.h"
#include "PostGisCtn.h"
#include "SpatiaLiteCtn.h"
#include "DenseVectorCtn.h"

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

uint32_t g_Quadtree_Depth = 25;

// reads the full element
void inline read_element(const valuetype &el) {
  valuetype volatile elemt = *(valuetype *) &el;
}

template<typename T>
void run_bench(int argc, char *argv[], const std::vector<elttype> &input, const bench_t &parameters) {
  // store temporary memory
  size_t resident = getCurrentRSS();

  //create container
  std::unique_ptr < T > container = std::make_unique<T>(argc, argv);
  container->create((uint32_t) input.size());

  std::vector<elttype>::const_iterator it_begin = input.begin();
  std::vector<elttype>::const_iterator it_curr = input.begin();

  duration_t timer;

  int id = 0;
  while (it_begin != input.end()) {
    it_curr = std::min(it_begin + parameters.batch_size, input.end());

    std::vector<elttype> batch(it_begin, it_curr);

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

  const uint32_t quadtree_depth = 25;

  std::vector<elttype> input;

  if (!fname.empty()) {
    PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());
    input = input::loadn(fname, quadtree_depth, nb_elements);
    PRINTOUT("%d teewts loaded \n", (uint32_t) input.size());
  } else {
    PRINTOUT("Generate random keys...\n");
    // use the batch id as timestamp
    input = input::dist_random(nb_elements, seed, parameters.batch_size);
    PRINTOUT("%d teewts generated \n", (uint32_t) input.size());
  }

  if (container == "ghb") {
    run_bench<GeoHashBinary>(argc, argv, input, parameters);
  } else if (container == "ghs") {
    run_bench<GeoHashSequential>(argc, argv, input, parameters);
  } else if (container == "bt") {
    run_bench<BTreeCtn>(argc, argv, input, parameters);
  } else if (container == "rt") {
    run_bench<RTreeCtn<bgi::quadratic < 16>> > (argc, argv, input, parameters);
  } else {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

