#include <arbor/cable_cell.hpp>
#include "csv_lib.hpp"

#include "../gtest.h"

TEST(csv_file, constructor) {
    std::string datadir{DATADIR};

    {
        auto filename = datadir + "/dummy_0.csv";
        auto f = csv_file(filename);
        auto data = f.get_data();

        EXPECT_EQ(4, data.size());
        EXPECT_EQ(5, data.front().size());
    }

    {
        auto filename = datadir + "/nodes.csv";
        auto f = csv_file(filename);
        auto data = f.get_data();

        EXPECT_EQ(4, data.size());
        EXPECT_EQ(6, data.front().size());
    }
}

TEST(csv_record, constructor) {
    std::string datadir{DATADIR};
    auto filename = datadir + "/nodes.csv";
    auto f = csv_file(filename);
    auto r = csv_record({f});

    EXPECT_EQ()
}