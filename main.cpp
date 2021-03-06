#include "stde.h"

#include "Server.h"

#include "GeoRunner.h"
#include "GeoCtnIntf.h"

#include "PMQ.h"

#include "BTreeCtn.h"
#include "RTreeCtn.h"

uint32_t g_Quadtree_Depth = 25;

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

int main(int argc, char *argv[]) {
  cimg_usage("command line arguments");

  bool server(cimg_option("-server", false, "program arg: enable server"));

  GeoRunner &runner = GeoRunner::getInstance(argc, argv);

  // btree
  std::shared_ptr < BTreeCtn > container = std::make_shared<BTreeCtn>(argc, argv);

  const char *is_help = cimg_option("-h", (char *) 0, 0);
  if (is_help) return 0;

  container->create(runner.input_size());

  runner.set(container);

  std::thread run_thread(&GeoRunner::run, &runner);

  Server::server_opts nds_opts;
  nds_opts.port = 7000;
  nds_opts.cache = false;
  nds_opts.multithreading = true;

  // http server
  std::unique_ptr < std::thread > server_ptr;
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
  } else {
    std::cout << "*Runner* Executing... press any key to terminate." << std::endl;
    getchar();
  }

  runner.stop();
  run_thread.join();

  return 0;
}
