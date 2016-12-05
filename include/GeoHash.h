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
	uint32_t seg_size;
	float tau_0, tau_h, rho_0, rho_h;

	pma_struct* _pma{ nullptr };
};

std::string GeoHash::name() const {
	static auto name_str = "GeoHash";
	return name_str;
}