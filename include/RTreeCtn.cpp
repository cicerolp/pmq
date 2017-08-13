//
// Created by cicerolp on 8/13/17.
//

#include "RTreeCtn.h"

// just for output
#include <iostream>
#include <boost/foreach.hpp>

RTreeCtn::RTreeCtn(int argc, char *argv[]) : GeoCtnIntf() {

}

duration_t RTreeCtn::create(uint32_t size) {
  std::vector<point> points;

  //_ind = std::make_unique<indexable_getter>(points);

  _rtree = std::make_unique<rtree_t>(bgi::dynamic_linear(16));
  //_rtree = std::make_unique<rtree_t>(bgi::dynamic_linear(16), *_ind);
  //_rtree = std::make_unique<rtree_t>(bgi::dynamic_quadratic(16));
  //_rtree = std::make_unique<rtree_t>(bgi::dynamic_rstar(16));


  // create some values
  for (float f = 0; f < 10; f += 1) {
    // insert new value
    _rtree->insert(point(f, f));
  }

  // find values intersecting some area defined by a box
  box query_box(point(0, 0), point(5, 5));
  std::vector<value> result_s;
  _rtree->query(bgi::intersects(query_box), std::back_inserter(result_s));

  // find 5 nearest values to a point
  //std::vector<value> result_n;
  //_rtree->query(bgi::nearest(point(0, 0), 5), std::back_inserter(result_n));

  // display results
  std::cout << "spatial query box:" << std::endl;
  std::cout << bg::wkt<box>(query_box) << std::endl;
  std::cout << "spatial query result:" << std::endl;
  BOOST_FOREACH(value i, result_s)std::cout << bg::wkt<point>(i) << std::endl;

  //std::cout << "knn query point:" << std::endl;
  //std::cout << bg::wkt<point>(point(0, 0)) << std::endl;
  //std::cout << "knn query result:" << std::endl;
  //BOOST_FOREACH(value i, result_n)
  //std::cout << bg::wkt<box>(boxes[i]) << std::endl;

  return duration_t();
}

duration_t RTreeCtn::insert(std::vector<elttype> batch) {

  /*
   // create some values
for (double f = 0; f < 10; f += 1) {
  // insert new value
  _rtree->insert(point(f, f));
}

// query point
point pt(5.1, 5.1);

// iterate over nearest values
for (rtree_t::const_query_iterator it = _rtree->qbegin(bgi::nearest(pt, 100)); it != _rtree->qend(); ++it) {
  double d = bg::distance(pt, *it);

  std::cout << bg::wkt(*it) << ", distance= " << d << std::endl;

  // break if the distance is too big
  if (d > 2) {
    std::cout << "break!" << std::endl;
    break;
  }
}
   */

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
