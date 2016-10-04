
add_executable(test_pma_diff
test_pma_diff.cpp
../include/PMABatch.cpp
)

target_link_libraries(test_pma_diff pma  ${CMAKE_THREAD_LIBS_INIT} )
add_test(pma_diff_b10 test_pma_diff -f ${CMAKE_SOURCE_DIR}/data/tweet10_4.dat -b 10)

########################################################

add_executable(test_pma_quadtree_counts test_pma_quadtree_counts.cpp
../include/PMABatch.cpp
	../include/QuadtreeIntf.cpp
)

target_link_libraries(test_pma_quadtree_counts pma  ${CMAKE_THREAD_LIBS_INIT} )
add_test(pma_quadtree_counts_b10 test_pma_quadtree_counts -f ${CMAKE_SOURCE_DIR}/data/tweet10_4.dat -b 10)


