# Build mechanisms used solely in unit tests.
set(unit_sources
    test_csv.cpp

    # unit test driver
    test.cpp
)

add_executable(unit ${unit_sources})
add_dependencies(tests unit)

target_compile_definitions(unit PRIVATE "-DDATADIR=\"${CMAKE_CURRENT_SOURCE_DIR}/inputs\"")
target_include_directories(unit PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(unit PRIVATE gtest arbor::arbor arbor::arborenv sonata)
