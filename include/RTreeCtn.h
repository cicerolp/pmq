//
// Created by cicerolp on 8/13/17.
//

#pragma once

#include "GeoCtnIntf.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

template<typename T, typename Balancing>
class RTreeCtn : public GeoCtnIntf<T> {
  // function that access a reference to the element in the container
  using scantype_function = typename GeoCtnIntf<T>::scantype_function;
  // function that counts elements in spatial areas
  using applytype_function = typename GeoCtnIntf<T>::applytype_function;

 public:
  RTreeCtn(int argc, char *argv[], int _refLevel = 8) : GeoCtnIntf<T>(_refLevel) {}
  virtual ~RTreeCtn() {
    _rtree->clear();
  };

  // build container
  virtual duration_t create(uint32_t size) override {
    Timer timer;
    timer.start();

    _size = size;

    // rtree
    _rtree = std::make_unique<rtree_t>(Balancing());

    timer.stop();
    return {duration_info("create", timer)};
  }

  // update container
  virtual duration_t insert(std::vector<T> batch) override {
    Timer timer;
    timer.start();

    for (const auto &elt: batch) {
      _rtree->insert(std::make_pair(point(elt.getLatitude(), elt.getLatitude()), elt));
    }

    timer.stop();
    return {duration_info("insert", timer)};
  }

  duration_t insert_rm(std::vector<T> batch, std::function<int(const void *)> is_removed) override {
    duration_t duration;

    Timer timer;
    timer.start();

    // insertion
    for (const auto &elt: batch) {
      _rtree->insert(std::make_pair(point(elt.getLatitude(), elt.getLatitude()), elt));
    }

    // insert end
    timer.stop();
    duration.emplace_back("insert", timer);

    if (_rtree->size() > _size) {

      // remove start
      timer.start();

      // temporary result
      std::vector<value> result;
      _rtree->query(bgi::satisfies([&is_removed](value const &elt) { return is_removed(&elt.second); }),
                    std::back_inserter(result));

      // remove end
      timer.stop();
      duration.emplace_back("remove", timer);
    }

    return duration;
  }

  // apply function for every el<valuetype>
  duration_t scan_at_region(const region_t &region, scantype_function __apply) override {
    Timer timer;
    timer.start();

    // longitude
    float xmin = mercator_util::tilex2lon(region.x0, region.z);
    float xmax = mercator_util::tilex2lon(region.x1 + 1, region.z);

    // latitude
    float ymin = mercator_util::tiley2lat(region.y1 + 1, region.z);
    float ymax = mercator_util::tiley2lat(region.y0, region.z);

    // convert from region_t to boost:box
    box query_box(point(ymin, xmin), point(ymax, xmax));

    // temporary result
    std::vector<value> result;
    _rtree->query(bgi::intersects(query_box), std::back_inserter(result));

    for (const auto &elt : result) {
      __apply(elt.second);
    }

    timer.stop();
    return {duration_info("scan_at_region", timer)};
  }

  duration_t apply_at_region(const region_t &region, applytype_function __apply) override {
    Timer timer;
    timer.start();

    // longitude
    float xmin = mercator_util::tilex2lon(region.x0, region.z);
    float xmax = mercator_util::tilex2lon(region.x1 + 1, region.z);  // JULIO : why + 1 ??

    // latitude
    float ymin = mercator_util::tiley2lat(region.y1 + 1, region.z);
    float ymax = mercator_util::tiley2lat(region.y0, region.z);

    // convert from region_t to boost:box
    box query_box(point(ymin, xmin), point(ymax, xmax));

    // temporary result
    RTreeCtnCounter<value> result;
    _rtree->query(bgi::intersects(query_box), std::back_inserter(result));

    __apply(spatial_t(region.x0 + (uint32_t) ((region.x1 - region.x0) / 2),
                      region.y0 + (uint32_t) ((region.y1 - region.y0) / 2),
                      0), result.size());

    timer.stop();
    return {duration_info("apply_at_region", timer)};
  }

  size_t size() const override {
    return _rtree->size();
  }

  virtual inline std::string name() const override {
    static auto name_str = "RTree";
    return name_str;
  }

 protected:
  template<typename _Tp>
  class RTreeCtnCounter {
   public:
    typedef _Tp value_type;

    uint32_t
    size() const _GLIBCXX_NOEXCEPT {
      return _count;
    }

    void
    clear() _GLIBCXX_NOEXCEPT {
      _count = 0;
      return;
    }

    void
    push_back(const value_type &__x) {
      ++_count;
      return;
    }

   private:
    uint32_t _count = 0;
  };

  //typedef bg::model::point<float, 2, bg::cs::geographic<bg::degree>> point;
  typedef bg::model::point<float, 2, bg::cs::cartesian> point;
  typedef bg::model::box<point> box;
  typedef std::pair<point, T> value;
  typedef bgi::rtree<value, Balancing> rtree_t;

  std::unique_ptr<rtree_t> _rtree{nullptr};
  uint32_t _size;
};

template<typename T, typename Balancing>
class RTreeBulkCtn : public RTreeCtn<T, Balancing> {
 public:
  RTreeBulkCtn(int argc, char **argv, int _refLevel = 8) : RTreeCtn<T, Balancing>(argc, argv, _refLevel) {}
  virtual ~RTreeBulkCtn() = default;

  // build container
  duration_t create(uint32_t size) override {
    Timer timer;
    timer.start();

    this->_size = size;

    timer.stop();
    return {duration_info("create", timer)};
  }

  // update container
  duration_t insert(std::vector<T> batch) override {
    Timer timer;
    timer.start();

    if (this->_rtree == nullptr) {
      // bulk loading
      std::vector<value> data;

      std::transform(
          batch.begin(), batch.end(), std::back_inserter(data),
          [&](const auto &elt) {
            return std::make_pair(point(elt.getLatitude(), elt.getLatitude()), elt);
          }
      );

      this->_rtree = std::make_unique<rtree_t>(data);

    } else {
      for (const auto &elt: batch) {
        this->_rtree->insert(std::make_pair(point(elt.getLatitude(), elt.getLatitude()), elt));
      }
    }

    timer.stop();
    return {duration_info("insert", timer)};
  }

  inline std::string name() const override {
    static auto name_str = "RTreeBulk";
    return name_str;
  }

 protected:
  typedef typename RTreeCtn<T, Balancing>::point point;
  typedef typename RTreeCtn<T, Balancing>::value value;
  typedef typename RTreeCtn<T, Balancing>::rtree_t rtree_t;
};
