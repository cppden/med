# find Google Benchmark Library and headers
#
# BENCHMARK_INCLUDE_DIRS = benchmark headers
# BENCHMARK_LIBS = benchmark lib

find_package(Threads REQUIRED)

if (BENCHMARK_INCLUDE_DIRS AND BENCHMARK_LIBS)
  set(BENCHMARK_FIND_QUIETLY TRUE)
endif (BENCHMARK_INCLUDE_DIRS AND BENCHMARK_LIBS)


find_path(BENCHMARK_INCLUDE_DIRS NAMES benchmark/benchmark.h
  /usr/local/include
  /opt/local/include
  /usr/include
  /opt/include
)

find_library(BENCHMARK_LIBS NAMES libbenchmark.a)


include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(BENCHMARK DEFAULT_MSG BENCHMARK_INCLUDE_DIRS BENCHMARK_LIBS)


mark_as_advanced(BENCHMARK_INCLUDE_DIRS BENCHMARK_LIBS)

