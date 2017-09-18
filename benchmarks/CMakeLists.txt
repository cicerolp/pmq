set(Boost_USE_STATIC_LIBS ON) # only find static libs
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.64.0 REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

add_executable(bench_insert_remove_scan
        bench_insert_remove_scan.cpp

        ../include/BTreeCtn.cpp
        ../include/ImplicitDenseVectorCtn.cpp
        )

target_link_libraries(bench_insert_remove_scan pma ${CMAKE_THREAD_LIBS_INIT})

add_executable(bench_insert_remove_count
        bench_insert_remove_count.cpp

        ../include/BTreeCtn.cpp
        ../include/ImplicitDenseVectorCtn.cpp
        )

target_link_libraries(bench_insert_remove_count pma ${CMAKE_THREAD_LIBS_INIT})

add_executable(bench_queries_region
        bench_queries_region.cpp

        ../include/BTreeCtn.cpp
        ../include/ImplicitDenseVectorCtn.cpp
        )

target_link_libraries(bench_queries_region
        pma
        ${CMAKE_THREAD_LIBS_INIT}
        ${Boost_LIBRARIES}
        )

add_executable(bench_insert_and_scan
        bench_insert_and_scan.cpp

        #../include/BTreeCtn.cpp
        #../include/ImplicitDenseVectorCtn.cpp
        )

target_link_libraries(bench_insert_and_scan
        pma
        ${CMAKE_THREAD_LIBS_INIT}
        ${Boost_LIBRARIES}
        )

add_executable(bench_memory
        bench_memory.cpp

        ../include/BTreeCtn.cpp
        ../include/ImplicitDenseVectorCtn.cpp
        )

target_link_libraries(bench_memory
        pma
        ${CMAKE_THREAD_LIBS_INIT}
        ${Boost_LIBRARIES}
        )
