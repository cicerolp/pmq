#pragma once
#include "stde.h"
#include "types.h"

using valuetype_function = std::function<void(const valuetype&)>;
using elttype_function = std::function<void(const void*)>;

class ContainerIntf {
public:
   static constexpr const char* name() {return "ConatinerIntf";}

   // building
   virtual duration_t create(uint32_t size) = 0;
   
   // updating
   virtual duration_t insert(std::vector<elttype> batch) = 0;
   virtual duration_t diff(std::vector<elinfo_t>& keys) = 0;

   // acessing
   virtual duration_t count(const uint32_t& begin, const uint32_t& end, const spatial_t& el, uint32_t& count) const = 0;

   // iterating
   virtual duration_t apply(const uint32_t& begin, const uint32_t& end, const spatial_t& el, uint32_t& count, uint32_t max, valuetype_function __apply) const = 0;
   virtual duration_t apply(const uint32_t& begin, const uint32_t& end, const spatial_t& el, elttype_function __apply) const = 0;
};
