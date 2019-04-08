cmake_minimum_required(VERSION 3.9)
project(arbor-sonata LANGUAGES CXX)
ENABLE_LANGUAGE(C)

set (CMAKE_CXX_STANDARD 14)

# When arbor PR is in, this will become find_package(arbor REQUIRED MPI)
find_package(arbor REQUIRED)
find_package(HDF5 REQUIRED)

add_executable(sonata sonata.cpp hdf5_lib.hpp data_management_lib.hpp csv_lib.hpp mpi_helper.hpp)
target_link_libraries(sonata PRIVATE arbor::arbor arbor::arborenv ${HDF5_C_LIBRARIES})
target_include_directories(sonata PRIVATE ../common/cpp/include)

set_target_properties(sonata PROPERTIES OUTPUT_NAME arbor-sonata)

install(TARGETS sonata DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
