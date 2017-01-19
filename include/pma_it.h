#pragma once

#include "stde.h"
/*
class pma_elt_it {
public:
   typedef std::ptrdiff_t difference_type;
   typedef void* value_type;
   typedef void* reference;
   typedef void* pointer;
   typedef uint64_t size_type;
   typedef std::forward_iterator_tag iterator_category;

   static pma_elt_it begin(pma_struct* pma, size_t seg) {
      return pma_elt_it(pma, seg);
   }

   static pma_elt_it end(pma_struct* pma, size_t seg) {
      return pma_elt_it(pma, seg) += pma->elts[seg];

   }

   pma_elt_it(pma_struct* pma, size_t seg) : _pma(pma), _seg(seg) {
      _offset = 0;
   }

   pma_elt_it(const pma_elt_it&) = default;

   ~pma_elt_it() = default;

   pma_elt_it& operator=(const pma_elt_it&) = default;

   bool operator==(const pma_elt_it& other) const {
      return _seg == other._seg && _offset == other._offset;
   }

   bool operator!=(const pma_elt_it& other) const {
      return _seg != other._seg || _offset != other._offset;
   };

   bool operator<(const pma_elt_it& other) const {
      return _seg == other._seg && _offset < other._offset;
   }

   bool operator>(const pma_elt_it& other) const {
      return _seg == other._seg && _offset > other._offset;
   }

   bool operator<=(const pma_elt_it& other) const {
      return _seg == other._seg && _offset <= other._offset;
   }

   bool operator>=(const pma_elt_it& other) const {
      return _seg == other._seg && _offset >= other._offset;
   }

   pma_elt_it& operator++() {
      if (_offset <= _pma->elts[_seg]) {
         ++_offset;
      }
      return *this;
   }

   pma_elt_it operator++(int) {
      pma_elt_it old(*this);
      operator++();
      return old;
   }

   pma_elt_it& operator+=(size_type value) {
      if (_offset < _pma->elts[_seg]) {
         _offset = std::min((size_type)_pma->elts[_seg], _offset + value);
      }
      return *this;
   }

   //pma_offset_it operator+(size_type) const; //optional
   //friend pma_offset_it operator+(size_type, const pma_offset_it&); //optional

   difference_type operator-(const pma_elt_it& other) const {
      return (difference_type)std::abs((int64_t)_offset - (int64_t)other._offset);
   }

   reference operator*() const {
      if (_offset < _pma->elts[_seg]) {
         return SEGMENT_ELT(_pma, _seg, _offset);
      }
      else {
         return nullptr;
      }
   }

   pointer operator->() const {
      if (_offset < _pma->elts[_seg]) {
         return SEGMENT_ELT(_pma, _seg, _offset);
      }
      else {
         return nullptr;
      }
   }

protected:
   size_type _seg;
   pma_struct* _pma;
   size_type _offset;
};
*/

class pma_offset_it {
public:
   typedef std::ptrdiff_t difference_type;
   typedef void* value_type;
   typedef void* reference;
   typedef void* pointer;
   typedef uint64_t size_type;
   typedef std::forward_iterator_tag iterator_category;

   static pma_offset_it begin(pma_struct* pma, size_t seg) {
      return pma_offset_it(pma, seg);
   }

   static pma_offset_it end(pma_struct* pma, size_t seg) {
      return pma_offset_it(pma, seg) += pma->elts[seg];

   }

   pma_offset_it(pma_struct* pma, size_t seg) : _pma(pma), _seg(seg) {
      _offset = 0;
   }

   pma_offset_it(const pma_offset_it&) = default;

   ~pma_offset_it() = default;

   pma_offset_it& operator=(const pma_offset_it&) = default;

   bool operator==(const pma_offset_it& other) const {
      return _seg == other._seg && _offset == other._offset;
   }

   bool operator!=(const pma_offset_it& other) const {
      return _seg != other._seg || _offset != other._offset;
   };

   bool operator<(const pma_offset_it& other) const {
      return _seg == other._seg && _offset < other._offset;
   }

   bool operator>(const pma_offset_it& other) const {
      return _seg == other._seg && _offset > other._offset;
   }

   bool operator<=(const pma_offset_it& other) const {
      return _seg == other._seg && _offset <= other._offset;
   }

