/** @file
 * Benchmark for the query time
 */

#include "stde.h"
#include "types.h"
#include "string_util.h"

#include "InputIntf.h"
#include "GeoCtnIntf.h"

#include "GeoHash.h"

#define PRINTBENCH( ... ) do { \
   std::cout << "TopkSearchBench " << container.name() << " ; ";\
   printcsv( __VA_ARGS__ ) ; \
   std::cout << std::endl ;\
} while (0)

/*#define PRINTBENCH( ... ) do { \
} while (0)*/

uint32_t g_Quadtree_Depth = 25;

void inline count_element(uint32_t& accum, const spatial_t&, uint32_t count) {
   accum += count;
}

// reads the full element
void inline read_element(const valuetype& el) {
   valuetype volatile elemt = *(valuetype*)&el;
}

template <typename container_t>
void inline run_queries(container_t& container, const region_t& region, uint32_t id, uint32_t n_exp) {

   Timer timer;

   uint32_t count = 0;
   applytype_function _apply = std::bind(count_element, std::ref(count),
                                         std::placeholders::_1, std::placeholders::_2);
   // warm up
   container.scan_at_region(region, read_element);

   for (uint32_t i = 0; i < n_exp; i++) {
      timer.start();
      container.scan_at_region(region, read_element);
      timer.stop();
      PRINTBENCH("scan_at_region", id, timer.milliseconds(), "ms");
   }

   // warm up
   container.apply_at_tile(region, _apply);

   for (uint32_t i = 0; i < n_exp; i++) {
      count = 0;
      timer.start();
      container.apply_at_tile(region, _apply);
      timer.stop();
      PRINTBENCH("apply_at_tile", id, count, timer.milliseconds(), "ms");
   }

   // warm up
   container.apply_at_region(region, _apply);

   for (uint32_t i = 0; i < n_exp; i++) {
      count = 0;
      timer.start();
      container.apply_at_region(region, _apply);
      timer.stop();
      PRINTBENCH("apply_at_region", id, count, timer.milliseconds(), "ms");
   }
}

template <typename container_t>
void run_bench(container_t& container, const std::vector<elttype>& input, const std::vector<region_t>& queries, uint32_t batch_size, uint32_t n_exp) {
   //create container
   container.create((uint32_t)input.size());

   std::vector<elttype>::const_iterator it_begin = input.begin();
   std::vector<elttype>::const_iterator it_curr = input.begin();

   while (it_begin != input.end()) {
      it_curr = std::min(it_begin + batch_size, input.end());

      std::vector<elttype> batch(it_begin, it_curr);

      // insert batch
      container.insert(batch);

      // update iterator
      it_begin = it_curr;
   }

   for (uint32_t id = 0; id < queries.size(); id++) {
      //run_queries(container, queries[id], id, n_exp);
   }

   region_t region(0, 0, 1, 100);

   std::cout << region.lat << std::endl;
   std::cout << region.lon << std::endl;
   std::cout << region.z << std::endl;

   std::cout << region.x0 << std::endl;
   std::cout << region.x1 << std::endl;

   std::cout << region.y0 << std::endl;
   std::cout << region.y1 << std::endl;

   std::vector<valuetype> output;

   float alpha = 0.5;
   uint64_t now = 1483644389;
   uint64_t time = 1483644389;

   container.topk_search(region, output, alpha, now, time);

   std::cout << output.size() << std::endl;
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

            queries_vec.emplace_back(region_t(std::stoi(url[8]), std::stoi(url[9]), std::stoi(url[10]), std::stoi(url[11]), zoom));
         }
      } catch (std::invalid_argument) {
         std::cerr << "error: invalid query log [" << line << "]" << std::endl;
      }
   }

   infile.close();

   PRINTOUT(" %d quries loaded. \n", (uint32_t)queries_vec.size());
}

int main(int argc, char* argv[]) {

   cimg_usage("Queries Benchmark inserts elements in batches.");
   const unsigned int nb_elements(cimg_option("-n", 0, "Number of elements to generate randomly"));
   const long seed(cimg_option("-r", 0, "Random seed to generate elements"));
   const int batch_size(cimg_option("-b", 10, "Batch size used in batched insertions"));
   std::string fname(cimg_option("-f", "./data/tweet100.dat", "file with tweets"));
   std::string bench_file(cimg_option("-bf", "./data/log.csv", "file with logs"));
   const unsigned int n_exp(cimg_option("-x", 1, "Number of repetitions of each experiment"));

   GeoHashSequential container5(argc, argv);
   GeoHashBinary container6(argc, argv);

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
      input = input::dist_random(nb_elements, seed);
      PRINTOUT(" %d teewts generated \n", (uint32_t)input.size());
   }

   std::vector<region_t> queries;
   //load_bench_file(bench_file, queries);

   //run_bench(container5, input, queries, batch_size, n_exp);
   run_bench(container6, input, queries, batch_size, n_exp);

   return EXIT_SUCCESS;
}
