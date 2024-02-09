cmake_minimum_required(VERSION 3.27)
project(tsc_benchmark)

set(CMAKE_CXX_STANDARD 20)

#todo split headers and sources
# Add headers
set(INCLUDE_DIRECTORY ${CMAKE_SOURCE_DIR}/include/)
set(SOURCE_DIRECTORY ${CMAKE_SOURCE_DIR}/source/)

list(APPEND BENCHMARKING_DIRECTORIES ${INCLUDE_DIRECTORY} ${SOURCE_DIRECTORY})

# Add targets
set(BENCHMARK_TARGET benchmark_example)

# Add executables
add_executable(BENCHMARK_TARGET example.cpp)

# Include directories
target_include_directories(BENCHMARK_TARGET PRIVATE ${BENCHMARKING_DIRECTORIES})
