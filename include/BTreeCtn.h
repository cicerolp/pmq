//
// Created by cicerolp on 8/20/17.
//

#pragma once

#include "GeoCtnIntf.h"

template<typename T>
class BTreeCtn : public GeoCtnIntf<T> {
 public:
  // function that access a reference to the element in the container
  using scantype_function = typename GeoCtnIntf<T>::scantype_function;
  // function that counts elements in spatial areas
  using applytype_function = typename GeoCtnIntf<T>::applytype_function;

  BTreeCtn(int argc, char *argv[], int _refLevel = 8) : GeoCtnIntf<T>(_refLevel) {}
  virtual ~BTreeCtn() {
    _btree->clear();
  }

  duration_t create(uint32_t size) override {
    Timer timer;
    timer.start();

    _size = size;

    _btree = std::make_unique<stx::btree_multimap<key, T>>();

    timer.stop();
    return {duration_info("create", timer)};
  }

  duration_t insert(std::vector<T> batch) override {
    Timer timer;
    timer.start();

    for (const auto &elt: batch) {
      uint32_t y = mercator_util::lat2tiley(elt.getLatitude(), 25);
      uint32_t x = mercator_util::lon2tilex(elt.getLatitude(), 25);
      _btree->insert(mortonEncode_RAM(x, y), elt);
    }

    timer.stop();
    return {duration_info("insert", timer)};
  }

  duration_t insert_rm(std::vector<T> batch, std::function<int(const void *)> is_removed) override {
    duration_t duration;

    Timer timer;
    timer.start();

    for (const auto &elt: batch) {
      uint32_t y = mercator_util::lat2tiley(elt.getLatitude(), 25);
      uint32_t x = mercator_util::lon2tilex(elt.getLongitude(), 25);
      _btree->insert(mortonEncode_RAM(x, y), elt);
    }

    // insert end
    timer.stop();
    duration.emplace_back("insert", timer);

    if (_btree->size() > _size) {
      // remove start
      timer.start();

      //using a temporary array
      std::vector<uint64_t> rm;
      for (auto it = _btree->begin(); it != _btree->end(); it++) {
        if (is_removed(&(it->second))) {
          rm.push_back(it->first);
        }
      }

      for (auto &e : rm) {
        auto it = _btree->find(e);
        //prevents deleting wrong element (same key with different timestamp)
        while (!is_removed(&(it->second))) it++;

        _btree->erase(it);

      }

      // remove end
      timer.stop();
      duration.emplace_back("remove", timer);
    }

    return duration;
  }

  duration_t scan_at_region(const region_t &region, scantype_function __apply) override {
    duration_t duration;
    Timer timer;

    timer.start();

    uint32_t refinements = scan_btree_at_region(get_parent_quadrant(region), region, __apply);

    timer.stop();
    duration.emplace_back("scan_at_region", timer);

    duration.emplace_back("scan_at_region_refinements", refinements);

    return duration;
  }

  duration_t apply_at_region(const region_t &region, applytype_function __apply) override {
    duration_t duration;
    Timer timer;

    timer.start();

    uint32_t refinements = apply_btree_at_region(get_parent_quadrant(region), region, __apply);

    timer.stop();
    duration.emplace_back("apply_at_region", timer);

    duration.emplace_back("apply_at_region_refinements", refinements);

    return duration;
  }

  size_t size() const override {
    return _btree->size();
  }

  std::string name() const override {
    static auto name_str = "BTree";
    return name_str;
  }

  inline code_t get_parent_quadrant(const region_t &region) const {
    uint64_t mask = region.code0 ^region.code1;

    mask |= mask >> 32;
    mask |= mask >> 16;
    mask |= mask >> 8;
    mask |= mask >> 4;
    mask |= mask >> 2;
    mask |= mask >> 1;

    mask = (~mask);

    uint64_t prefix = mask & region.code1;

    uint32_t depth_diff = 0;
    while ((mask & 3) != 3) {
      depth_diff++;
      mask = mask >> 2;
    }

    prefix = prefix >> (depth_diff * 2);

    return code_t(prefix, region.z - depth_diff);
  }

