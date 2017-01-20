#pragma once

#include "stde.h"

class pma_seg_it :
   public std::iterator<std::random_access_iterator_tag, void*, std::ptrdiff_t, void*, void*> {
public:
   friend class pma_offset_it;
   typedef uint64_t size_type;

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
      ++_seg;
      while (_seg < _pma->nb_segments && _pma->elts[_seg] == 0) ++_seg;
      return *this;
   }

   pma_seg_it operator++(int) {
      pma_seg_it old(*this);
      operator++();
      return old;
   }

   pma_seg_it& operator+=(size_type value) {
      _seg += value;
      while (_seg < _pma->nb_segments && _pma->elts[_seg] == 0) ++_seg;    
      return *this;
   }

   //pma_seg_it operator+(size_type) const; //optional
   //friend pma_seg_it operator+(size_type, const pma_seg_it&); //optional

   difference_type operator-(const pma_seg_it& other) const {
      //return _seg - other._seg;
      difference_type diff = 0;
      for (size_type it = other._seg; it < _seg; ++it) {
         if (_pma->elts[it] != 0) diff++;
      }
      return diff;
   }

   reference operator*() const {
      return SEGMENT_LAST(_pma, _seg);
   }

   pointer operator->() const {
      return SEGMENT_LAST(_pma, _seg);
   }

   size_type size() const {
      return _pma->elts[_seg];
   }

protected:
   size_type _seg;
   pma_struct* _pma;
};

class pma_offset_it :
   public std::iterator<std::random_access_iterator_tag, void*, std::ptrdiff_t, void*, void*> {
public:
   typedef uint64_t size_type;

   static pma_offset_it begin(pma_struct* pma, const pma_seg_it& it) {
      return pma_offset_it(pma, it._seg);
   }

   static pma_offset_it begin(pma_struct* pma, size_t seg) {
      return pma_offset_it(pma, seg);
   }

   static pma_offset_it end(pma_struct* pma, size_t seg) {
      return pma_offset_it(pma, seg) += pma->elts[seg];
   }

   static pma_offset_it end(pma_struct* pma, const pma_seg_it& it) {
      return pma_offset_it(pma, it._seg) += pma->elts[it._seg];
   }

   pma_offset_it() : _pma(nullptr), _seg(0), _offset(0) {
   }

   pma_offset_it(pma_struct* pma, size_t seg) : _pma(pma), _seg(seg), _offset(0) {
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
      ++_offset;
      return *this;
   }

   pma_offset_it operator++(int) {
      pma_offset_it old(*this);
      operator++();
      return old;
   }

   pma_offset_it& operator+=(size_type value) {
      _offset += value;
      return *this;
   }

   //pma_offset_it operator+(size_type) const; //optional
   //friend pma_offset_it operator+(size_type, const pma_offset_it&); //optional

   difference_type operator-(const pma_offset_it& other) const {
      return _offset - other._offset;
   }

   reference operator*() const {
      return SEGMENT_ELT(_pma, _seg, _offset);
   }

   pointer operator->() const {
      return SEGMENT_ELT(_pma, _seg, _offset);
   }

protected:
   size_type _seg, _offset;
   pma_struct* _pma;
};
