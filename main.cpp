#include "stde.h"

#ifdef PMA_TRACE_MOVE
  extern unsigned int g_iteration_counter;
#endif

#include "Server.h"
#include "PMAInstance.h"

uint32_t g_Quadtree_Depth = 25;

int main(int argc, char *argv[]) {

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


   PMAInstance::getInstance().create(argc, argv);
   
   
   if (server_ptr) {
      std::cout << "Server Running... press any key to terminate." << std::endl;
      getchar();

      Server::getInstance().stop();
      server_ptr->join();
   }

   PMAInstance::getInstance().destroy();
   return 0;
}
