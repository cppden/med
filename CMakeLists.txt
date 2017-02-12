cmake_minimum_required(VERSION 2.8)
project(med)
enable_language(CXX)

message(STATUS "-------------------------------------------------------")
message(STATUS "MED library optional configuration:")
message(STATUS "\t(use ccmake or cmake -DWITH_XXX=true)")
message(STATUS "-------------------------------------------------------")
option (WITH_BM "With Google Benchmark." OFF)
option (WITH_EX "With throw exception." OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

include_directories(
  ${GTEST_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/med
)

# -----------------------------------------------------------------------------
# Benchmarks
# -----------------------------------------------------------------------------
if (WITH_BM)
  message(STATUS "\tWITH_BM=true (Google Benchmark)")
  find_package(Benchmark REQUIRED)
  if (BENCHMARK_FOUND)
    include_directories( ${BENCHMARK_INCLUDE_DIRS})
    message(STATUS "Google Benchmark found.   ${BENCHMARK_LIBS}")
  else (BENCHMARK_FOUND)
    message(FATAL_ERROR "Google Benchmark not found. Disable WITH_BM or install it.")
  endif (BENCHMARK_FOUND)
endif (WITH_BM)

if (WITH_EX)
  message(STATUS "Throwing exceptions for error handling.")
else (WITH_EX)
  add_definitions(-DMED_NO_EXCEPTIONS)
  message(STATUS "Not using exceptions for error handling.")
endif (WITH_EX)

#set(CMAKE_LDFLAGS "${CMAKE_LDFLAGS} ")

# Library path
set(CMAKE_LDFLAGS "${CMAKE_LDFLAGS} -L\".\" ")

set(UT_SRCS
  ut/med.cpp
)

set(BM_SRCS
  benchmark/bm.cpp
)

set_source_files_properties(
  ${UT_SRCS} ${BM_SRCS} PROPERTIES COMPILE_FLAGS
  "-O3 -std=c++14 -Wall -Werror"
)


add_executable(gtest_med ${UT_SRCS})

if (WITH_BM)
  add_executable(bm_med ${BM_SRCS})
#  set_target_properties(bm_med PROPERTIES
#    INTERPROCEDURAL_OPTIMIZATION ON)
  target_link_libraries(bm_med
    ${BENCHMARK_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
  )
  add_custom_target(benchmark COMMAND echo "Benchmarking....."
    DEPENDS bm_med
  )
endif (WITH_BM)

target_link_libraries(gtest_med
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT} 
)

enable_testing()
add_test(UT gtest_med)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
  DEPENDS gtest_med
)
