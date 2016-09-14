#include "Query.h"

Query::Query(const std::string& url) : Query(string_util::split(url, "[/]+")) { }

Query::Query(const std::vector<std::string>& tokens) {
   for (auto it = tokens.begin() + 2; it != tokens.end(); ++it) {
      
      if ((*it) == "tile") {
         if (!restriction) restriction = std::make_unique<spatial_query_t>();
         
         auto x = std::stoi(string_util::next_token(it));
         auto y = std::stoi(string_util::next_token(it));
         auto z = std::stoi(string_util::next_token(it));

         get<spatial_query_t>()->tile.emplace_back(spatial_t(x, y, z));
         
      } else if ((*it) == "resolution") {
          if (!restriction) restriction = std::make_unique<spatial_query_t>();
         
         get<spatial_query_t>()->resolution = std::stoi(string_util::next_token(it));
         
      } else if ((*it) == "region") {
          if (!restriction) restriction = std::make_unique<region_query_t>();
         
         auto z = std::stoi(string_util::next_token(it));
         auto x0 = std::stoi(string_util::next_token(it));
         auto y0 = std::stoi(string_util::next_token(it));
         auto x1 = std::stoi(string_util::next_token(it));
         auto y1 = std::stoi(string_util::next_token(it));
          
         get<region_query_t>()->region = region_t(x0, y0, x1, y1, z);         
      }
   }
}

std::ostream& operator<<(std::ostream& os, const Query& query) {
   auto& r = query.restriction;
   if (r == nullptr) return os;

   switch (r->type) {
      case Query::TILE: {
         auto spatial = query.get<Query::spatial_query_t>();
   
         for (auto& el :spatial->tile) {
            os << "/tile/" << el;
         }   
         os << "/resolution/" << spatial->resolution;
         
      } break;      
      case Query::REGION: {
         auto el = query.get<Query::region_query_t>();
         os << "/region/" << el->region; 
      } break;
      default: {
         os << "/error";    
      } break;
   }

   return os;
}

