add_executable(test_pmq_count
        test_pmq_count.cpp
        )

target_link_libraries(test_pmq_count pma ${CMAKE_THREAD_LIBS_INIT})

add_executable(test_containers_count
        test_containers_count.cpp

        #../include/BTreeCtn.cpp
        #../include/ImplicitDenseVectorCtn.cpp
        )

target_link_libraries(test_containers_count pma ${CMAKE_THREAD_LIBS_INIT})
