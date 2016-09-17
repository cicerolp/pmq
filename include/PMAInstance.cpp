#include "PMAInstance.h"

bool PMAInstance::create(int argc, char *argv[]) {
   cimg_usage("Benchmark inserts elements in batches.");
   //const unsigned int nb_elements ( cimg_option("-n",100,"Number of elements to insert"));
   const unsigned int seg_size ( cimg_option("-s",8,"Segment size for the pma"));
   const int batch_size ( cimg_option("-b",10,"Batch size used in batched insertions"));
   const float tau_0 ( cimg_option("-t0",0.92,"pma parameter tau_0"));
   const float tau_h ( cimg_option("-th",0.7,"pma parameter tau_h"));
   const float rho_0 ( cimg_option("-r0",0.08,"pma parameter rho_0"));
   const float rho_h ( cimg_option("-rh",0.3,"pma parameter rho_0"));
   std::string fname ( cimg_option("-f","../data/tweet100.dat","file with tweets"));

   const char* is_help = cimg_option("-h",(char*)0,0);

   if (is_help) return false;

   PRINTOUT("Loading twitter dataset... %s \n",fname.c_str());

   // Create <key,value> elements
   std::vector<elttype> input_vec;
   loadTweetFile(input_vec, fname);

   int nb_elements = input_vec.size();

   PRINTOUT(" %d teewts loaded \n", (uint32_t)input_vec.size());

   pma = (struct pma_struct * ) build_pma(nb_elements, sizeof(valuetype), tau_0, tau_h, rho_0, rho_h, seg_size);
        
   
   simpleTimer t;
   elttype * batch_start;
   int size = nb_elements / batch_size;
   int num_batches = 1 + (nb_elements-1)/batch_size;

   for (int k = 0; k < num_batches; k++) {
      batch_start = &input_vec[k*size];

      if ((nb_elements-k*batch_size) / batch_size == 0) {
         size = nb_elements % batch_size;
      } else {
         size = batch_size;
      }
      // lock pma and quadtree update
      mutex.lock();

      insert_batch(pma, batch_start, size);

#ifndef NDEBUG
      PRINTOUT( "PMA WINDOWS : ") ;
      for (auto k: *(pma->last_rebalanced_segs)){
          std::cout << k << " "; //<< std::endl;
      }
#endif
      std::cout << "\n";

      // Creates a map with begin and end of each index in the pma.
      map_t modifiedKeys;
      t.start();
      pma_diff(pma,modifiedKeys); //Extract information of new key range boundaries inside the pma.
      t.stop();

      PRINTCSVL("ModifiedKeys", t.miliseconds(),"ms" );

      if (quadtree == nullptr)
         quadtree = std::make_unique<SpatialElement>(spatial_t(0,0,0));

#ifndef NDEBUG
      PRINTOUT("ModifiedKeys %d : ",modifiedKeys.size()) ;
      for (auto& k: modifiedKeys){
          std::cout << k.key.mCode << " " ;
      }

      std::cout << "\n";

      PRINTOUT("pma keys: ");
      print_pma_keys(pma);
#endif
      t.start();
      quadtree->update(modifiedKeys.begin(), modifiedKeys.end());
      if (modifiedKeys.size() != 0) up_to_date = false;     
      t.stop();
      PRINTCSVL("QuadtreeUpdate" , t.miliseconds(),"ms" , k);
      // unlock pma and quadtree update
      mutex.unlock();

      std::this_thread::sleep_for(std::chrono::milliseconds(100));      
   }
   return true;
}

void PMAInstance::destroy() {
   destroy_pma(pma);
}

std::string PMAInstance::query(const Query& query) {

   if (!quadtree) return ("[]");

   std::vector<SpatialElement*> json;
   simpleTimer t1;

   // serialization
   rapidjson::StringBuffer buffer;
   rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

   // start json
   writer.StartArray();

   switch (query.type()) {
      case Query::TILE: {
         auto restriction = query.get<Query::spatial_query_t>();

         mutex.lock();
         quadtree->query_tile(restriction->region, json);

         uint32_t max = 0;
         uint32_t min = std::numeric_limits<uint32_t>::max();

         writer.StartObject();

         writer.String("data");      
         writer.StartArray();      
         for (auto& el : json) {
            uint32_t x, y;
            mortonDecode_RAM(el->code(), x, y);

            writer.StartArray();

            writer.Double(mercator_util::tiley2lat(y, el->zoom()));
            writer.Double(mercator_util::tilex2lon(x, el->zoom()));

            uint32_t count = count_elts_pma(pma, el->begin(), el->end(), el->code(), el->zoom());

            max = std::max(max, count);
            min = std::min(min, count);

            writer.Uint(count);
            writer.EndArray();
         }
         writer.EndArray();      

         writer.String("min");
         writer.Uint(min);

         writer.String("max");
         writer.Uint(max);

         writer.EndObject();

         mutex.unlock();
      } break;

      case Query::REGION: {
         auto restriction = query.get<Query::region_query_t>();

         mutex.lock();
         t1.start();
         quadtree->query_region(restriction->region, json);
         t1.stop();
         PRINTCSVL("Quadtree_query",t1.miliseconds(),"ms");

         uint32_t count = 0;

         t1.start();
         for (auto& el : json) {         
            count += count_elts_pma(pma, el->begin(), el->end(), el->code(), el->zoom());
         }
         t1.stop();
         PRINTCSVL("PMA_query",t1.miliseconds(),"ms");
         mutex.unlock();
         writer.Uint(count);
      } break;
      
      default: {
         return ("[]");
      } break;
   }

   // end json
   writer.EndArray();

   return buffer.GetString();
}

std::string PMAInstance::update() {
   if (up_to_date) {
      return ("[true]");   
   } else {
      up_to_date = true;
      return ("[false]");
   }
}
