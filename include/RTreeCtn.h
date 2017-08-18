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
      _rtree->insert(std::make_pair(point(elt.value.latitude, elt.value.longitude), elt.value));
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
    std::vector<value> result;
    _rtree->query(bgi::intersects(query_box), std::back_inserter(result));

    for (const auto &elt : result) {
      __apply(elt.second);
    }

    timer.stop();
    return {duration_info("scan_at_region", timer)};
  }

  // apply function for every spatial area/region
  duration_t apply_at_tile(const region_t &region, applytype_function __apply) override {
    Timer timer;
    timer.start();

    uint32_t curr_z = std::min((uint32_t)8, 25 - region.z);
    uint32_t n = (uint64_t)1 << curr_z;

    uint32_t x_min = region.x0 * n;
    uint32_t x_max = (region.x1 + 1) * n;

    uint32_t y_min = region.y0 * n;
    uint32_t y_max = (region.y1 + 1) * n;

    curr_z += region.z;

    for (uint32_t x = x_min; x < x_max; ++x) {
      for (uint32_t y = y_min; y < y_max; ++y) {


        // longitude
        float xmin = mercator_util::tilex2lon(x, curr_z);
        float xmax = mercator_util::tilex2lon(x + 1, curr_z);

        // latitude
        float ymin = mercator_util::tiley2lat(y + 1, curr_z);
        float ymax = mercator_util::tiley2lat(y, curr_z);

        // convert from region_t to boost:box
        box query_box(point(ymin, xmin), point(ymax, xmax));

        std::stringstream stream;

        std::string xmin = std::to_string(mercator_util::tilex2lon(x, curr_z));
        std::string xmax = std::to_string(mercator_util::tilex2lon(x + 1, curr_z));

        std::string ymin = std::to_string(mercator_util::tiley2lat(y + 1, curr_z));
        std::string ymax = std::to_string(mercator_util::tiley2lat(y, curr_z));

        stream << "SELECT count(*) FROM db ";
        stream << "WHERE ST_WITHIN(key, BuildMbr(";
        stream << xmin << "," << ymin << ",";
        stream << xmax << "," << ymax << ")) AND ROWID IN (";
        stream << "SELECT pkid FROM idx_db_key WHERE ";
        stream << "xmin >= " << xmin << " AND ";
        stream << "xmax <= " << xmax << " AND ";
        stream << "ymin >= " << ymin << " AND ";
        stream << "ymax <= " << ymax << ")";

        sqlite3_stmt* stmt;

        // preparing to populate the table
        ret = sqlite3_prepare_v2(_handle, stream.str().c_str(), stream.str().size(), &stmt, NULL);
        if (ret != SQLITE_OK) {
          // an error occurred
          printf("INSERT SQL error: %s\n", sqlite3_errmsg(_handle));

          timer.stop();
          return {duration_info("total", timer)};
        }

        if (sqlite3_step(stmt) == SQLITE_ROW) {
          int count = sqlite3_column_int(stmt, 0);
          if (count > 0) __apply(spatial_t(x, y, curr_z), count);
        }

        sqlite3_finalize(stmt);
      }
    }

    timer.stop();
    return {duration_info("apply_at_tile", timer)};
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
    std::vector<value> result;
    _rtree->query(bgi::intersects(query_box), std::back_inserter(result));

    __apply(spatial_t(region.x0 + (uint32_t) ((region.x1 - region.x0) / 2),
                      region.y0 + (uint32_t) ((region.y1 - region.y0) / 2),
                      0), result.size());

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
  typedef bg::model::point<float, 2, bg::cs::geographic<bg::degree>> point;
  typedef bg::model::box<point> box;
  typedef std::pair<point, valuetype> value;
  typedef bgi::rtree<value, Balancing> rtree_t;

  std::unique_ptr<rtree_t> _rtree;
};
