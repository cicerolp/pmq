project(twitterVis)
cmake_minimum_required (VERSION 3.0)

if(WIN32)
   if (MSVC)
      set(CMAKE_CXX_FLAGS "/EHsc")
   endif()   

   set(BOOST_LIBRARYDIR "C:/Boost/lib/")
   set(BOOST_INCLUDEDIR "C:/Boost/include/boost-1_62/")
endif()

if(UNIX)
   set(CMAKE_CXX_FLAGS "-std=c++1z")
   
   add_custom_target(debug
     COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug ${CMAKE_SOURCE_DIR}
     COMMENT "Switch CMAKE_BUILD_TYPE to Debug"
   )

   add_custom_target(release
     COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release ${CMAKE_SOURCE_DIR}
     COMMENT "Switch CMAKE_BUILD_TYPE to Release"
   )

   add_custom_target(clang
     COMMAND ${CMAKE_COMMAND} -DCMAKE_C_COMPILER=/usr/bin/clang  -DCMAKE_CXX_COMPILER=/usr/bin/clang++ ${CMAKE_SOURCE_DIR}
     COMMENT "Force CMAKE_C/CXX_COMPILER to Clang"
   )

   add_custom_target(gcc
     COMMAND ${CMAKE_COMMAND} -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ ${CMAKE_SOURCE_DIR}
     COMMENT "Force CMAKE_C/CXX_COMPILER to GCC"
   )
endif()

aux_source_directory(. SRC_LIST)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")

include(cotire)

set(PMA_BUILD_DIR "../pma/clang-release" CACHE PATH "Location of the PMA build tree")

#include pma export tree
include("${PMA_BUILD_DIR}/inc/pma/pma-exports.cmake")

get_target_property(PMA_INCLUDE_DIR pma PROJECT_SOURCE_DIR)

add_executable(${PROJECT_NAME} ${SRC_LIST}
   include/GeoRunner.cpp

   include/GeoCtnIntf.h

   include/PostGisCtn.h
   include/PostGisCtn.cpp

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

FIND_PACKAGE(Threads REQUIRED)

FIND_LIBRARY(PQ_C_LIBRARY NAMES pq)
FIND_LIBRARY(GEOS_C_LIBRARY NAMES geos_c)
FIND_LIBRARY(SPATIALITE_LIBRARY NAMES spatialite)
FIND_LIBRARY(SQLITE_LIBRARY NAMES sqlite3)

target_link_libraries(${PROJECT_NAME} pma  ${CMAKE_THREAD_LIBS_INIT} ${SPATIALITE_LIBRARY} ${SQLITE_LIBRARY} ${GEOS_C_LIBRARY} ${PQ_C_LIBRARY})

set_target_properties(${PROJECT_NAME} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "include/stde.h")
cotire(${PROJECT_NAME})

######## Benchmarks
add_subdirectory(benchmarks)


######## PMQ Tests ############
enable_testing()
add_subdirectory(tests)


