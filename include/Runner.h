#pragma once
#include "Singleton.h"
#include "Query.h"
#include "QuadtreeIntf.h"
#include "ContainerIntf.h"

class Runner : public Singleton<Runner> {
   friend class Singleton<Runner>;
public:
   struct runner_opts {
      uint32_t batch{100}; //batch size
      uint32_t interval{100}; //insertion interval between batches in milliseconds
      bool hint_server{true};
   };

   void set(std::shared_ptr<ContainerIntf> container, std::shared_ptr<QuadtreeIntf>& quadtree);

   void run();
   inline void stop();

   std::string query(const Query& query);

   inline uint32_t input_size() const;

private:
   Runner(int argc, char *argv[]);
   Runner() = default;
   virtual ~Runner() = default;

   static void write_el(json_writer& runner, const valuetype& el);

   bool _running{ true };

   std::mutex _mutex;
   
   runner_opts _opts;
   std::vector<elttype> _input;

   std::shared_ptr<QuadtreeIntf> _quadtree;
   std::shared_ptr<ContainerIntf> _container;
};

void Runner::stop() {
   _running = false;
}

uint32_t Runner::input_size() const {
   return (uint32_t)_input.size();
};
