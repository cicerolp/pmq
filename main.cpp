#include "stde.h"

#include "Server.h"

#include "Runner.h"
#include "PMABatch.h"
#include "QuadtreeIntf.h"

uint32_t g_Quadtree_Depth = 25;

int main(int argc, char *argv[]) {
   cimg_usage("command line arguments");

   Runner& runner = Runner::getInstance(argc, argv);

   std::shared_ptr<ContainerIntf> container = std::make_shared<PMABatch>(argc, argv);
   std::shared_ptr<QuadtreeIntf> quadtree = std::make_shared<QuadtreeIntf>(spatial_t(0, 0, 0));

   const char* is_help = cimg_option("-h", (char*)0, 0);
   if (is_help) 0;

   container->create(runner.input_size());
   
   runner.set(container, quadtree);

   std::thread run_thread(&Runner::run, &runner);
   
   bool server = true;
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
   
   return 0;
}
