add_executable(bench_topk_search
      bench_topk_search.cpp

	  ../include/GeoHash.cpp
	  ../include/string_util.cpp
      ../include/QuadtreeIntf.cpp
   )

   target_link_libraries(bench_topk_search pma ${CMAKE_THREAD_LIBS_INIT})

add_executable(bench_insert_remove_scan
      bench_insert_remove_scan.cpp

      ../include/QuadtreeIntf.cpp
      ../include/GeoHash.cpp
      ../include/GeoHash.cpp
      ../include/PMABatchCtn.cpp
      ../include/DenseVectorCtn.cpp
   )

   target_link_libraries(bench_insert_remove_scan pma ${CMAKE_THREAD_LIBS_INIT})

if(WIN32)
   add_executable(bench_queries_region
      bench_queries_region.cpp

	  ../include/string_util.cpp

      ../include/QuadtreeIntf.cpp
      
      ../include/GeoHash.cpp
      ../include/PostGisCtn.cpp
      ../include/PMABatchCtn.cpp
      ../include/SpatiaLiteCtn.cpp
      ../include/DenseVectorCtn.cpp
   )

   target_link_libraries(bench_queries_region pma ${CMAKE_THREAD_LIBS_INIT})

   add_executable(bench_micro_pmq
      bench_micro_pmq.cpp

      ../include/QuadtreeIntf.cpp
      
      ../include/GeoHash.cpp
      ../include/PostGisCtn.cpp
      ../include/PMABatchCtn.cpp
      ../include/SpatiaLiteCtn.cpp
      ../include/DenseVectorCtn.cpp
   )

   target_link_libraries(bench_micro_pmq pma ${CMAKE_THREAD_LIBS_INIT})

   add_executable(bench_queries_pmq
      bench_queries_pmq.cpp
         
      ../include/QuadtreeIntf.cpp

      ../include/GeoHash.cpp
      ../include/PostGisCtn.cpp
      ../include/PMABatchCtn.cpp
      ../include/SpatiaLiteCtn.cpp
      ../include/DenseVectorCtn.cpp
   )

   target_link_libraries(bench_queries_pmq pma ${CMAKE_THREAD_LIBS_INIT})

   add_executable(bench_insert_and_scan 
      bench_insert_and_scan.cpp

      ../include/QuadtreeIntf.cpp

      ../include/GeoHash.cpp
      ../include/PostGisCtn.cpp
      ../include/PMABatchCtn.cpp
      ../include/SpatiaLiteCtn.cpp
      ../include/DenseVectorCtn.cpp
   )

   target_link_libraries(bench_insert_and_scan pma ${CMAKE_THREAD_LIBS_INIT})

elseif(UNIX)
   FIND_LIBRARY(PQ_C_LIBRARY NAMES pq)
   FIND_LIBRARY(GEOS_C_LIBRARY NAMES geos_c)
   FIND_LIBRARY(SPATIALITE_LIBRARY NAMES spatialite)
   FIND_LIBRARY(SQLITE_LIBRARY NAMES sqlite3)

   add_executable(bench_queries_region
      bench_queries_region.cpp

	  ../include/string_util.cpp

      ../include/QuadtreeIntf.cpp
      
      ../include/GeoHash.cpp
      ../include/PostGisCtn.cpp
      ../include/PMABatchCtn.cpp
      ../include/SpatiaLiteCtn.cpp
      ../include/DenseVectorCtn.cpp
   )

   target_link_libraries(bench_queries_region pma ${CMAKE_THREAD_LIBS_INIT} ${SPATIALITE_LIBRARY} ${SQLITE_LIBRARY} ${GEOS_C_LIBRARY} ${PQ_C_LIBRARY})

   add_executable(bench_micro_pmq
      bench_micro_pmq.cpp

      ../include/QuadtreeIntf.cpp
      
      ../include/GeoHash.cpp
      ../include/PostGisCtn.cpp
      ../include/PMABatchCtn.cpp
      ../include/SpatiaLiteCtn.cpp
      ../include/DenseVectorCtn.cpp
   )

   target_link_libraries(bench_micro_pmq pma ${CMAKE_THREAD_LIBS_INIT} ${SPATIALITE_LIBRARY} ${SQLITE_LIBRARY} ${GEOS_C_LIBRARY} ${PQ_C_LIBRARY})

   add_executable(bench_queries_pmq
      bench_queries_pmq.cpp
         
      ../include/QuadtreeIntf.cpp

      ../include/GeoHash.cpp
      ../include/PostGisCtn.cpp
      ../include/PMABatchCtn.cpp
      ../include/SpatiaLiteCtn.cpp
      ../include/DenseVectorCtn.cpp
   )

   target_link_libraries(bench_queries_pmq pma ${CMAKE_THREAD_LIBS_INIT} ${SPATIALITE_LIBRARY} ${SQLITE_LIBRARY} ${GEOS_C_LIBRARY} ${PQ_C_LIBRARY})

   add_executable(bench_insert_and_scan 
      bench_insert_and_scan.cpp

      ../include/QuadtreeIntf.cpp

      ../include/GeoHash.cpp
      ../include/PostGisCtn.cpp
      ../include/PMABatchCtn.cpp
      ../include/SpatiaLiteCtn.cpp
      ../include/DenseVectorCtn.cpp
   )

   target_link_libraries(bench_insert_and_scan pma ${CMAKE_THREAD_LIBS_INIT} ${SPATIALITE_LIBRARY} ${SQLITE_LIBRARY} ${GEOS_C_LIBRARY} ${PQ_C_LIBRARY})
endif()
