set(Boost_USE_STATIC_LIBS ON) # only find static libs
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.64.0 REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

file(GLOB HEADERS "*.h")
source_group("Header Files" FILES ${HEADERS})

add_executable(bench_insert_remove_scan ${HEADERS}
        bench_insert_remove_scan.cpp

        ../include/BTreeCtn.cpp
        ../include/ImplicitDenseVectorCtn.cpp
        )

target_link_libraries(bench_insert_remove_scan pma ${CMAKE_THREAD_LIBS_INIT})

add_executable(bench_insert_remove_count ${HEADERS}
        bench_insert_remove_count.cpp

        ../include/BTreeCtn.cpp
        ../include/ImplicitDenseVectorCtn.cpp
        )

target_link_libraries(bench_insert_remove_count pma ${CMAKE_THREAD_LIBS_INIT})

add_executable(bench_queries_region ${HEADERS}
        bench_queries_region.cpp

        ../include/BTreeCtn.cpp
        ../include/ImplicitDenseVectorCtn.cpp
        )

target_link_libraries(bench_queries_region
        pma
        ${CMAKE_THREAD_LIBS_INIT}
        ${Boost_LIBRARIES}
        )

add_executable(bench_insert_and_scan ${HEADERS}
        bench_insert_and_scan.cpp

        #../include/BTreeCtn.cpp
        #../include/ImplicitDenseVectorCtn.cpp
        )

target_link_libraries(bench_insert_and_scan
        pma
        ${CMAKE_THREAD_LIBS_INIT}
        ${Boost_LIBRARIES}
        )

add_executable(bench_memory ${HEADERS}
        bench_memory.cpp

        ../include/BTreeCtn.cpp
        ../include/ImplicitDenseVectorCtn.cpp
        )

target_link_libraries(bench_memory
        pma
        ${CMAKE_THREAD_LIBS_INIT}
        ${Boost_LIBRARIES}
        )