 protected:
  uint32_t scan_btree_at_region(const code_t &el, const region_t &region, scantype_function __apply) {
    if (el.z > region.z) return 0;

    region_t::overlap overlap = region.test(el);

    if (overlap == region_t::full) {
      uint32_t count = 0;

      auto lower_it = _btree->lower_bound(el.min_code);
      auto upper_it = _btree->lower_bound(el.max_code + 1);

      for (auto it = lower_it; it != upper_it; ++it) {
        __apply((*it).second);
      }

      if (lower_it != upper_it) {
        return 1;
      } else {
        return 0;
      }
    } else if (overlap == region_t::partial) {
      if (el.z < this->getRefLevel()) {
        uint32_t refinements = 0;

        // break morton code into four
        uint64_t code = el.code << 2;

        refinements += scan_btree_at_region(code_t(code | 0, (uint32_t) (el.z + 1)), region, __apply);
        refinements += scan_btree_at_region(code_t(code | 1, (uint32_t) (el.z + 1)), region, __apply);
        refinements += scan_btree_at_region(code_t(code | 2, (uint32_t) (el.z + 1)), region, __apply);
        refinements += scan_btree_at_region(code_t(code | 3, (uint32_t) (el.z + 1)), region, __apply);

        return refinements;

      } else {
        auto lower_it = _btree->lower_bound(el.min_code);
        auto upper_it = _btree->lower_bound(el.max_code + 1);

        for (auto it = lower_it; it != upper_it; ++it) {
          uint64_t code = (*it).first;

          uint32_t x, y;
          mortonDecode_RAM(code, x, y);

          if (region.x0 <= x && region.x1 >= x && region.y0 <= y && region.y1 >= y) {
            __apply((*it).second);
          }
        }

        if (lower_it != upper_it) {
          return 1;
        } else {
          return 0;
        }
      }

    } else {
      return 0;
    }
  }

  uint32_t apply_btree_at_region(const code_t &el, const region_t &region, applytype_function __apply) {
    if (el.z > region.z) return 0;

    region_t::overlap overlap = region.test(el);

    if (overlap == region_t::full) {
      uint32_t count = 0;

      auto lower_it = _btree->lower_bound(el.min_code);
      auto upper_it = _btree->lower_bound(el.max_code + 1);

      while (lower_it++ != upper_it) {
        ++count;
      }

      if (count) {
        __apply(el, count);
        return 1;
      } else {
        return 0;
      }
    } else if (overlap == region_t::partial) {
      if (el.z < this->getRefLevel()) {
        uint32_t refinements = 0;

        // break morton code into four
        uint64_t code = el.code << 2;

        refinements += apply_btree_at_region(code_t(code | 0, (uint32_t) (el.z + 1)), region, __apply);
        refinements += apply_btree_at_region(code_t(code | 1, (uint32_t) (el.z + 1)), region, __apply);
        refinements += apply_btree_at_region(code_t(code | 2, (uint32_t) (el.z + 1)), region, __apply);
        refinements += apply_btree_at_region(code_t(code | 3, (uint32_t) (el.z + 1)), region, __apply);

        return refinements;

      } else {
        auto lower_it = _btree->lower_bound(el.min_code);
        auto upper_it = _btree->lower_bound(el.max_code + 1);

        uint32_t count = 0;

        while (lower_it != upper_it) {
          uint64_t code = (*lower_it).first;

          uint32_t x, y;
          mortonDecode_RAM(code, x, y);

          if (region.x0 <= x && region.x1 >= x && region.y0 <= y && region.y1 >= y) {
            ++count;
          }
          ++lower_it;
        }

        if (count) {
          __apply(el, count);
          return 1;
        } else {
          return 0;
        }
      }

    } else {
      return 0;
    }
  }

  typedef uint64_t key;

  uint32_t _size;
  std::unique_ptr<stx::btree_multimap<key, T>> _btree;
};
