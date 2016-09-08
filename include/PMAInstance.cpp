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

   std::vector<tweet_t> tweet_vec;
   loadTweetFile(tweet_vec,fname);


   // Create <key,value> elements
   std::vector<elttype> input_vec;
   input_vec.reserve(tweet_vec.size());


   PRINTOUT("Computing keys for tweets... ");
   //use the spatial index as key
   for (auto& tweet : tweet_vec){
       elttype e;
       e.key = spatialKey(tweet,g_Quadtree_Depth);
       e.value = tweet;
       input_vec.emplace_back(e);
   }

   int nb_elements = input_vec.size();

   PRINTOUT(" %d teewts loaded \n",tweet_vec.size());

   pma = (struct pma_struct * ) build_pma(nb_elements,sizeof(valuetype), tau_0, tau_h, rho_0, rho_h, seg_size);
   
   // Creates a map with begin and end of each index in the pma.
   map_t range;
   quadtree = std::make_unique<SpatialElement>(spatial_t(0,0,0));

   simpleTimer t;
   elttype * batch_start;
   int size = nb_elements / batch_size;
   int num_batches = 1 + (nb_elements-1)/batch_size;

   for (int k = 0; k < num_batches; k++) {
     batch_start = &input_vec[k*size];

     if ((nb_elements-k*batch_size) / batch_size == 0){
       size = nb_elements % batch_size;
     }else{
       size = batch_size;
     }
     insert_batch(pma,batch_start,size);

     update_map(pma,range); //Extract information of new key range boundaries inside the pma.

     _ready = false;
     t.start();
     quadtree->update(range);
     t.stop();
     _ready = true;

     std::cout << "Quadtree update " << k << " in " << t.miliseconds() << "ms" << std::endl;
     sleep(1);

   }
   return true;
}

void PMAInstance::destroy() {
   destroy_pma(pma);
}

std::string PMAInstance::query(const Query& query) {

   if ( !_ready || !pma || !quadtree) return ("[]");
   
   json_ctn json;
   auto restriction = query.get<Query::spatial_query_t>();
   
   quadtree->query_tile(pma, restriction->tile, json);
      
   // serialization
   rapidjson::StringBuffer buffer;
   rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
   
   // start json
   writer.StartArray();
   
   for (auto& el : json) {
      writer.StartArray();
      writer.Uint(el.tile.x);
      writer.Uint(el.tile.y);
      writer.Uint(el.tile.z);
      writer.Uint(el.count);
      writer.EndArray();   
   }
   
   // end json
   writer.EndArray();
   
   return buffer.GetString();
}
