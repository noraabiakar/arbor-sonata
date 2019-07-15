#include <iostream>

#include <arbor/version.hpp>

#ifdef ARB_MPI_ENABLED
#include <mpi.h>
#include <arborenv/with_mpi.hpp>
#endif

#include "../gtest.h"

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

#ifdef ARB_MPI_ENABLED
    arbenv::with_mpi guard(argc, argv, false);
#endif
    return RUN_ALL_TESTS();

}
