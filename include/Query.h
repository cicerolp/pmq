#pragma once
#include "stde.h"

#include "types.h"
#include "string_util.h"

class Query {
public:
   Query(const std::string& url);
   Query(const std::vector<std::string>& tokens);

   friend std::ostream& operator<<(std::ostream& os, const Query& query);

   struct spatial_query_t {
      uint32_t resolution {0};
      spatial_t tile;
   };

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
   std::unique_ptr<spatial_query_t> restriction;
};
