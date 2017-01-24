/** @file
 * Benchmark for the query time
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
   std::cout << "TopkSearchBench " << container.name() << " ; ";\
   printcsv( __VA_ARGS__ ) ; \
   std::cout << std::endl ;\
} while (0)

/*#define PRINTBENCH( ... ) do { \
} while (0)*/

uint32_t g_Quadtree_Depth = 25;

struct center_t {
   center_t(float _lat, float _lon) : lat(_lat), lon(_lon) {
   }

   float lat, lon;
};

struct bench_t {
   // topk parameters
   bool var_k, var_r, var_t, var_a;

   uint32_t def_k, min_k, max_k, inc_k;
   float def_r, min_r, max_r, inc_r;
   uint64_t def_t, min_t, max_t, inc_t;
   float def_a, min_a, max_a, inc_a;

   // benchmark parameters
   uint32_t n_exp;
   uint32_t batch_size;
   uint64_t now;

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
      PRINTOUT("parameters-> n_exp: %d, batch_size: %d, now: %lld\n", n_exp, batch_size, now);
      PRINTOUT("k-> enable: %s, default: %d, interval: [%d,%d], increment: %d\n", var_k ? "true" : "false", def_k, min_k, max_k, inc_k);
      PRINTOUT("r-> enable: %s, default: %f, interval: [%f,%f], increment: %f\n", var_r ? "true" : "false", def_r, min_r, max_r, inc_r);
      PRINTOUT("t-> enable: %s, default: %lld, interval: [%lld,%lld], increment: %lld\n", var_t ? "true" : "false", def_t, min_t, max_t, inc_t);
      PRINTOUT("a-> enable: %s, default: %f, interval: [%f,%f], increment: %f\n", var_a ? "true" : "false", def_a, min_a, max_a, inc_a);
   }
};

void inline count_element(uint32_t& accum, const spatial_t&, uint32_t count) {
   accum += count;
}

template <typename T>
void inline run_queries(T& container, const center_t& center, uint32_t id, const bench_t& parameters) {
   Timer timer;

   topk_t topk_info;
   std::vector<valuetype> output;

   uint32_t count = 0;
   applytype_function _apply = std::bind(count_element, std::ref(count),
                                         std::placeholders::_1, std::placeholders::_2);

   if (parameters.var_k) {
      for (uint32_t k = parameters.min_k; k <= parameters.max_k; k += parameters.inc_k) {
         // warm up
         output.clear();
         topk_info = parameters.reset_topk();
         topk_info.k = k;

         count = 0;
         container.apply_at_region(region_t(center.lat, center.lon, topk_info.radius), _apply);
         container.topk_search(region_t(center.lat, center.lon, topk_info.radius), topk_info, output);

         for (uint32_t i = 0; i < parameters.n_exp; i++) {
            output.clear();
            topk_info = parameters.reset_topk();
            topk_info.k = k;

            timer.start();
            container.topk_search(region_t(center.lat, center.lon, topk_info.radius), topk_info, output);
            timer.stop();

            PRINTBENCH("topk_search_k", id, k, output.size(), count, timer.milliseconds(), "ms");
         }
      }
   }

   if (parameters.var_r) {
      for (float r = parameters.min_r; r <= parameters.max_r; r += parameters.inc_r) {
         // warm up
         output.clear();
         topk_info = parameters.reset_topk();
         topk_info.radius = r;

         count = 0;
         container.apply_at_region(region_t(center.lat, center.lon, topk_info.radius), _apply);
         container.topk_search(region_t(center.lat, center.lon, topk_info.radius), topk_info, output);

         for (uint32_t i = 0; i < parameters.n_exp; i++) {
            output.clear();
            topk_info = parameters.reset_topk();
            topk_info.radius = r;

            timer.start();
            container.topk_search(region_t(center.lat, center.lon, topk_info.radius), topk_info, output);
            timer.stop();

            PRINTBENCH("topk_search_r", id, r, output.size(), count, timer.milliseconds(), "ms");
         }
      }
   }

   if (parameters.var_t) {
      for (uint64_t t = parameters.min_t; t <= parameters.max_t; t += parameters.inc_t) {
         // warm up
         output.clear();
         topk_info = parameters.reset_topk();
         topk_info.time = t;

         count = 0;
         container.apply_at_region(region_t(center.lat, center.lon, topk_info.radius), _apply);
         container.topk_search(region_t(center.lat, center.lon, topk_info.radius), topk_info, output);

         for (uint32_t i = 0; i < parameters.n_exp; i++) {
            output.clear();
            topk_info = parameters.reset_topk();
            topk_info.time = t;

            timer.start();
            container.topk_search(region_t(center.lat, center.lon, topk_info.radius), topk_info, output);
            timer.stop();

            PRINTBENCH("topk_search_t", id, t, output.size(), count, timer.milliseconds(), "ms");
         }
      }
   }

   if (parameters.var_a) {
      for (float a = parameters.min_a; a <= parameters.max_a; a += parameters.inc_a) {
         // warm up
         output.clear();
         topk_info = parameters.reset_topk();
         topk_info.alpha = a;

         count = 0;
         container.apply_at_region(region_t(center.lat, center.lon, topk_info.radius), _apply);
         container.topk_search(region_t(center.lat, center.lon, topk_info.radius), topk_info, output);

         for (uint32_t i = 0; i < parameters.n_exp; i++) {
            output.clear();
            topk_info = parameters.reset_topk();
            topk_info.alpha = a;

            timer.start();
            container.topk_search(region_t(center.lat, center.lon, topk_info.radius), topk_info, output);
            timer.stop();

            PRINTBENCH("topk_search_a", id, a, output.size(), count, timer.milliseconds(), "ms");
         }
      }
   }
}

