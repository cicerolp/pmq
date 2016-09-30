#pragma once
#include "stde.h"
#include "Query.h"
#include "QuadtreeNode.h"
#include "ContainerInterface.h"

class Runner {
public:
   struct runner_opts {
      uint32_t batch{100};
      uint32_t interval{100};
      bool hint_server{false};
   };

   Runner(std::unique_ptr<ContainerInterface>& container);
   virtual ~Runner() = default;

   void run(const std::vector<elttype>& records, const runner_opts& opts);
   std::string query(const Query& query);

private:
   static void write_el(json_writer& runner, const valuetype& el);

   std::mutex _mutex;
   
   std::unique_ptr<QuadtreeNode> _quadtree;
   std::unique_ptr<ContainerInterface> _container;
};
