#pragma once

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

// mongoose http server
#include <mongoose/mongoose.h>

// rapidjson
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

using json_writer = rapidjson::Writer<rapidjson::StringBuffer>;