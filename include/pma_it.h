#pragma once

#include "stde.h"

/**
 * @brief Class to iterate over the segments of a PMA
 *
 * Dereferencing this iterator returns a pointer to last element of current segment.
 */
class pma_seg_it :
    public std::iterator<std::random_access_iterator_tag, void *, std::ptrdiff_t, void *, void *> {
 public:
  friend class pma_offset_it;
  typedef uint64_t size_type;

  static pma_seg_it begin(pma_struct *pma) {
    return pma_seg_it(pma);
  }

  static pma_seg_it end(pma_struct *pma) {
    return pma_seg_it(pma) += pma->nb_segments;
  }

  pma_seg_it(pma_struct *pma) : _pma(pma), _seg(0) {
  }

  pma_seg_it(const pma_seg_it &) = default;

  ~pma_seg_it() = default;

  pma_seg_it &operator=(const pma_seg_it &) = default;

  bool operator==(const pma_seg_it &other) const {
    return _seg == other._seg;
  }

  bool operator!=(const pma_seg_it &other) const {
    return _seg != other._seg;
  };

  bool operator<(const pma_seg_it &other) const {
    return _seg < other._seg;
  }

  bool operator>(const pma_seg_it &other) const {
    return _seg > other._seg;
  }

  bool operator<=(const pma_seg_it &other) const {
    return _seg <= other._seg;
  }

  bool operator>=(const pma_seg_it &other) const {
    return _seg >= other._seg;
  }

  pma_seg_it &operator--() {
    --_seg;
    return *this;
  }

  pma_seg_it &operator++() {
    ++_seg;
    return *this;
  }

  pma_seg_it operator++(int) {
    pma_seg_it old(*this);
    operator++();
    return old;
  }

  pma_seg_it &operator+=(size_type value) {
    _seg += value;
    return *this;
  }

  //pma_seg_it operator+(size_type) const; //optional
  //friend pma_seg_it operator+(size_type, const pma_seg_it&); //optional

  difference_type operator-(const pma_seg_it &other) const {
    return _seg - other._seg;
  }

  reference front() const {
    return SEGMENT_START(_pma, _seg);
  }

  reference back() const {
    return SEGMENT_LAST(_pma, _seg);
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
  pma_struct *_pma;
};
/**
 * @brief Iterates over elements inside a single segement
 *
 * Iterator is initalized with a segment number.
 * Iterartor advances element by element inside a segment @param _seg
 * Derrefencing this iterator returns the elements in @param _seg at position @param _offset.
 */
class pma_offset_it :
    public std::iterator<std::random_access_iterator_tag, void *, std::ptrdiff_t, void *, void *> {
 public:
  typedef uint64_t size_type;

  static pma_offset_it begin(pma_struct *pma, const pma_seg_it &it) {
    return pma_offset_it(pma, it._seg);
  }

  static pma_offset_it begin(pma_struct *pma, size_t seg) {
    return pma_offset_it(pma, seg);
  }

  static pma_offset_it end(pma_struct *pma, size_t seg) {
    return pma_offset_it(pma, seg) += pma->elts[seg];
  }

  static pma_offset_it end(pma_struct *pma, const pma_seg_it &it) {
    return pma_offset_it(pma, it._seg) += pma->elts[it._seg];
  }

  pma_offset_it() : _pma(nullptr), _seg(0), _offset(0) {
  }

  pma_offset_it(pma_struct *pma, size_t seg) : _pma(pma), _seg(seg), _offset(0) {
  }

  pma_offset_it(const pma_offset_it &) = default;

  ~pma_offset_it() = default;

  pma_offset_it &operator=(const pma_offset_it &) = default;

  bool operator==(const pma_offset_it &other) const {
    return _offset == other._offset/* && _seg == other._seg*/;
  }

  bool operator!=(const pma_offset_it &other) const {
    return _offset != other._offset/* || _seg != other._seg*/;
  };

  bool operator<(const pma_offset_it &other) const {
    return _offset < other._offset/* && _seg == other._seg*/;
  }

  bool operator>(const pma_offset_it &other) const {
    return _offset > other._offset/* && _seg == other._seg*/;
  }

  bool operator<=(const pma_offset_it &other) const {
    return _offset <= other._offset/* && _seg == other._seg*/;
  }

  bool operator>=(const pma_offset_it &other) const {
    return _offset >= other._offset/* && _seg == other._seg*/;
  }

  pma_offset_it &operator++() {
    ++_offset;
    return *this;
  }

  pma_offset_it operator++(int) {
    pma_offset_it old(*this);
    operator++();
    return old;
  }

  pma_offset_it &operator+=(size_type value) {
    _offset += value;
    return *this;
  }

  //pma_offset_it operator+(size_type) const; //optional
  //friend pma_offset_it operator+(size_type, const pma_offset_it&); //optional

  difference_type operator-(const pma_offset_it &other) const {
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
  pma_struct *_pma;
};