   bool operator>=(const pma_offset_it& other) const {
      return _seg == other._seg && _offset >= other._offset;
   }

   pma_offset_it& operator++() {
      if (_offset <= _pma->elts[_seg]) {
         ++_offset;
      }
      return *this;
   }

   pma_offset_it operator++(int) {
      pma_offset_it old(*this);
      operator++();
      return old;
   }

   pma_offset_it& operator+=(size_type value) {
      if (_offset < _pma->elts[_seg]) {
         _offset = std::min((size_type)_pma->elts[_seg], _offset + value);
      }
      return *this;
   }

   //pma_offset_it operator+(size_type) const; //optional
   //friend pma_offset_it operator+(size_type, const pma_offset_it&); //optional

   difference_type operator-(const pma_offset_it& other) const {
      return (difference_type)std::abs((int64_t)_offset - (int64_t)other._offset);
   }

   reference operator*() const {
      if (_offset < _pma->elts[_seg]) {
         return SEGMENT_ELT(_pma, _seg, _offset);
      } else {
         return nullptr;
      }
   }

   pointer operator->() const {
      if (_offset < _pma->elts[_seg]) {
         return SEGMENT_ELT(_pma, _seg, _offset);
      } else {
         return nullptr;
      }
   }

protected:
   size_type _seg;
   pma_struct* _pma;
   size_type _offset;
};

class pma_seg_it {
public:
   typedef std::ptrdiff_t difference_type;
   typedef void* value_type;
   typedef void* reference;
   typedef void* pointer;
   typedef uint64_t size_type;
   typedef std::forward_iterator_tag iterator_category;

   static pma_seg_it begin(pma_struct* pma) {
      return pma_seg_it(pma);
   }

   static pma_seg_it end(pma_struct* pma) {
      return pma_seg_it(pma) += pma->nb_segments;
   }

   pma_seg_it(pma_struct* pma) : _pma(pma) {
      _seg = 0;
      while (_seg < _pma->nb_segments && _pma->elts[_seg] == 0) ++_seg;
   }

   pma_seg_it(const pma_seg_it&) = default;

   ~pma_seg_it() = default;

   pma_seg_it& operator=(const pma_seg_it&) = default;

   bool operator==(const pma_seg_it& other) const {
      return _seg == other._seg;
   }

   bool operator!=(const pma_seg_it& other) const {
      return _seg != other._seg;
   };

   bool operator<(const pma_seg_it& other) const {
      return _seg < other._seg;
   }

   bool operator>(const pma_seg_it& other) const {
      return _seg > other._seg;
   }

   bool operator<=(const pma_seg_it& other) const {
      return _seg <= other._seg;
   }

   bool operator>=(const pma_seg_it& other) const {
      return _seg >= other._seg;
   }

   pma_seg_it& operator++() {
      if (_seg < _pma->nb_segments) {
         ++_seg;
         while (_seg < _pma->nb_segments && _pma->elts[_seg] == 0) ++_seg;
      }
      return *this;
   }

   pma_seg_it operator++(int) {
      pma_seg_it old(*this);
      operator++();
      return old;
   }

   pma_seg_it& operator+=(size_type value) {
      if (_seg < _pma->nb_segments) {
         _seg = std::min((size_type)_pma->nb_segments, _seg + value);
         while (_seg < _pma->nb_segments && _pma->elts[_seg] == 0) ++_seg;
      }
      return *this;
   }

   //pma_seg_it operator+(size_type) const; //optional
   //friend pma_seg_it operator+(size_type, const pma_seg_it&); //optional

   difference_type operator-(const pma_seg_it& other) const {
      difference_type diff = 0;
      size_t begin, end;

      if (_seg > other._seg) {
         begin = other._seg;
         end = _seg;
      } else {
         begin = _seg;
         end = other._seg;
      }

      for (size_t it = begin; it < end; ++it) {
         if (_pma->elts[it] != 0) diff++;
      }

      return diff;
   }

   reference operator*() const {
      if (_seg < _pma->nb_segments) {
         return SEGMENT_LAST(_pma, _seg);
      } else {
         return nullptr;
      }
   }

   pointer operator->() const {
      if (_seg < _pma->nb_segments) {
         return SEGMENT_LAST(_pma, _seg);
      } else {
         return nullptr;
      }
   }

protected:
   size_type _seg;
   pma_struct* _pma;
};
