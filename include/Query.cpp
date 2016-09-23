#include "Query.h"

Query::Query(const std::string& url) : Query(string_util::split(url, "[/]+")) { }

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
      }
   }
}
