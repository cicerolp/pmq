
add_executable(bench_micro_pmq bench_micro_pmq.cpp
../include/DMPLoader/dmploader.cpp
../include/SpatialElement.cpp
)


target_link_libraries(bench_micro_pmq pma ${CMAKE_THREAD_LIBS_INIT} )