template <typename T>
void run_bench(int argc, char* argv[], const std::vector<elttype>& input, const std::vector<center_t>& queries, const bench_t& parameters) {
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

void load_bench_file(const std::string& file, std::vector<center_t>& queries) {
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
            uint32_t x = std::stoi(url[8]);
            uint32_t y = std::stoi(url[9]);
            uint32_t zoom = std::stoi(url[6]);

            if (x >= std::pow(2, zoom)) x = (uint32_t)std::pow(2, zoom) - 1;
            if (y >= std::pow(2, zoom)) y = (uint32_t)std::pow(2, zoom) - 1;

            float lon = mercator_util::tilex2lon(x + 0.5f, zoom);
            float lat = mercator_util::tiley2lat(y + 0.5f, zoom);

            if (lat < -85.051132f || lat > 85.051132f) throw std::invalid_argument("[lat: " + std::to_string(lat) + "]");
            if (lon < -180.f || lon > 180.f) throw std::invalid_argument("[lon: " + std::to_string(lon) + "]");

            queries.emplace_back(lat, lon);

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

            float lon = mercator_util::tilex2lon((((x1 - x0) / 2.f) + x0) + 0.5f, zoom);
            float lat = mercator_util::tiley2lat((((y1 - y0) / 2.f) + y0) + 0.5f, zoom);

            if (lat < -85.051132f || lat > 85.051132f) throw std::invalid_argument("[lat: " + std::to_string(lat) + "]");
            if (lon < -180.f || lon > 180.f) throw std::invalid_argument("[lon: " + std::to_string(lon) + "]");

            queries.emplace_back(lat, lon);
         }
      } catch (const std::invalid_argument& e) {
         std::cerr << "error: [" << line << "]" << std::endl;
         std::cerr << "error: " + std::string(e.what()) << std::endl;
      }
   }

   infile.close();

   PRINTOUT(" %d quries loaded. \n", (uint32_t)queries.size());
}

int main(int argc, char* argv[]) {
   bench_t parameters;

   cimg_usage("Topk Search Benchmark.");

   const unsigned int nb_elements(cimg_option("-n", 0, "Number of elements to generate randomly"));
   const long seed(cimg_option("-r", 123, "Random seed to generate elements"));
   std::string fname(cimg_option("-f", "./data/tweet10_6.dat", "file with tweets"));
   std::string bench_file(cimg_option("-bf", "./data/log.csv", "file with logs"));

   parameters.batch_size = (cimg_option("-b", 1000, "Batch size used in batched insertions"));
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

   parameters.var_t = (cimg_option("-var_t", false, "T: Enable benchmark"));
   parameters.def_t = (cimg_option("-def_t", 21600, "T: Default value (in seconds)"));
   parameters.min_t = (cimg_option("-min_t", 10800, "T: Min"));
   parameters.max_t = (cimg_option("-max_t", 43200, "T: Max"));
   parameters.inc_t = (cimg_option("-inc_t", 10800, "T: Increment"));

   parameters.var_a = (cimg_option("-var_a", false, "a: Enable benchmark"));
   parameters.def_a = (cimg_option("-def_a", 0.2f, "a: Default value"));
   parameters.min_a = (cimg_option("-min_a", 0.f, "a: Min"));
   parameters.max_a = (cimg_option("-max_a", 1.f, "a: Max"));
   parameters.inc_a = (cimg_option("-inc_a", 0.2f, "a: Increment"));

   const char* is_help = cimg_option("-h", (char*)0, 0);
   if (is_help) return false;

   const uint32_t quadtree_depth = 25;

   std::vector<elttype> input;

   if (nb_elements == 0) {
      PRINTOUT("Loading twitter dataset... %s \n", fname.c_str());
      input = input::load(fname, quadtree_depth, parameters.batch_size);
      PRINTOUT(" %d teewts loaded \n", (uint32_t)input.size());
   } else {
      PRINTOUT("Generate random keys..");
      input = input::dist_random(nb_elements, seed, parameters.batch_size);
      PRINTOUT(" %d teewts generated \n", (uint32_t)input.size());
   }

   // set now to last valid time
   parameters.now = input.back().value.time;

   parameters.print();

   std::vector<center_t> queries;
   load_bench_file(bench_file, queries);

   //run_bench<GeoHashSequential>(argc, argv, input, queries, parameters);
   run_bench<GeoHashBinary>(argc, argv, input, queries, parameters);

   return EXIT_SUCCESS;
}
