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

int main(int argc, char *argv[]) {
   cimg_usage("command line arguments");

   std::string input_file(cimg_option("-f", "../data/tweet100.dat", "program arg: twitter input file"));

   Runner::runner_opts run_opts;
   run_opts.batch = cimg_option("-b", 10, "runner arg: batch size");
   run_opts.interval = cimg_option("-b", 10, "runner arg: insertion interval");
   
   std::vector<elttype> records = load_input(input_file);

   std::unique_ptr<ContainerInterface> pma = std::make_unique<PMABatch>();
   pma->create(records.size(), argc, argv);

   Runner runner(pma);
   
   std::thread run_thread(&Runner::run, &runner, records, run_opts);
   
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
