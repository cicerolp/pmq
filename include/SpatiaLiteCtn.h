#pragma once
#include "GeoCtnIntf.h"

class SpatiaLiteCtn : public GeoCtnIntf {
public:
   SpatiaLiteCtn();
   ~SpatiaLiteCtn();

   // build container
   duration_t create(uint32_t size) override;

   // update container
   duration_t insert(std::vector<elttype> batch) override;

   // apply function for every el<valuetype>
   duration_t scan_at_region(const region_t& region, scantype_function __apply) override;

   // apply function for every spatial area/region
   duration_t apply_at_tile(const region_t& region, applytype_function __apply) override;
   duration_t apply_at_region(const region_t& region, applytype_function __apply) override;

   inline virtual std::string name() const;

private:
   static void notice(const char* fmt, ...);
   static void log_and_exit(const char* fmt, ...);

private:
   bool _init{false};
   void* _cache{nullptr};
   sqlite3* _handle{nullptr};
};

std::string SpatiaLiteCtn::name() const {
   static auto name_str = "SQLite";
   return name_str;
}