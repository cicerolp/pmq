#include "Query.h"

Query::Query(const std::string& url) : Query(string_util::split(url, "[/]+")) {
}

Query::Query(const std::vector<std::string>& tokens) {
   for (auto it = tokens.begin() + 2; it != tokens.end(); ++it) {

      if ((*it) == "tile") {
         type = TILE;

         auto z = std::stoi(string_util::next_token(it));
         auto x0 = std::stoi(string_util::next_token(it));
         auto y0 = std::stoi(string_util::next_token(it));
         auto x1 = std::stoi(string_util::next_token(it));
         auto y1 = std::stoi(string_util::next_token(it));

         region = region_t(x0, y0, x1, y1, z);

      } else if ((*it) == "resolution") {
         type = TILE;

         resolution = std::stoi(string_util::next_token(it));

      } else if ((*it) == "region") {
         type = REGION;

         auto z = std::stoi(string_util::next_token(it));
         auto x0 = std::stoi(string_util::next_token(it));
         auto y0 = std::stoi(string_util::next_token(it));
         auto x1 = std::stoi(string_util::next_token(it));
         auto y1 = std::stoi(string_util::next_token(it));

         region = region_t(x0, y0, x1, y1, z);

      } else if ((*it) == "data") {
         type = DATA;

         auto z = std::stoi(string_util::next_token(it));
         auto x0 = std::stoi(string_util::next_token(it));
         auto y0 = std::stoi(string_util::next_token(it));
         auto x1 = std::stoi(string_util::next_token(it));
         auto y1 = std::stoi(string_util::next_token(it));

         region = region_t(x0, y0, x1, y1, z);

      } else if ((*it) == "topk") {
         type = TOPK;

         auto z = std::stoi(string_util::next_token(it));

         auto lat = std::stof(string_util::next_token(it));
         auto lon = std::stof(string_util::next_token(it));

         topk_info.alpha = std::stof(string_util::next_token(it));
         topk_info.radius = std::stof(string_util::next_token(it));
         topk_info.k = std::stoi(string_util::next_token(it));
         topk_info.now = std::stoull(string_util::next_token(it));
         topk_info.time = std::stoull(string_util::next_token(it));

         region = region_t(lat, lon, topk_info.radius, z);

      } else if ((*it) == "triggers") {
         type = TRIGGER;

         triggers_info.frequency = std::stof(string_util::next_token(it));         
      }
   }
}
