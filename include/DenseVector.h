#pragma once
#include "ContainerIntf.h"

class DenseVector : public ContainerIntf {
public:
   DenseVector() = default;
   virtual ~DenseVector();

   // building
   duration_t create(uint32_t size) override final;

   // updating
   duration_t insert(std::vector<elttype> batch) override final;
   duration_t diff(std::vector<elinfo_t>& keys) override final;

   // acessing
   duration_t count(const uint32_t& begin, const uint32_t& end, const spatial_t& el, uint32_t& count) const override final;

   // iterating
   duration_t apply(const uint32_t& begin, const uint32_t& end, const spatial_t& el, uint32_t& count, uint32_t max, valuetype_function __apply) const override final;
   duration_t apply(const uint32_t& begin, const uint32_t& end, const spatial_t& el, elttype_function __apply) const override final;

private:
   uint32_t _diff_index;
   std::vector<elttype> _container;
};
