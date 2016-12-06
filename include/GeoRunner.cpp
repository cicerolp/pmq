#include "GeoRunner.h"
#include "Server.h"

#include"InputIntf.h"

GeoRunner::GeoRunner(int argc, char* argv[]) {
   std::string input_file(cimg_option("-f", "../data/tweet100.dat", "program arg: twitter input file"));

   _input = input::load(input_file, 25);

   _opts.batch = cimg_option("-b", 100, "runner arg: batch size");
   _opts.interval = cimg_option("-i", 10, "runner arg: insertion interval");
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

      // lock container
      _mutex.lock();

      // insert batch
      _container->insert(batch);

      // unlock container
      _mutex.unlock();

      // FIX remove comment
      if (_opts.hint_server /*&& keys.size() != 0*/) Server::getInstance().renew_data();

      // update iterator
      it_begin = it_curr;

      std::this_thread::sleep_for(std::chrono::milliseconds(_opts.interval));
   }

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

      default: {
         return ("[]");
      }
         break;
   }

   return buffer.GetString();
}

void GeoRunner::accum_region(uint32_t& accum, const spatial_t& area, uint32_t count) {
   accum += count;
}

void GeoRunner::write_data(json_writer& writer, uint32_t& accum, const valuetype& el) {
   static const uint32_t max = 1000;

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

