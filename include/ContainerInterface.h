#pragma once
#include "stde.h"
#include "types.h"

using valuetype_function = std::function<void(const valuetype&)>;

template<typename _It>
class ContainerInterface {
public:
   // building
   virtual duration_t create(uint32_t size, int argc, char *argv[]) = 0;
   
   // updating
   virtual duration_t insert(std::vector<elttype> batch) = 0;
   virtual duration_t diff(std::vector<elinfo_t>& keys) = 0;

   // acessing
   virtual duration_t count(const _It& begin, const _It& end, const spatial_t& el, uint32_t& count) = 0;

   virtual duration_t apply(const _It& begin, const _It& end, const spatial_t& el, valuetype_function __apply) = 0;
};
