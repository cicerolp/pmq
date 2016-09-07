#include "Query.h"

Query::Query(const std::string& url) : Query(string_util::split(url, "[/]+")) { }

Query::Query(const std::vector<std::string>& tokens) {
   for (auto it = tokens.begin() + 2; it != tokens.end(); ++it) {
      if ((*it) == "tile") {
         if (!restriction) restriction = std::make_unique<spatial_query_t>();

         auto x = std::stoi(string_util::next_token(it));
         auto y = std::stoi(string_util::next_token(it));
         auto z = std::stoi(string_util::next_token(it));

         get<spatial_query_t>()->tile = spatial_t(x, y, z);
         get<spatial_query_t>()->resolution = std::stoi(string_util::next_token(it));

      }
   }
}

std::ostream& operator<<(std::ostream& os, const Query& query) {
   auto& r = query.restriction;
   if (r == nullptr) return os;

   auto spatial = static_cast<Query::spatial_query_t*>(r.get());
   os << spatial->tile << "/" << spatial->resolution;

   return os;
}

