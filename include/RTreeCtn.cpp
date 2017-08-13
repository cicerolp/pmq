//
// Created by cicerolp on 8/13/17.
//

#include "RTreeCtn.h"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>

#include <boost/geometry/index/rtree.hpp>

// to store queries results
#include <vector>

// just for output
#include <iostream>
#include <boost/foreach.hpp>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::point<float, 2, bg::cs::cartesian> point;
typedef bg::model::box<point> box;
typedef std::pair<box, unsigned> value;

RTreeCtn::RTreeCtn(int argc, char *argv[]) : GeoCtnIntf() {

}
duration_t RTreeCtn::create(uint32_t size) {
  // create the rtree using default constructor
  bgi::rtree<value, bgi::quadratic<16>> rtree;

  // create some values
  for (unsigned i = 0; i < 10; ++i) {
    // create a box
    box b(point(i + 0.0f, i + 0.0f), point(i + 0.5f, i + 0.5f));
    // insert new value
    rtree.insert(std::make_pair(b, i));
  }


  // find values intersecting some area defined by a box
  box query_box(point(0, 0), point(5, 5));
  std::vector<value> result_s;
  rtree.query(bgi::intersects(query_box), std::back_inserter(result_s));



  // find 5 nearest values to a point
  std::vector<value> result_n;
  rtree.query(bgi::nearest(point(0, 0), 5), std::back_inserter(result_n));



  // display results
  std::cout << "spatial query box:" << std::endl;
  std::cout << bg::wkt<box>(query_box) << std::endl;
  std::cout << "spatial query result:" << std::endl;
  BOOST_FOREACH(value const &v, result_s)std::cout << bg::wkt<box>(v.first) << " - " << v.second << std::endl;

  std::cout << "knn query point:" << std::endl;
  std::cout << bg::wkt<point>(point(0, 0)) << std::endl;
  std::cout << "knn query result:" << std::endl;
  BOOST_FOREACH(value const &v, result_n)std::cout << bg::wkt<box>(v.first) << " - " << v.second << std::endl;

  return duration_t();
}

duration_t RTreeCtn::insert(std::vector<elttype> batch) {
  return duration_t();
}

duration_t RTreeCtn::scan_at_region(const region_t &region, scantype_function __apply) {
  return duration_t();
}

duration_t RTreeCtn::apply_at_tile(const region_t &region, applytype_function __apply) {
  return duration_t();
}

duration_t RTreeCtn::apply_at_region(const region_t &region, applytype_function __apply) {
  return duration_t();
}

duration_t RTreeCtn::topk_search(const region_t &region, topk_t &topk, scantype_function __apply) {
  return GeoCtnIntf::topk_search(region, topk, __apply);
}
