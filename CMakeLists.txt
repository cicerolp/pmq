project(twitterVis)
cmake_minimum_required(VERSION 2.8)

aux_source_directory(. SRC_LIST)

set(PMA_BUILD_DIR "../hppsimulations/pma_cd-release" CACHE PATH "Location of the PMA build tree")

#include pma export tree
include("${PMA_BUILD_DIR}/inc/pma/pma-exports.cmake")

get_target_property(PMA_INCLUDE_DIR pma PROJECT_SOURCE_DIR)

add_executable(${PROJECT_NAME} ${SRC_LIST}

include/morton.h

include/DMPLoader/dmploader.hpp
include/DMPLoader/dmploader.cpp

include/stde.h
include/types.h

include/string_util.h
include/string_util.cpp

include/SpatialElement.h
include/SpatialElement.cpp

include/mongoose/mongoose.h
include/mongoose/mongoose.c

include/Server.h
include/Server.cpp

include/Query.h
include/Query.cpp

include/PMAInstance.h
include/PMAInstance.cpp

include/Singleton.h
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


target_link_libraries(${PROJECT_NAME} pma  ${CMAKE_THREAD_LIBS_INIT} )


