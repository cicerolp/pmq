//
// Created by cicerolp on 8/13/17.
//

#pragma once

#include "GeoCtnIntf.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

class RTreeCtn : public GeoCtnIntf {
 public:
  RTreeCtn(int argc, char *argv[]);
  virtual ~RTreeCtn() = default;

  // build container
  duration_t create(uint32_t size) override;

  // update container
  duration_t insert(std::vector<elttype> batch) override;

  // apply function for every el<valuetype>
  duration_t scan_at_region(const region_t &region, scantype_function __apply) override;

  // apply function for every spatial area/region
  duration_t apply_at_tile(const region_t &region, applytype_function __apply) override;
  duration_t apply_at_region(const region_t &region, applytype_function __apply) override;

  duration_t topk_search(const region_t &region, topk_t &topk, scantype_function __apply) override;

  inline std::string name() const override {
    static auto name_str = "RTree";
    return name_str;
  }

 protected:

  template<typename Container>
  class indexable_t {
    typedef typename Container::size_type size_t;
    typedef typename Container::const_reference cref;
    Container const &container;

   public:
    typedef cref result_type;
    explicit indexable_t(Container const &c) : container(c) {}
    result_type operator()(size_t i) const { return container[i]; }
  };

  typedef bg::model::point<float, 2, bg::cs::geographic<bg::degree>> point;
  typedef bg::model::box<point> box;
  typedef point value;
  typedef indexable_t<std::vector<point>> indexable_getter;
  //typedef bgi::rtree<value, bgi::dynamic_linear, indexable_getter> rtree_t;
  typedef bgi::rtree<value, bgi::dynamic_linear> rtree_t;

  std::unique_ptr<rtree_t> _rtree;
  std::unique_ptr<indexable_getter> _ind;
};
