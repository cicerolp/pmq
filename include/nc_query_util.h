#pragma once

#include "Query.h"
#include "string_util.h"

namespace nc_query_util {

   static Query get_from_url(const std::string& url);

   static std::vector<Query> get_from_file(const std::string& file);

} // namespace nc_query_util
