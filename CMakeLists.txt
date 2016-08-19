cmake_minimum_required(VERSION 2.8.11)

project(med_ut)



include_directories(
  ${PROJECT_SOURCE_DIR}/med/
)

# Linker options
set(CMAKE_LDFLAGS "${CMAKE_LDFLAGS} ")

# Library path
set(CMAKE_LDFLAGS "${CMAKE_LDFLAGS} -L\".\" ")

# Define the CXX sources
set ( CXX_SRCS
    ut/med.cpp
)

set_source_files_properties(
    ${CXX_SRCS} PROPERTIES COMPILE_FLAGS 
    " -g -O3 -std=c++14 -Wall -Werror")

add_executable(med_ut ${CXX_SRCS} ${C_SRCS})

target_link_libraries(med_ut
    gtest
)

enable_testing()
add_test(UT med_ut)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS med_ut)
