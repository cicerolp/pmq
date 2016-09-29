#pragma once
#include "stde.h"

#include "Query.h"
#include "Singleton.h"

#include "SpatialElement.h"

#include "DMPLoader/dmploader.hpp"

#include "PMQInterface.h"

class Runner : public Singleton<Runner> {
	friend class Singleton<Runner>;
public:
   /**
     * @brief query
     * @param query  "/rest/query/region/1/0/0/1/1"
     *
     * /zoom/x0/y0/x1/y1/
     *
     */
   std::string query(const Query& query);

private:
   Runner() = default;
	virtual ~Runner() = default;

   std::mutex mutex;
   std::unique_ptr<SpatialElement> quadtree;
};
