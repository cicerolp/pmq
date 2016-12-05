#pragma once

#ifdef __GNUC__
   #pragma GCC system_header
   #pragma system_header
#elif _MSC_VER
   #define NOMINMAX
   #define _USE_MATH_DEFINES
#endif

// C includes
#include <cmath>
#include <cstdio>
#include <cassert>
#include <cstdint>

// C++ includes
#include <map>
#include <mutex>
#include <chrono>
#include <regex>
#include <array>
#include <queue>
#include <tuple>
#include <thread>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <functional>
#include <unordered_map>

// sorting algorithms
#include "sorting/timsort.hpp"

// mongoose http server
#include <mongoose/mongoose.h>

// singleton
#include "Singleton.h"

// rapidjson
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

// pma includes
#ifdef _MSC_VER
   #pragma warning( push )
   #pragma warning(disable: 4244; disable: 4018; disable: 4477; disable: 4244; disable: 4267; disable: 4129)
#endif
#include "pma/pma.h"
#include "pma/utils/test_utils.h"
#include "pma/utils/debugMacros.h"
#include "pma/utils/benchmark_utils.h"
#ifdef _MSC_VER
   #pragma warning( pop ) 
#endif

#undef sleep
#include "ext/CImg/CImg.h"

#ifdef __GNUC__
   // geos lib
   #include <geos_c.h>

   // spatialite
   #include <sqlite3.h>
   #include <spatialite.h>
   #include <spatialite/gaiageo.h>

   // postgis
   #include <postgresql/libpq-fe.h>
#endif


