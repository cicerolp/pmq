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
      uint64_t now{0};
   };

   struct grid_coord {
      float lat0, lon0, lat1, lon1;

      bool operator==(const grid_coord& other) const {
         return lat0 == other.lat0 &&
            lat1 == other.lat1 &&
            lon0 == other.lon0 &&
            lon1 == other.lon1;
      }
   };

   void set(std::shared_ptr<GeoCtnIntf> container);

   void run();

   inline void stop();

   std::string query(const Query& query);

   inline uint32_t input_size() const;

   inline grid_coord grid_coord_to_index(float lat, float lon) const;

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

   uint32_t _trigger_alert;
   uint32_t _x_grid, _y_grid;
   std::vector<uint32_t> _grid;

   std::mutex _grid_mutex;
   std::condition_variable _grid_condition;

   std::unique_ptr<std::thread> _grid_thread;
   std::queue<std::vector<elttype>> _grid_buffer;
};

namespace std {
   template <>
   struct hash<GeoRunner::grid_coord> {
      typedef GeoRunner::grid_coord argument_type;
      typedef std::size_t result_type;

      std::size_t operator()(argument_type const& obj) const {
         result_type const h1(std::hash<float>{}(obj.lat0));
         result_type const h2(std::hash<float>{}(obj.lat1));
         result_type const h3(std::hash<float>{}(obj.lon0));
         result_type const h4(std::hash<float>{}(obj.lon1));

         return ((h1 ^ (h2 << 1)) ^ (h3 << 2)) ^ (h4 << 3);
      }
   };

}

void GeoRunner::stop() {
   _running = false;
}

uint32_t GeoRunner::input_size() const {
   return (uint32_t)_input.size();
};

GeoRunner::grid_coord GeoRunner::grid_coord_to_index(float lat, float lon) const {
   grid_coord index;

   index.lat0 = std::floor(lat);
   index.lon0 = std::floor(lon);

   index.lat1 = std::ceil(lat);
   index.lon1 = std::ceil(lon);

   return index;
}
