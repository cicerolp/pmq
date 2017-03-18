#pragma once

#include "Query.h"
#include "Singleton.h"
#include "GeoCtnIntf.h"

class GeoRunner : public Singleton<GeoRunner> {
   friend class Singleton<GeoRunner>;
public:
   struct runner_opts {
      uint32_t batch{100}; //batch size
      uint32_t interval{100}; //insertion interval between batches in milliseconds
      bool hint_server{true};
   };

   void set(std::shared_ptr<GeoCtnIntf> container);

   void run();

   inline void stop();

   std::string query(const Query& query);

   inline uint32_t input_size() const;

   inline std::pair<float, float> convert_grid_index(uint32_t index) const;

private:
   GeoRunner(int argc, char* argv[]);

   GeoRunner() = default;

   virtual ~GeoRunner() = default;

   void grid_runner();

   static void accum_region(uint32_t& accum, const spatial_t& area, uint32_t count);

   static void write_data(json_writer& writer, uint32_t& accum, const valuetype& el);

   static void write_tile(json_writer& writer, uint32_t& min, uint32_t& max, const spatial_t& area, uint32_t count);

private:
   bool _running{true};

   std::mutex _mutex;

   runner_opts _opts;
   std::vector<elttype> _input;

   std::shared_ptr<GeoCtnIntf> _container;

   uint32_t _trigger_alert, _trigger_n_batch;
   uint32_t _x_grid, _y_grid, _n_grid{1};
   std::vector<uint32_t> _grid;

   std::mutex _grid_mutex;
   std::condition_variable _grid_condition;

   std::unique_ptr<std::thread> _grid_thread;
   std::queue<std::vector<elttype>> _grid_buffer;
};

void GeoRunner::stop() {
   _running = false;
}

uint32_t GeoRunner::input_size() const {
   return (uint32_t)_input.size();
};

std::pair<float, float> GeoRunner::convert_grid_index(uint32_t index) const {
   float x = (float)std::floor(index / _y_grid);
   float y = (float)(index - (x * _y_grid));

   return {x - 180, y - 90};
}
