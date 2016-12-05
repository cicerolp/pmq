#pragma once

#include "GeoCtnIntf.h"

class GeoHash : public GeoCtnIntf {
public:
	GeoHash(int argc, char* argv[]);
	virtual ~GeoHash();

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

protected:
   bool naive_search_pma(const spatial_t& el) const;
   void get_mcode_range(const spatial_t& el, uint64_t& min, uint64_t& max, uint32_t morton_size) const;

	uint32_t seg_size;
	float tau_0, tau_h, rho_0, rho_h;

	pma_struct* _pma{ nullptr };
};

std::string GeoHash::name() const {
	static auto name_str = "GeoHash";
	return name_str;
}

inline void GeoHash::get_mcode_range(const spatial_t& el, uint64_t& min, uint64_t& max, uint32_t morton_size) const {
   uint32_t diffDepth = morton_size - el.z;
   min = el.code << 2 * (diffDepth);
   max = min | ((uint64_t)~0 >> (64 - 2 * diffDepth));
}
