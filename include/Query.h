#pragma once

#include "types.h"
#include "string_util.h"

class Query {
public:
   enum query_type { INVALID, TILE, REGION, DATA };

   Query(const std::string& url);

   Query(const std::vector<std::string>& tokens);

   uint32_t resolution{0};
   region_t region;
   query_type type{INVALID};
};
