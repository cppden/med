cmake_minimum_required(VERSION 2.8)
project(med)
enable_language(CXX)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

include_directories(
  ${GTEST_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/med/
)

#set(CMAKE_LDFLAGS "${CMAKE_LDFLAGS} ")

# Library path
set(CMAKE_LDFLAGS "${CMAKE_LDFLAGS} -L\".\" ")

# Define the CXX sources
set ( CXX_SRCS
    ut/med.cpp
)

set_source_files_properties(
    ${CXX_SRCS} PROPERTIES COMPILE_FLAGS 
    " -g -O3 -std=c++14 -Wall -Werror")

add_executable(gtest_med ${CXX_SRCS})

target_link_libraries(gtest_med
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT} 
)

enable_testing()
add_test(UT gtest_med)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS gtest_med)
