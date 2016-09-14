#pragma once

#include "types.h"
#include "string_util.h"

class Query {
public:
   enum query_type { INVALID, TILE, REGION };
   
   Query(const std::string& url);
   Query(const std::vector<std::string>& tokens);

   friend std::ostream& operator<<(std::ostream& os, const Query& query);

    struct query_t {
      query_t(query_type type) : type(type) {}
      query_type type;
   };
   struct spatial_query_t : public query_t {
      spatial_query_t() : query_t(TILE) {}
      uint32_t resolution {0};
      region_t region;
   };
   struct region_query_t : public query_t {
      region_query_t() : query_t(REGION) {}
      region_t region;
   };

   inline query_type type() const {
      return restriction ? restriction->type : INVALID;
   }
   
   inline bool eval() const {
      return restriction != nullptr;
   }

   template<typename T>
   inline T* get() const {
      return (T*)restriction.get();
   }

   template<typename T>
   inline T* get() {
      return (T*)restriction.get();
   }

private:  
   std::unique_ptr<query_t> restriction;
};
