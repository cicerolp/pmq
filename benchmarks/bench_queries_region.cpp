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

#include "GeoHash.h"
#include "PMABatchCtn.h"
#include "PostGisCtn.h"
#include "SpatiaLiteCtn.h"
#include "DenseVectorCtn.h"

#define PRINTBENCH( ... ) do { \
   std::cout << "QueryBench " << container.name() << " ; ";\
   printcsv( __VA_ARGS__ ) ; \
   std::cout << std::endl ;\
} while (0)

/*#define PRINTBENCH( ... ) do { \
} while (0)*/

struct bench_t {
   // benchmark parameters
   uint32_t n_exp;
   uint32_t batch_size;
};

uint32_t g_Quadtree_Depth = 25;

void inline count_element(uint32_t& accum, const spatial_t&, uint32_t count) {
   accum += count;
}

// reads the full element
void inline read_element(const valuetype& el) {
   valuetype volatile elemt = *(valuetype*)&el;
}

template <typename T>
void inline run_queries(T& container, const region_t& region, uint32_t id, const bench_t& parameters) {

   Timer timer;

   uint32_t count = 0;
   applytype_function _apply = std::bind(count_element, std::ref(count),
                                         std::placeholders::_1, std::placeholders::_2);
   // warm up
   container.scan_at_region(region, read_element);

   for (uint32_t i = 0; i < parameters.n_exp; i++) {
      timer.start();
      container.scan_at_region(region, read_element);
      timer.stop();
      PRINTBENCH("scan_at_region", id, timer.milliseconds(), "ms");
   }

   // warm up
   container.apply_at_tile(region, _apply);

   for (uint32_t i = 0; i < parameters.n_exp; i++) {
      count = 0;
      timer.start();
      container.apply_at_tile(region, _apply);
      timer.stop();
      PRINTBENCH("apply_at_tile", id, timer.milliseconds(), "ms");
   }

   // warm up
   container.apply_at_region(region, _apply);

   for (uint32_t i = 0; i < parameters.n_exp; i++) {
      count = 0;
      timer.start();
      container.apply_at_region(region, _apply);
      timer.stop();
      PRINTBENCH("apply_at_region", id, timer.milliseconds(), "ms");
   }
}

template <typename T>
void run_bench(int argc, char* argv[], const std::vector<elttype>& input, const std::vector<region_t>& queries, const bench_t& parameters) {
   //create container
   std::unique_ptr<T> container = std::make_unique<T>(argc, argv);
   container->create((uint32_t)input.size());

   std::vector<elttype>::const_iterator it_begin = input.begin();
   std::vector<elttype>::const_iterator it_curr = input.begin();

   while (it_begin != input.end()) {
      it_curr = std::min(it_begin + parameters.batch_size, input.end());

      std::vector<elttype> batch(it_begin, it_curr);

      // insert batch
      container->insert(batch);

      // update iterator
      it_begin = it_curr;
   }

   for (uint32_t id = 0; id < queries.size(); id++) {
      run_queries((*container.get()), queries[id], id, parameters);
   }
}

void load_bench_file(const std::string& file, std::vector<region_t>& queries_vec) {
   PRINTOUT("Loading log file: %s \n", file.c_str());

   std::ifstream infile(file);

   while (!infile.eof()) {

      std::string line;
      std::getline(infile, line);

      try {
         if (line.empty()) continue;

         auto record = string_util::split(line, ",");

         if (record.size() != 3) continue;

         auto url = string_util::split(record[0], "/");

         if (url[5] == "tile") {
            continue;

         } else if (url[5] == "query") {

            uint32_t zoom;
            if (url[7] == "undefined") {
               zoom = 1;
            } else {
               zoom = std::stoi(url[7]);
            }

            uint32_t x0 = std::stoi(url[8]);
            uint32_t y0 = std::stoi(url[9]);
            uint32_t x1 = std::stoi(url[10]);
            uint32_t y1 = std::stoi(url[11]);

            if (x1 >= std::pow(2, zoom)) x1 = (uint32_t)std::pow(2, zoom) - 1;
            if (y1 >= std::pow(2, zoom)) y1 = (uint32_t)std::pow(2, zoom) - 1;

            if (x0 > x1) throw std::invalid_argument("[x: " + std::to_string(x0) + " > " + std::to_string(x1) + " ]");
            if (y0 > y1) throw std::invalid_argument("[y: " + std::to_string(y0) + " > " + std::to_string(y1) + " ]");

            queries_vec.emplace_back(region_t(x0, y0, x1, y1, zoom));
         }
      } catch (const std::invalid_argument& e) {
         std::cerr << "error: [" << line << "]" << std::endl;
         std::cerr << "error: " + std::string(e.what()) << std::endl;
      }
   }

   infile.close();

   PRINTOUT(" %d quries loaded. \n", (uint32_t)queries_vec.size());
}

int main(int argc, char* argv[]) {
   bench_t parameters;

   cimg_usage("Queries Benchmark inserts elements in batches.");

   const unsigned int nb_elements(cimg_option("-n", 0, "Number of elements to generate randomly"));
   const long seed(cimg_option("-r", 0, "Random seed to generate elements"));

   std::string fname(cimg_option("-f", "./data/tweet100.dat", "file with tweets"));
   std::string bench_file(cimg_option("-bf", "./data/log.csv", "file with logs"));

   parameters.batch_size = (cimg_option("-b", 100, "Batch size used in batched insertions"));
   parameters.n_exp = (cimg_option("-x", 1, "Number of repetitions of each experiment"));

   const char* is_help = cimg_option("-h", (char*)0, 0);
   if (is_help) return false;

   const uint32_t quadtree_depth = 25;

   std::vector<elttype> input;

   if (nb_elements == 0) {
      PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());
      input = input::load(fname, quadtree_depth);
      PRINTOUT(" %d teewts loaded \n", (uint32_t)input.size());
   } else {
      PRINTOUT("Generate random keys..");
      input = input::dist_random(nb_elements, seed, parameters.batch_size);
      PRINTOUT(" %d teewts generated \n", (uint32_t)input.size());
   }

   std::vector<region_t> queries;
   load_bench_file(bench_file, queries);

   run_bench<PMABatchCtn>(argc, argv, input, queries, parameters);
   run_bench<GeoHashSequential>(argc, argv, input, queries, parameters);
   run_bench<GeoHashBinary>(argc, argv, input, queries, parameters);
   //run_bench<DenseCtnStdSort>(argc, argv, input, queries, parameters);
   //run_bench<DenseCtnTimSort>(argc, argv, input, queries, parameters);
   //run_bench<SpatiaLiteCtn>(argc, argv, input, queries, parameters);
   //run_bench<PostGisCtn>(argc, argv, input, queries, parameters);

   return EXIT_SUCCESS;
}
