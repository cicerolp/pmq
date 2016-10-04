#pragma once
#include "Singleton.h"
#include "Query.h"
#include "QuadtreeIntf.h"
#include "ContainerIntf.h"

class Runner : public Singleton<Runner> {
   friend class Singleton<Runner>;
public:
   struct runner_opts {
      uint32_t batch{100};
      uint32_t interval{100};
      bool hint_server{true};
   };

   void set(std::shared_ptr<ContainerIntf> container, std::shared_ptr<QuadtreeIntf>& quadtree);

   void run();
   std::string query(const Query& query);

   inline uint32_t input_size() const;

private:
   Runner(int argc, char *argv[]);
   Runner() = default;
   virtual ~Runner() = default;

   static void write_el(json_writer& runner, const valuetype& el);
   static std::vector<elttype> load_input(const std::string& fname);

   std::mutex _mutex;
   
   runner_opts _opts;
   std::vector<elttype> _input;

   std::shared_ptr<QuadtreeIntf> _quadtree;
   std::shared_ptr<ContainerIntf> _container;
};

uint32_t Runner::input_size() const {
   return _input.size();
};
