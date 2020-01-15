cmake_minimum_required(VERSION 2.8)

set(THIS_NAME med)

project(${THIS_NAME})
enable_language(CXX)

message(STATUS "-------------------------------------------------------")
message(STATUS "MED library optional configuration:")
message(STATUS "\t(use ccmake or cmake -DWITH_XX=true)")
message(STATUS "-------------------------------------------------------")
option (WITH_BM "With Google benchmark." OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

include_directories(
	${GTEST_INCLUDE_DIRS}
	${PROJECT_SOURCE_DIR}/med
)

#find_package(Boost)
#if(Boost_FOUND)
#	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${Boost_INCLUDE_DIRS})
#else()
#	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBRIGAND_NO_BOOST_SUPPORT")
#endif()

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


include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++17" COMPILER_SUPPORTS_CXX17)
if (COMPILER_SUPPORTS_CXX17)
	add_compile_options(-std=c++17)
else ()
	message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++17 support. Please use a different C++ compiler.")
endif ()


# Library path
set(CMAKE_LDFLAGS "${CMAKE_LDFLAGS} -L\".\" ")

file(GLOB_RECURSE MED_SRCS med/*.hpp)
# file(GLOB_RECURSE UT_SRCS ut/*.cpp)
set(UT_SRCS 
	#ut/bits.cpp
	#ut/choice.cpp
	#ut/copy.cpp
	#ut/diameter.cpp
	#ut/gtpc.cpp
#	#ut/json.cpp
	#ut/length.cpp
	#ut/med.cpp
	#ut/meta.cpp
	#ut/multi.cpp
	#ut/octets.cpp
	#ut/padding.cpp
	#ut/print.cpp
	#ut/protobuf.cpp
	#ut/sequence.cpp
	ut/set.cpp
	#ut/unique.cpp
	ut/asn/ber.cpp
)
file(GLOB_RECURSE BM_SRCS benchmark/*.cpp)

#expose private for UTs
#add_compile_definitions(UNIT_TEST)
add_definitions(-DUNIT_TEST)

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
	#-march=native
	-mtune=native
)

#set(PROJECT_CXXFLAGS "${PROJECT_CXXFLAGS} -Wno-gnu-zero-variadic-macro-arguments")

add_executable(gtest_med ${UT_SRCS} ${MED_SRCS})

if (WITH_BM)
	add_executable(bm_med ${BM_SRCS})
#	set_target_properties(bm_med PROPERTIES
#		INTERPROCEDURAL_OPTIMIZATION ON)
	target_link_libraries(bm_med
		${BENCHMARK_LIBS}
		${CMAKE_THREAD_LIBS_INIT}
	)
	add_custom_target(benchmark COMMAND echo "Benchmarking....."
		DEPENDS bm_med
	)
endif (WITH_BM)

# ------------------------------------------------------------------------------
# Coverage
# ------------------------------------------------------------------------------
if(COVERAGE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

#if(DEFINED ENV{BUILD_FLAGS})
#	set(BUILD_FLAGS "$ENV{BUILD_FLAGS}")
#endif ()
#set_target_properties(gtest_${THIS_NAME} PROPERTIES COMPILE_FLAGS
#	${BUILD_FLAGS}
#)

target_link_libraries(gtest_med
	${GTEST_BOTH_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
)

enable_testing()
add_test(UT gtest_${THIS_NAME})
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
	DEPENDS gtest_${THIS_NAME}
)
