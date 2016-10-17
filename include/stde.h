#pragma once
#pragma GCC system_header
#pragma system_header

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

// singleton
#include "Singleton.h"

// mongoose http server
#include <mongoose/mongoose.h>

// rapidjson
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

// pma includes
#include "pma/pma.h"
#include "pma/utils/test_utils.h"
#include "pma/utils/debugMacros.h"
#include "pma/utils/benchmark_utils.h"

// retrieving command line arguments
#include "ext/CImg/CImg.h"

// geos lib
#include <geos_c.h>

// spatialite
#include <sqlite3.h>
#include <spatialite.h>
#include <spatialite/gaiageo.h>

// postgis
#include <postgresql/libpq-fe.h>