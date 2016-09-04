project(twitterVis)
cmake_minimum_required(VERSION 2.8)

aux_source_directory(. SRC_LIST)




#include pma export tree
include("${PROJECT_SOURCE_DIR}/../hppsimulations/pma_cd-release/inc/pma/pma-exports.cmake")

get_target_property(PMA_INCLUDE_DIR pma PROJECT_SOURCE_DIR)

add_executable(${PROJECT_NAME} ${SRC_LIST}
                               include/DMPLoader/dmploader.cpp
)


#SET(CMAKE_C_COMPILER /usr/bin/g++)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#SET(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS}")

# Your-external "mylib", add GLOBAL if the imported library is located in directories above the current.
#add_library( pma SHARED IMPORTED GLOBAL)
# You can define two import-locations: one for debug and one for release.
#set_target_properties( mylib PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/res/ ) # DONT KNOW IF THIS WILL WORK

#LINK_DIRECTORIES("../hppsimulations/pma_cd-release/inc/pma")
#target_link_libraries (twitterVis pma )

target_link_libraries(${PROJECT_NAME} pma)


