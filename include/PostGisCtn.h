#pragma once
#include "GeoCtnIntf.h"

class PostGisCtn : public GeoCtnIntf {
public:
   PostGisCtn(int argc, char* argv[]);

   ~PostGisCtn();

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
   bool _init{false};
#ifdef __GNUC__
   PGconn* _conn;
#endif // __GNUC__
};

std::string PostGisCtn::name() const {
   static auto name_str = "PostgreSQL";
   return name_str;
}
