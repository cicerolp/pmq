set(Boost_USE_STATIC_LIBS ON) # only find static libs
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)


option(BENCH_PMQ "Enable PMQ in the benchmarks" ON)
option(BENCH_BTREE "Enable BTREE in the benchmarks" ON)
option(BENCH_RTREE "Enable RTREE in the benchmarks" ON)
option(BENCH_DENSE "Enable Dense vectors in the benchmarks" ON)

configure_file (benchmarkconfig.h.in benchmarkconfig.h)
include_directories("${CMAKE_CURRENT_BINARY_DIR}") # To find the .h

find_package(Boost 1.64.0 REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

file(GLOB HEADERS "*.h" "../include/*.h")
source_group("Header Files" FILES ${HEADERS})

add_executable(bench_insert_remove_scan ${HEADERS}
        bench_insert_remove_scan.cpp
        )

target_link_libraries(bench_insert_remove_scan pma ${CMAKE_THREAD_LIBS_INIT})

add_executable(bench_insert_remove_count ${HEADERS}
        bench_insert_remove_count.cpp
        )

target_link_libraries(bench_insert_remove_count pma ${CMAKE_THREAD_LIBS_INIT})

add_executable(bench_queries_region ${HEADERS}
        bench_queries_region.cpp
        )

target_link_libraries(bench_queries_region
        pma
        ${CMAKE_THREAD_LIBS_INIT}
        ${Boost_LIBRARIES}
        )

add_executable(bench_insert_and_scan ${HEADERS}
        bench_insert_and_scan.cpp
        )

target_link_libraries(bench_insert_and_scan
        pma
        ${CMAKE_THREAD_LIBS_INIT}
        ${Boost_LIBRARIES}
        )

add_executable(bench_memory ${HEADERS}
        bench_memory.cpp
        )

target_link_libraries(bench_memory
        pma
        ${CMAKE_THREAD_LIBS_INIT}
        ${Boost_LIBRARIES}
        )
