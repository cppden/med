cmake_minimum_required(VERSION 2.8)

set(THIS_NAME med)

project(${THIS_NAME})
enable_language(CXX)

message(STATUS "-------------------------------------------------------")
message(STATUS "MED library optional configuration:")
message(STATUS "\t(use ccmake or cmake -DWITH_XX=true)")
message(STATUS "-------------------------------------------------------")
option (WITH_BM "With Google benchmark." OFF)
option (WITH_EX "With exceptions." OFF)

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
  message(STATUS "\tWITH_BM=true (google benchmark)")
  find_package(Benchmark REQUIRED)
  if (BENCHMARK_FOUND)
    include_directories( ${BENCHMARK_INCLUDE_DIRS})
    message(STATUS "Google benchmark found.   ${BENCHMARK_LIBS}")
  else (BENCHMARK_FOUND)
    message(FATAL_ERROR "Google benchmark not found. Disable WITH_BM or install it.")
  endif (BENCHMARK_FOUND)
else (WITH_BM)
  message(STATUS "\tWITH_BM=false (no benchmark)")
endif (WITH_BM)

if (WITH_EX)
  message(STATUS "\tWITH_EX=true (exceptions for error handling)")
  add_definitions(-DMED_EXCEPTIONS=1)
else (WITH_EX)
  add_definitions(-DMED_EXCEPTIONS=0)
  message(STATUS "\tWITH_EX=false (bool for error handling)")
endif (WITH_EX)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++17" COMPILER_SUPPORTS_CXX17)
if (COMPILER_SUPPORTS_CXX17)
    add_definitions(-std=c++17)
else ()
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++17 support. Please use a different C++ compiler.")
endif ()


# Library path
set(CMAKE_LDFLAGS "${CMAKE_LDFLAGS} -L\".\" ")

file(GLOB_RECURSE UT_SRCS ut/*.cpp)
file(GLOB_RECURSE BM_SRCS benchmark/*.cpp)

add_compile_options(
    -Werror 
    -Wall 
    -Wextra 
    -Waddress
    -Warray-bounds
    -Winit-self
    -Wunreachable-code
    -pedantic
    -pedantic-errors
)

#set(PROJECT_CXXFLAGS "${PROJECT_CXXFLAGS} -Wno-gnu-zero-variadic-macro-arguments")

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

if(DEFINED ENV{BUILD_FLAGS})
    set(BUILD_FLAGS "$ENV{BUILD_FLAGS}")
else ()
    set(BUILD_FLAGS "-O3")
endif ()

set_target_properties(gtest_${THIS_NAME} PROPERTIES COMPILE_FLAGS 
    ${BUILD_FLAGS}
)

target_link_libraries(gtest_med
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT} 
)

enable_testing()
add_test(UT gtest_${THIS_NAME})
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
  DEPENDS gtest_${THIS_NAME}
)
