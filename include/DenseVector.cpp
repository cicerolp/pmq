#include "DenseVector.h"

duration_t DenseVector::create(uint32_t size) {
   Timer t;
   t.start();
   _container.reserve(size);
   t.stop();
   return t;
}

duration_t DenseVector::insert(std::vector<elttype> batch) {
   Timer t;
   t.start();
   // std algorithm

   // we need the batch sorted
   sort(batch);

   // first element in batch
   auto ref = batch.front();
   // find insertion point before sorting
   auto ref_it = std::lower_bound(_container.begin(), _container.end(), ref);
   // store initial diff index
   _diff_index = ref_it - _container.begin();

   // insert batch at end
   _container.insert(_container.end(), batch.begin(), batch.end());

   // sorting algorithm
   sort(_container);
   t.stop();
   return t;
}

duration_t DenseVector::diff(std::vector<elinfo_t>& keys) {
   Timer t;
   t.start();
   
   uint32_t begin_index = _diff_index;
   uint32_t curr_index = _diff_index;

   uint64_t last_key = _container[curr_index].key;

   while (curr_index < _container.size()) {

      if (_container[curr_index].key != last_key) {
         keys.emplace_back(last_key, begin_index, curr_index);

         begin_index = curr_index;
         last_key = _container[curr_index].key;
      }
      curr_index++;
   }

   keys.emplace_back(last_key, begin_index, curr_index);

   t.stop();
   return t;
}

duration_t DenseVector::count(const uint32_t& begin, const uint32_t& end, const spatial_t& el, uint32_t& count) const {
   assert(begin >= 0 && begin < end && end <= _container.size());

   Timer t;
   t.start();

   count += end - begin;
   t.stop();
   return t;
}

duration_t DenseVector::apply(const uint32_t& begin, const uint32_t& end, const spatial_t& el, uint32_t& count, uint32_t max, valuetype_function __apply) const {
   assert(begin >= 0 && begin < end && end <= _container.size());

   Timer t;
   t.start();

   uint32_t curr = begin;
   while (curr < end) {
      if (count >= max) break;
      __apply(_container[curr].value);

      curr++;
      count++;
   }
   t.stop();
   return t;
}

duration_t DenseVector::apply(const uint32_t& begin, const uint32_t& end, const spatial_t& el, elttype_function __apply) const {
   assert(begin >= 0 && begin < end && end <= _container.size());

   Timer t;
   t.start();
   uint32_t curr = begin;
   while (curr < end) {
      __apply(&_container[curr]);
      curr++;
   }
   t.stop();
   return t;
}

void DenseVector::clear_diff()
{
   _diff_index = 0;
}
