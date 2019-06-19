#include <arbor/cable_cell.hpp>

#include "hdf5_lib.hpp"
#include "sonata_exceptions.hpp"

#include "../gtest.h"

TEST(hdf5_record, verify_nodes) {
    std::string datadir{DATADIR};

    auto filename0 = datadir + "/nodes_0.h5";
    auto filename1 = datadir + "/nodes_1.h5";

    auto f0 = std::make_shared<h5_file>(filename0);
    auto f1 = std::make_shared<h5_file>(filename1);

    h5_record r({f0, f1});

    EXPECT_TRUE(r.verify_nodes());
    EXPECT_THROW(r.verify_edges(), sonata_exception);
}

TEST(hdf5_record, verify_nodes) {
    std::string datadir{DATADIR};

    auto filename = datadir + "/nodes_0.h5";
    auto f = std::make_shared<h5_file>(filename);

    h5_record r({f});

    EXPECT_TRUE(r.verify_nodes());
}
