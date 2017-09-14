if(FALSE) #Disable test for explicit PMQ
add_executable(test_pma_diff
        test_pma_diff.cpp
        ../include/PMABatchCtn.cpp
        ../include/QuadtreeIntf.cpp
        )

target_link_libraries(test_pma_diff pma ${CMAKE_THREAD_LIBS_INIT})
add_test(pma_diff_b10 test_pma_diff -f ${CMAKE_SOURCE_DIR}/data/tweet10_4.dat -b 10)
endif()
########################################################

# TODO fix this test

#add_executable(test_pma_quadtree_counts test_pma_quadtree_counts.cpp
#   ../include/PMABatchCtn.cpp
#   ../include/QuadtreeIntf.cpp
#)

#target_link_libraries(test_pma_quadtree_counts pma  ${CMAKE_THREAD_LIBS_INIT} )
#add_test(pma_quadtree_counts_b10 test_pma_quadtree_counts -f ${CMAKE_SOURCE_DIR}/data/tweet10_4.dat -b 10)


#######################################################
add_executable(test_geohash
        test_geohash.cpp

        ../include/QuadtreeIntf.cpp
        ../include/GeoHash.cpp
        )

target_link_libraries(test_geohash pma ${CMAKE_THREAD_LIBS_INIT})

##########################################################
### Teste the insertion with removal
add_executable(test_insert_remove_scan
        test_insert_remove_scan.cpp

        ../include/QuadtreeIntf.cpp
        ../include/GeoHash.cpp
#        ../include/PMABatchCtn.cpp
        ../include/ExplicitDenseVectorCtn.cpp
        )

target_link_libraries(test_insert_remove_scan pma ${CMAKE_THREAD_LIBS_INIT})

#######################################################

add_executable(test_geohash_count
        test_geohash_count.cpp

        ../include/QuadtreeIntf.cpp
        ../include/GeoHash.cpp
        )

target_link_libraries(test_geohash_count pma ${CMAKE_THREAD_LIBS_INIT})

#######################################################

add_executable(test_containers_count
        test_containers_count.cpp

        ../include/QuadtreeIntf.cpp
        ../include/GeoHash.cpp
        ../include/BTreeCtn.cpp
        )

target_link_libraries(test_containers_count pma ${CMAKE_THREAD_LIBS_INIT})
