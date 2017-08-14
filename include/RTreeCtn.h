//
// Created by cicerolp on 8/13/17.
//

#pragma once

#include "GeoCtnIntf.h"

// just for output
#include <iostream>
#include <boost/foreach.hpp>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

template<typename Balancing>
class RTreeCtn : public GeoCtnIntf {
 public:
  RTreeCtn(int argc, char *argv[]) : GeoCtnIntf() {

  }
  virtual ~RTreeCtn() = default;

  // build container
  duration_t create(uint32_t size) override {
    Timer timer;
    timer.start();

    // container
    _container.reserve(size);

    // rtree
    _rtree = std::make_unique<rtree_t>(Balancing());

    timer.stop();
    return {duration_info("create", timer)};
  }

  // update container
  duration_t insert(std::vector<elttype> batch) override {
    Timer timer;
    timer.start();

    for (const auto &elt: batch) {
      _container.emplace_back(elt.value);
      _rtree->insert(std::make_pair(point(elt.value.latitude, elt.value.longitude), _container.size() - 1));
    }

    timer.stop();
    return {duration_info("insert", timer)};
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
    std::vector<value> result_s;
    _rtree->query(bgi::intersects(query_box), std::back_inserter(result_s));

    for (const auto &elt : result_s) {
      __apply(_container[elt.second]);
    }

    return {duration_info("scan_at_region", timer)};
  }

  // apply function for every spatial area/region
  duration_t apply_at_tile(const region_t &region, applytype_function __apply) override {
    Timer timer;
    timer.start();

    timer.stop();
    return {duration_info("nullptr", timer)};
  }
  duration_t apply_at_region(const region_t &region, applytype_function __apply) override {
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
    std::vector<value> result_s;
    _rtree->query(bgi::intersects(query_box), std::back_inserter(result_s));

    __apply(spatial_t(region.x0 + (uint32_t) ((region.x1 - region.x0) / 2),
                      region.y0 + (uint32_t) ((region.y1 - region.y0) / 2),
                      0), result_s.size());

    timer.stop();
    return {duration_info("scan_at_region", timer)};
  }

  duration_t topk_search(const region_t &region, topk_t &topk, scantype_function __apply) override {
    Timer timer;
    timer.start();
    return {duration_info("nullptr", timer)};
  }

  inline std::string name() const override {
    static auto name_str = "RTree";
    return name_str;
  }

 protected:

  typedef std::vector<valuetype> container_t;

  typedef bg::model::point<float, 2, bg::cs::geographic<bg::degree>> point;
  typedef bg::model::box<point> box;

  typedef std::pair<point, unsigned> value;

  typedef bgi::rtree<value, Balancing> rtree_t;

  /*class indexable_t {
    typedef typename container_t::size_type size_t;
    typedef typename container_t::const_reference cref;
    container_t const &container;

   public:
    typedef point result_type;
    explicit indexable_t(container_t const &c) : container(c) {}
    result_type operator()(size_t i) const {
      // conversion from twittervis to boost:geographic
      return point(container[i].value.latitude, container[i].value.longitude);
    }
  };*/


  container_t _container;
  std::unique_ptr<rtree_t> _rtree;
};
