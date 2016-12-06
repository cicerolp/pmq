#include "stde.h"

#include "Server.h"
#include "Runner.h"

#include "PMABatch.h"
#include "DenseVector.h"

#include "QuadtreeIntf.h"

#include "GeoRunner.h"
#include "GeoCtnIntf.h"

#include "GeoHash.h"
#include "PMABatchCtn.h"
#include "PostGisCtn.h"
#include "SpatiaLiteCtn.h"
#include "DenseVectorCtn.h"

uint32_t g_Quadtree_Depth = 25;

int main(int argc, char *argv[]) {
   cimg_usage("command line arguments");

   bool server(cimg_option("-server", true, "program arg: enable server"));

   GeoRunner& runner = GeoRunner::getInstance(argc, argv);

   // implicit quadtree
   //std::shared_ptr<GeoCtnIntf> container = std::make_shared<GeoHashSequential>(argc, argv);
   std::shared_ptr<GeoCtnIntf> container = std::make_shared<GeoHashBinary>(argc, argv);

   //std::shared_ptr<GeoCtnIntf> container = std::make_shared<PMABatchCtn>(argc, argv);
   //std::shared_ptr<GeoCtnIntf> container = std::make_shared<DenseCtnStdSort>();
   //std::shared_ptr<GeoCtnIntf> container = std::make_shared<DenseCtnTimSort>();
   
   // SQL
   //std::shared_ptr<GeoCtnIntf> container = std::make_shared<SpatiaLiteCtn>();
   //std::shared_ptr<GeoCtnIntf> container = std::make_shared<PostGisCtn>();   

   const char* is_help = cimg_option("-h", (char*)0, 0);
   if (is_help) return 0;

   container->create(runner.input_size());

   runner.set(container);

   std::thread run_thread(&GeoRunner::run, &runner);

   Server::server_opts nds_opts;
   nds_opts.port = 7000;
   nds_opts.cache = false;
   nds_opts.multithreading = true;

   // http server
   std::unique_ptr<std::thread> server_ptr;
   if (server) {
      Server::getInstance(nds_opts);
      server_ptr = std::make_unique<std::thread>(Server::run);
   }

   std::this_thread::sleep_for(std::chrono::milliseconds(1));

   if (server_ptr) {
      std::cout << "*Server* Running... press any key to terminate." << std::endl;
      getchar();

      Server::getInstance().stop();
      server_ptr->join();
   }
   else {
      std::cout << "*Runner* Executing... press any key to terminate." << std::endl;
      getchar();
   }

   runner.stop();
   run_thread.join();

   return 0;
}
