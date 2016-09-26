#include "stde.h"

#ifdef PMA_TRACE_MOVE
  extern unsigned int g_iteration_counter;
#endif

#include "Server.h"
#include "PMAInstance.h"

uint32_t g_Quadtree_Depth = 25;

int main(int argc, char *argv[]) {

   uint32_t z_diff_2 = (25 - 7 - 1) * 2;

   std::cout << ((222844826865401 >> z_diff_2) & 3) << std::endl;
   std::cout << ((222845364849203 >> z_diff_2) & 3) << std::endl;
   std::cout << ((222846651023631 >> z_diff_2) & 3) << std::endl;
   std::cout << ((222847165741530 >> z_diff_2) & 3) << std::endl;
   std::cout << ((222847311711782 >> z_diff_2) & 3) << std::endl;
   std::cout << ((222848171958240 >> z_diff_2) & 3) << std::endl;
   std::cout << ((222854373566221 >> z_diff_2) & 3) << std::endl;

   std::cout << std::endl;

   z_diff_2 = (25 - 8 - 1) * 2;

   std::cout << ((222844826865401 >> z_diff_2) & 3) << std::endl;
   std::cout << ((222845364849203 >> z_diff_2) & 3) << std::endl;
   std::cout << ((222846651023631 >> z_diff_2) & 3) << std::endl;
   std::cout << ((222847165741530 >> z_diff_2) & 3) << std::endl;
   std::cout << ((222847311711782 >> z_diff_2) & 3) << std::endl;
   std::cout << ((222848171958240 >> z_diff_2) & 3) << std::endl;
   std::cout << ((222854373566221 >> z_diff_2) & 3) << std::endl;

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
