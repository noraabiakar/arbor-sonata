add_library(sonata
        ../sonata/hdf5_lib.cpp
        ../sonata/data_management_lib.cpp
        ../sonata/dynamics_params_helper.cpp
        ../sonata/csv_lib.cpp)

target_link_libraries(sonata PRIVATE arbor::arbor arbor::arborenv ${HDF5_C_LIBRARIES})
target_include_directories(sonata PRIVATE ../common/cpp/include ${HDF5_INCLUDE_DIRS} ${MPI_CXX_INCLUDE_PATH})

add_executable(
        sonata-example
        example.cpp
        ../sonata/hdf5_lib.cpp
        ../sonata/data_management_lib.cpp
        ../sonata/dynamics_params_helper.cpp
        ../sonata/csv_lib.cpp)

target_link_libraries(sonata-example PRIVATE arbor::arbor arbor::arborenv ${HDF5_C_LIBRARIES})
target_include_directories(sonata-example PRIVATE ../common/cpp/include ${HDF5_INCLUDE_DIRS} ${MPI_CXX_INCLUDE_PATH})