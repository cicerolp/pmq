cmake_minimum_required(VERSION 3.0)
project(PMQ)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
set(PMA_BUILD_DIR "../pma/build-release" CACHE PATH "Location of the PMA build tree")

#aux_source_directory(. SRC_LIST)

file(GLOB HEADERS "include/*.h")
source_group("Header Files" FILES ${HEADERS})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")

include(cotire)

#include pma export tree
include("${PMA_BUILD_DIR}/inc/pma/pma-exports.cmake")

get_target_property(PMA_INCLUDE_DIR pma PROJECT_SOURCE_DIR)

#add_executable(${PROJECT_NAME}
        #${SRC_LIST}
        #${HEADERS}

        #include/mongoose/mongoose.c

        #include/Query.cpp
        #include/Server.cpp
        #include/GeoRunner.cpp
        #)

include_directories(
        "${PROJECT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/include"
)

FIND_PACKAGE(Threads REQUIRED)

set(Boost_USE_STATIC_LIBS ON) # only find static libs
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.64.0 REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

#target_link_libraries(${PROJECT_NAME}
        #pma
        #${CMAKE_THREAD_LIBS_INIT}
        #${Boost_LIBRARIES}
        #)

######## PMQ Tests
enable_testing()
add_subdirectory(tests)

######## Benchmarks
add_subdirectory(benchmarks)

#set_target_properties(${PROJECT_NAME} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "include/stde.h")
#cotire(${PROJECT_NAME})
