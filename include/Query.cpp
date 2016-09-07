#include "Query.h"

Query::Query(const std::string& url) : Query(string_util::split(url, "[/]+")) { }

Query::Query(const std::vector<std::string>& tokens) : Query(tokens[3], tokens[4]) {
   for (auto it = tokens.begin() + 5; it != tokens.end(); ++it) {
      if ((*it) == "tile") {
         auto key = std::stoul(string_util::next_token(it));

         if (!restrictions[key]) restrictions[key] = std::make_unique<spatial_query_t>();

         auto x = std::stoi(string_util::next_token(it));
         auto y = std::stoi(string_util::next_token(it));
         auto z = std::stoi(string_util::next_token(it));

         get<spatial_query_t>(key)->tile.emplace_back(x, y, z);
         get<spatial_query_t>(key)->resolution = std::stoi(string_util::next_token(it));

      }
   }
}

Query::Query(const std::string& instance, const std::string& type) : _instance(instance) {
   if (type == "tile") {
      this->_type = TILE;
   } else if (type == "group") {
      this->_type = GROUP;
   } else if (type == "tseries") {
      this->_type = TSERIES;
   } else if (type == "scatter") {
      this->_type = SCATTER;
   } else if (type == "region") {
      this->_type = REGION;
   } else if (type == "mysql") {
      this->_type = MYSQL;
   }
   restrictions.resize(8);
}

std::ostream& operator<<(std::ostream& os, const Query& query) {
   os << "/" + query.instance();

   if (query.type() == Query::TILE) os << "/tile";

   for (int index = 0; index < query.restrictions.size(); ++index) {
      auto& r = query.restrictions[index];
      if (r == nullptr) continue;

      if (r->id == Query::query_t::spatial) {
         auto spatial = static_cast<Query::spatial_query_t*>(r.get());
         // /tile/key/x/y/z/r
         for (auto& tile : spatial->tile) {
            os << "/tile/" << index << "/" << tile << "/" << spatial->resolution;
         }
      }
   }

   return os;
}

