project(twitterVis)
cmake_minimum_required(VERSION 2.8)

aux_source_directory(. SRC_LIST)

set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")

include(cotire)

set(PMA_BUILD_DIR "../pma/build-release" CACHE PATH "Location of the PMA build tree")

#include pma export tree
include("${PMA_BUILD_DIR}/inc/pma/pma-exports.cmake")

get_target_property(PMA_INCLUDE_DIR pma PROJECT_SOURCE_DIR)

add_executable(${PROJECT_NAME} ${SRC_LIST}

include/GeoRunner.cpp

include/GeoCtnIntf.h

include/SpatiaLiteCtn.h
include/SpatiaLiteCtn.cpp

include/DenseVectorCtn.h
include/DenseVectorCtn.cpp

include/PMABatchCtn.h
include/PMABatchCtn.cpp

include/nc_query_util.h
include/nc_query_util.cpp

include/DenseVector.cpp
include/PMABatch.cpp

include/morton.h

include/stde.h
include/types.h

include/string_util.h
include/string_util.cpp

include/QuadtreeIntf.h
include/QuadtreeIntf.cpp

include/mongoose/mongoose.h
include/mongoose/mongoose.c

include/Server.h
include/Server.cpp

include/Query.h
include/Query.cpp

include/Runner.h
include/Runner.cpp

include/Singleton.h

include/ContainerIntf.h

include/InputIntf.h
)

include_directories(
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/include")

#SET(CMAKE_C_COMPILER /usr/bin/g++)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
#SET(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS}")

# Your-external "mylib", add GLOBAL if the imported library is located in directories above the current.
#add_library( pma SHARED IMPORTED GLOBAL)
# You can define two import-locations: one for debug and one for release.
#set_target_properties( mylib PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/res/ ) # DONT KNOW IF THIS WILL WORK

#LINK_DIRECTORIES("../hppsimulations/pma_cd-release/inc/pma")
#target_link_libraries (twitterVis pma )
#message(STATUS "System libs : " ${SYSTEM_LIBS} )

FIND_PACKAGE(Threads REQUIRED)


FIND_LIBRARY(GEOS_C_LIBRARY NAMES geos_c)
FIND_LIBRARY(SPATIALITE_LIBRARY NAMES spatialite)
FIND_LIBRARY(SQLITE_LIBRARY NAMES sqlite3)

target_link_libraries(${PROJECT_NAME} pma  ${CMAKE_THREAD_LIBS_INIT} ${SPATIALITE_LIBRARY} ${SQLITE_LIBRARY} ${GEOS_C_LIBRARY})

set_target_properties(${PROJECT_NAME} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "include/stde.h")
cotire(${PROJECT_NAME})

######## Benchmarks
add_subdirectory(benchmarks)


######## PMQ Tests ############
enable_testing()
add_subdirectory(tests)


