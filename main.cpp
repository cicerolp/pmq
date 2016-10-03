#include "stde.h"

#include "Server.h"
#include "Runner.h"
#include "PMABatch.h"

uint32_t g_Quadtree_Depth = 25;

std::vector<elttype> load_input(std::string fname) {
   std::vector<elttype> tweets;

   std::ifstream infile(fname, std::ios::binary);
   infile.unsetf(std::ios_base::skipws);

   // skip file header
   for (int i = 0; i < 32; ++i) {
      infile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
   }

   tweet_t record;
   size_t record_size = 19; //file record size

   while (true) {
      try {
         infile.read((char*)&record, record_size);

         if (infile.eof()) break;

         tweets.emplace_back(record, g_Quadtree_Depth);
      } catch (...) {
         break;
      }
   }
   infile.close();

   return tweets;
}

inline void insert_batch(struct pma_struct* pma, elttype* batch, int size) {
   // sorting algorithm
   std::sort(batch, batch + size, [](elttype a, elttype b) { return a < b; });

   void *begin = (void*)batch;
   void *end = (void*)((char*)begin + (size) * sizeof(elttype));

   pma::batch::add_array_elts(pma, begin, end, comp<uint64_t>);
}

inline void insert_batch(struct pma_struct* pma, std::vector<elttype> batch) {
   // sorting algorithm
   std::sort(batch.begin(), batch.end(), [](elttype a, elttype b) { return a < b; });

   void *begin = (void*)(&batch[0]);
   void *end = (void*)((char*)begin + ((int)batch.size()) * sizeof(elttype));

   pma::batch::add_array_elts(pma, begin, end, comp<uint64_t>);
}

inline int pma_diff(struct pma_struct* pma, map_t &modified) {

   for (unsigned int wId : *(pma->last_rebalanced_segs)) {
      //We don't need to track rebalance on leaf segments (TODO : could remove it from the PMA rebalance function)
      //  if (wId < pma->nb_segments)
      //    continue;

      unsigned int sStart = pma_get_window_start(pma, wId); //Get first segment of the window

      char* el_pt = (char*)SEGMENT_START(pma, sStart); //get first element of the segment
      uint64_t lastElKey = *(uint64_t*)el_pt; //mcode of the first element

      uint64_t fstSeg = sStart;
      //Check it this key appear previously in the pma.
      while (fstSeg > 0 && (lastElKey == *(uint64_t*)SEGMENT_LAST(pma, fstSeg - 1))) {
         fstSeg--;
      }


      modified.emplace_back(lastElKey, fstSeg, sStart);  //save the start for this key. Initialy end = begin+1 (open interval)

      // loop over the segments of the current window
      unsigned int s;
      for (s = sStart; s < sStart + pma_get_window_size(pma, wId); s++) {
         el_pt = (char*)SEGMENT_START(pma, s);

         //If we changed segment but the lastkey is also in first postition of this segment, we incremente its "end" index;
         if (lastElKey == (*(uint64_t*)el_pt)) {
            modified.back().end++;
         }

         for (el_pt; el_pt < SEGMENT_ELT(pma, s, pma->elts[s]); el_pt += pma->elt_size) {

            if (lastElKey != (*(uint64_t*)el_pt)) {

               lastElKey = *(uint64_t*)el_pt;

               modified.emplace_back(lastElKey, s, s + 1);
            }
         }

      }

      //if the last lastKey continues outside of this range of windows we remove it;
      //        modified.pop_back();

      //ACTUALLY I THINK WE STILL NEED IT; if the begin of the last range was modified we still need to inform to the quadtree.

      /* we still need to find the end of last element (can be out of the current window) */
      //initialize 'end' with the last segment of the pma + 1
      modified.back().end = pma->nb_segments;
      for (char* seg_pt = (char*)SEGMENT_START(pma, s); seg_pt < SEGMENT_START(pma, pma->nb_segments); seg_pt += (pma->cap_segments * pma->elt_size)) {
         // Check the first key of the following segments until we find one that differs;
         if (lastElKey != *(uint64_t*)seg_pt) {
            modified.back().end = (seg_pt - (char*)pma->array) / (pma->cap_segments*pma->elt_size); //gets they index of segment that differs;
            break;
         }
      }
   }

   return 0;
}

int main(int argc, char *argv[]) {
   cimg_usage("Benchmark inserts elements in batches.");
   
   const unsigned int seg_size(cimg_option("-s", 8, "Segment size for the pma"));

   const float tau_0(cimg_option("-t0", 0.92, "pma parameter tau_0"));
   const float tau_h(cimg_option("-th", 0.7, "pma parameter tau_h"));
   const float rho_0(cimg_option("-r0", 0.08, "pma parameter rho_0"));
   const float rho_h(cimg_option("-rh", 0.3, "pma parameter rho_0"));

   int batch_size(cimg_option("-b", 10, "Batch size used in batched insertions"));
   std::string fname(cimg_option("-f", "../data/tweet100.dat", "file with tweets"));

   // Create <key,value> elements
   std::vector<elttype> input_vec = load_input(fname);

   uint32_t nb_elements = input_vec.size();
   
   std::unique_ptr<QuadtreeNode> _quadtree = std::make_unique<QuadtreeNode>(spatial_t(0, 0, 0));

   struct pma_struct *pma = (struct pma_struct *) pma::build_pma(nb_elements, sizeof(valuetype), tau_0, tau_h, rho_0, rho_h, seg_size);

   elttype * batch_start;
   int size = nb_elements / batch_size;
   int num_batches = 1 + (nb_elements - 1) / batch_size;

   map_t modifiedKeys;
   for (int k = 0; k < num_batches; k++) {
      batch_start = &input_vec[k*size];
      size = batch_size;

      std::vector<elttype> batch;
      for (size_t i = k * size; i <= ((k * size) + batch_size); i++) {
         batch.emplace_back(input_vec[i]);
      }

      // OK
      //insert_batch(pma, batch_start, size);

      // ERROR
      insert_batch(pma, batch);

      // Creates a map with begin and end of each index in the pma.
      modifiedKeys.clear();
      pma_diff(pma, modifiedKeys); //Extract information of new key range boundaries inside the pma.
      
      _quadtree->update(modifiedKeys.begin(), modifiedKeys.end());
   }
      
   /*
   std::string input_file(cimg_option("-f", "../data/tweet100.dat", "program arg: twitter input file"));

   Runner::runner_opts run_opts;
   run_opts.batch = cimg_option("-b", 10, "runner arg: batch size");
   run_opts.interval = cimg_option("-i", 10, "runner arg: insertion interval");

   std::vector<elttype> records = load_input(input_file);

   std::unique_ptr<ContainerInterface> pma = std::make_unique<PMABatch>();
   pma->create(records.size(), argc, argv);

   Runner runner(pma.get());   
   std::thread run_thread(&Runner::run, &runner, records, run_opts);
   
   bool server = false;
   Server::server_opts nds_opts;
   nds_opts.port = 7000;
   nds_opts.cache = false;
   nds_opts.multithreading = true;

   std::cout << "Server Options:" << std::endl;
   std::cout << "\tOn/Off: " << server << std::endl;
   std::cout << "\t" << nds_opts << std::endl;

   // http server
   std::unique_ptr<std::thread> server_ptr;
   if (server) server_ptr = std::make_unique<std::thread>(Server::run, nds_opts);

   if (server_ptr) {
      std::cout << "Server Running... press any key to terminate." << std::endl;
      getchar();

      Server::getInstance().stop();
      server_ptr->join();
   }
   run_thread.join();

   */
   return 0;
}
