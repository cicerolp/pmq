#include "GeoRunner.h"
#include "Server.h"

#include"InputIntf.h"

GeoRunner::GeoRunner(int argc, char* argv[]) {
   std::string input_file(cimg_option("-f", "../data/tweet100.dat", "program arg: twitter input file"));
   _x_grid = std::max(cimg_option("-x_grid", 360, "program arg: grid resolution x"), 180);
   _y_grid = std::max(cimg_option("-y_grid", 180, "program arg: grid resolution y"), 360);
   _trigger_alert = std::max(cimg_option("-alert", 20, "program arg: trigger alert"), 0);

   _opts.batch = cimg_option("-b", 100, "runner arg: batch size");
   _opts.interval = cimg_option("-i", 10, "runner arg: insertion interval");

   _input = input::load(input_file, 25, _opts.batch);

   _grid.resize(_x_grid * _y_grid, 0);

   _grid_thread = std::make_unique<std::thread>(&GeoRunner::grid_runner, this);
}

void GeoRunner::set(std::shared_ptr<GeoCtnIntf> container) {
   _mutex.lock();
   _container = container;
   _mutex.unlock();
}

void GeoRunner::run() {
   if (!_container) return;

   std::vector<elttype>::iterator it_begin = _input.begin();
   std::vector<elttype>::iterator it_curr = _input.begin();

   while (it_begin != _input.end() && _running) {
      it_curr = std::min(it_begin + _opts.batch, _input.end());

      std::vector<elttype> batch(it_begin, it_curr);

      _opts.now = batch.back().value.time;

      std::unique_lock<std::mutex> lock(_grid_mutex);
      // add tweets to grid buffer
      _grid_buffer.emplace(batch);
      lock.unlock();
      _grid_condition.notify_all();

      // lock container
      _mutex.lock();

      // insert batch
      _container->insert(batch);

      // unlock container
      _mutex.unlock();

      if (_opts.hint_server) Server::getInstance().renew_data();

      // update iterator
      it_begin = it_curr;

      std::this_thread::sleep_for(std::chrono::milliseconds(_opts.interval));
   }

   _running = false;
   _grid_condition.notify_all();

   _grid_thread->join();

   std::cout << "*Runner* Stopped." << std::endl;
}

std::string GeoRunner::query(const Query& query) {
   if (!_container) return "[]";

   // serialization
   rapidjson::StringBuffer buffer;
   rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

   switch (query.type) {
      case Query::TILE: {
         uint32_t max = 0;
         uint32_t min = std::numeric_limits<uint32_t>::max();

         applytype_function _apply = std::bind(GeoRunner::write_tile, std::ref(writer), std::ref(min),
                                               std::ref(max), std::placeholders::_1, std::placeholders::_2);

         // start json
         writer.StartArray();

         writer.StartObject();

         writer.String("data");
         writer.StartArray();

         // lock mutex
         _mutex.lock();

         _container->apply_at_tile(query.region, _apply);

         // unlock mutex
         _mutex.unlock();

         writer.EndArray();

         writer.String("min");
         writer.Uint(min);

         writer.String("max");
         writer.Uint(max);

         writer.EndObject();

         // end json
         writer.EndArray();
      }
         break;

      case Query::REGION: {
         uint32_t count = 0;
         applytype_function _apply = std::bind(GeoRunner::accum_region, std::ref(count),
                                               std::placeholders::_1, std::placeholders::_2);

         // start json
         writer.StartArray();

         // lock mutex
         _mutex.lock();

         _container->apply_at_region(query.region, _apply);

         // unlock mutex
         _mutex.unlock();

         writer.Uint(count);

         // end json
         writer.EndArray();
      }
         break;

      case Query::DATA: {
         writer.StartObject();

         writer.String("draw");
         writer.Int(1);

         uint32_t count = 0;
         scantype_function _apply = std::bind(GeoRunner::write_data, std::ref(writer),
                                              std::ref(count), std::placeholders::_1);

         writer.String("data");
         writer.StartArray();

         // lock mutex
         _mutex.lock();

         _container->scan_at_region(query.region, _apply);

         // unlock mutex
         _mutex.unlock();

         writer.EndArray();

         writer.String("recordsTotal");
         writer.Int(count);

         writer.String("recordsFiltered");
         writer.Int(count);

         writer.EndObject();
      }
         break;

      case Query::TOPK: {
         writer.StartObject();

         topk_t topk_info = query.topk_info;
         topk_info.now = _opts.now;

         uint32_t count = 0;
         scantype_function _apply = std::bind(GeoRunner::write_data, std::ref(writer),
                                              std::ref(count), std::placeholders::_1);

         writer.String("data");
         writer.StartArray();

         // lock mutex
         _mutex.lock();

         _container->topk_search(query.region, topk_info, _apply);

         // unlock mutex
         _mutex.unlock();

         writer.EndArray();
         writer.EndObject();
      }
         break;

      default: {
         return ("[]");
      }
         break;
   }

   return buffer.GetString();
}

void GeoRunner::grid_runner() {
   while (true) {
      std::unique_lock<std::mutex> lock(_grid_mutex);

      _grid_condition.wait(lock, [this] { return !_running || _grid_buffer.size() != 0; });

      if (!_running && _grid_buffer.empty()) return;

      auto item = _grid_buffer.front();
      _grid_buffer.pop();

      lock.unlock();

      std::unordered_map<grid_coord, uint32_t> grid_map;

      for (auto& el : item) {
         grid_coord index = grid_coord_to_index(el.value.latitude, el.value.longitude);
         grid_map[index]++;
      }

      for (auto& el : grid_map) {
         if (el.second > _trigger_alert) {
            
            Server::getInstance().push_trigger(el.first);
         }
      }
   }
}

void GeoRunner::accum_region(uint32_t& accum, const spatial_t& area, uint32_t count) {
   accum += count;
}

void GeoRunner::write_data(json_writer& writer, uint32_t& accum, const valuetype& el) {
   static const uint32_t max = 100;

   if (accum >= max) return;

   writer.StartArray();
   writer.Uint((unsigned int)el.time);
   writer.Uint(el.language);
   writer.Uint(el.device);
   writer.Uint(el.app);
   writer.EndArray();

   accum += 1;
}

void GeoRunner::write_tile(json_writer& writer, uint32_t& min, uint32_t& max, const spatial_t& area, uint32_t count) {
   // update min/max
   max = std::max(max, count);
   min = std::min(min, count);

   uint32_t x, y;
   mortonDecode_RAM(area.code, x, y);

   writer.StartArray();

   writer.Double(mercator_util::tiley2lat(y + 0.5, area.z));
   writer.Double(mercator_util::tilex2lon(x + 0.5, area.z));

   writer.Uint(count);
   writer.EndArray();
}
