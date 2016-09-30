#include "Runner.h"
#include "Server.h"

Runner::Runner(std::unique_ptr<ContainerInterface>& container) : _container(std::move(container)) {
   _quadtree = std::make_unique<QuadtreeNode>(spatial_t(0, 0, 0));
}

void Runner::run(const std::vector<elttype>& records, const runner_opts& opts) {

   map_t keys;

   std::vector<elttype>::const_iterator it_begin = records.begin();
   std::vector<elttype>::const_iterator it_curr = records.begin();

   while (it_begin != records.end()) {
      it_curr = std::min(it_begin + opts.batch, records.end());
      
      std::vector<elttype> batch(it_begin, it_curr);

      // lock container and quadtree update
      _mutex.lock();

      // insert batch
      _container->insert(batch);

      // retrieve modified keys
      keys.clear();      
      _container->diff(keys);

      // update quadtree
      _quadtree->update(keys.begin(), keys.end());

      // unlock container and quadtree update
      _mutex.unlock();

      if (opts.hint_server && keys.size() != 0)
         Server::getInstance().renew_data();

      // update iterator
      it_begin = it_curr;

      std::this_thread::sleep_for(std::chrono::milliseconds(opts.interval));
   }
}

std::string Runner::query(const Query& query) {
   std::vector<QuadtreeNode*> json;
   
   // serialization
   rapidjson::StringBuffer buffer;
   rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

   switch (query.type) {
      case Query::TILE: {
         // start json
         writer.StartArray();

         // lock mutex
         _mutex.lock();

         _quadtree->query_tile(query.region, json);

         uint32_t max = 0;
         uint32_t min = std::numeric_limits<uint32_t>::max();
                  
         writer.StartObject();

         writer.String("data");      
         writer.StartArray();      
         for (auto& el : json) {
            uint32_t x, y;
            mortonDecode_RAM(el->el().code, x, y);

            writer.StartArray();

            writer.Double(mercator_util::tiley2lat(y, el->el().z));
            writer.Double(mercator_util::tilex2lon(x, el->el().z));

            uint32_t count = 0;
            _container->count(el->begin(), el->end(), el->el(), count);

            max = std::max(max, count);
            min = std::min(min, count);

            writer.Uint(count);
            writer.EndArray();
         }

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
      } break;

      case Query::REGION: {
         // start json
         writer.StartArray();

         // lock mutex
         _mutex.lock();

         _quadtree->query_region(query.region, json);

         uint32_t count = 0;
         for (auto& el : json) {         
            _container->count(el->begin(), el->end(), el->el(), count);
         }

         // unlock mutex
         _mutex.unlock();

         writer.Uint(count);

         // end json
         writer.EndArray();
      } break;
   
      case Query::DATA: {
         writer.StartObject();

         writer.String("draw");
         writer.Int(1);

         uint32_t count = 0;
         valuetype_function _apply = std::bind(Runner::write_el, std::ref(writer), std::placeholders::_1);

         // lock mutex
         _mutex.lock();

         _quadtree->query_region(query.region, json);

         writer.String("data");
         writer.StartArray();
         for (auto& el : json) {
            _container->apply(el->begin(), el->end(), el->el(), count, _apply);
         }
         writer.EndArray();

         // unlock mutex
         _mutex.unlock();

         writer.String("recordsTotal");
         writer.Int(count);

         writer.String("recordsFiltered");
         writer.Int(count);

         writer.EndObject();
      } break;

      default: {
         return ("[]");
      } break;
   }

   return buffer.GetString();
}

void Runner::write_el(json_writer& writer, const valuetype& el) {
   writer.StartArray();
   writer.Uint(el.time);
   writer.Uint(el.language);
   writer.Uint(el.device);
   writer.Uint(el.app);
   writer.EndArray();
}
