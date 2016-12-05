project(twitterVis)
cmake_minimum_required (VERSION 3.0)

if(WIN32)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
   set(PMA_BUILD_DIR "../pma/build-vs" CACHE PATH "Location of the PMA build tree")

   set(BOOST_LIBRARYDIR "C:/Boost/lib/")
   set(BOOST_INCLUDEDIR "C:/Boost/include/boost-1_62/")

elseif(UNIX)

   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
   set(PMA_BUILD_DIR "../pma/build-release" CACHE PATH "Location of the PMA build tree")
   
   add_custom_target(debug
     COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug ${CMAKE_SOURCE_DIR}
     COMMENT "Switch CMAKE_BUILD_TYPE to Debug"
   )

   add_custom_target(release
     COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release ${CMAKE_SOURCE_DIR}
     COMMENT "Switch CMAKE_BUILD_TYPE to Release"
   )
endif()

aux_source_directory(. SRC_LIST)

file(GLOB HEADERS "include/*.h")
source_group("Header Files" FILES ${HEADERS})

set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")

include(cotire)

#include pma export tree
include("${PMA_BUILD_DIR}/inc/pma/pma-exports.cmake")

get_target_property(PMA_INCLUDE_DIR pma PROJECT_SOURCE_DIR)

add_executable(${PROJECT_NAME} ${SRC_LIST} ${HEADERS}
   include/GeoRunner.cpp
   
   include/QuadtreeIntf.cpp
   
   include/PostGisCtn.cpp
   include/PMABatchCtn.cpp
   include/SpatiaLiteCtn.cpp
   include/DenseVectorCtn.cpp

   include/string_util.cpp
   include/nc_query_util.cpp

   include/Query.cpp
   include/Server.cpp

   include/mongoose/mongoose.c
)

include_directories(
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/include")

FIND_PACKAGE(Threads REQUIRED)

if(WIN32)
   target_link_libraries(${PROJECT_NAME} pma ${CMAKE_THREAD_LIBS_INIT})

elseif(UNIX)
   FIND_LIBRARY(PQ_C_LIBRARY NAMES pq)
   FIND_LIBRARY(GEOS_C_LIBRARY NAMES geos_c)
   FIND_LIBRARY(SPATIALITE_LIBRARY NAMES spatialite)
   FIND_LIBRARY(SQLITE_LIBRARY NAMES sqlite3)

   target_link_libraries(${PROJECT_NAME} pma ${CMAKE_THREAD_LIBS_INIT} ${SPATIALITE_LIBRARY} ${SQLITE_LIBRARY} ${GEOS_C_LIBRARY} ${PQ_C_LIBRARY})

   ######## Benchmarks
   add_subdirectory(benchmarks)

   ######## PMQ Tests
   enable_testing()
   add_subdirectory(tests)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "include/stde.h")
cotire(${PROJECT_NAME})
