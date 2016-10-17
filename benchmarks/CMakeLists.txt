
add_executable(bench_micro_pmq
   bench_micro_pmq.cpp

   ../include/QuadtreeIntf.cpp

   ../include/PMABatchCtn.cpp
   ../include/DenseVectorCtn.cpp
)

target_link_libraries(bench_micro_pmq pma ${CMAKE_THREAD_LIBS_INIT} )

add_executable(bench_queries_pmq
   bench_queries_pmq.cpp
      
   ../include/PMABatch.cpp
   ../include/DenseVector.cpp
   ../include/QuadtreeIntf.cpp
)

target_link_libraries(bench_queries_pmq pma ${CMAKE_THREAD_LIBS_INIT} )

add_executable(bench_insert_and_scan bench_insert_and_scan.cpp

   ../include/PMABatch.cpp
   ../include/DenseVector.cpp
   ../include/QuadtreeIntf.cpp
)

target_link_libraries(bench_insert_and_scan pma ${CMAKE_THREAD_LIBS_INIT} )
