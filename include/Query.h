#pragma once

#include "types.h"
#include "string_util.h"

class Query {
public:
   enum query_type { INVALID, TILE, REGION, DATA, TOPK, TRIGGER };

   Query(const std::string& url);

   Query(const std::vector<std::string>& tokens);

   uint32_t resolution{0};
   region_t region;
   topk_t topk_info;
   triggers_t triggers_info;


   query_type type{INVALID};
};
