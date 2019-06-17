cmake_minimum_required(VERSION 3.9)
project(arbor-sonata LANGUAGES CXX)
ENABLE_LANGUAGE(C)

cmake_policy(SET CMP0074 NEW)
set (CMAKE_CXX_STANDARD 14)

find_package(MPI REQUIRED CXX)
find_package(arbor REQUIRED)
find_package(HDF5 REQUIRED)

add_subdirectory(test)
add_subdirectory(example)

