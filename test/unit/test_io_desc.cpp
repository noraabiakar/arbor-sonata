#include <arbor/cable_cell.hpp>

#include "hdf5_lib.hpp"
#include "csv_lib.hpp"
#include "data_management_lib.hpp"
#include "sonata_exceptions.hpp"

#include "../gtest.h"

io_desc simple_input() {
    std::string datadir{DATADIR};

    auto nodes0 = datadir + "/nodes_0.h5";
    auto nodes1 = datadir + "/nodes_1.h5";
    auto nodes2 = datadir + "/nodes_2.h5";

    auto n0 = std::make_shared<h5_file>(nodes0);
    auto n1 = std::make_shared<h5_file>(nodes1);
    auto n2 = std::make_shared<h5_file>(nodes2);

    h5_record nodes({n0, n1, n2});

    auto spikes0 = datadir + "/spikes_0.h5";
    auto spikes1 = datadir + "/spikes_1.h5";

    h5_wrapper spike_gp0(h5_file(spikes0).top_group_);
    h5_wrapper spike_gp1(h5_file(spikes1).top_group_);

    std::vector<spike_in_info> spike_vec = {{spike_gp0, "pop_e"}, {spike_gp1, "pop_i"}};

    auto clamp_input = datadir + "/clamp_input.csv";
    auto clamp_electrode = datadir + "/clamp_electrode.csv";

    csv_file clamp_in(clamp_input);
    csv_file clamp_elec(clamp_electrode);

    std::vector<current_clamp_info> clamp_vec = {{clamp_elec, clamp_in}};

    io_desc in(nodes, spike_vec, clamp_vec, {});

    return std::move(in);
}

TEST(io_desc, spikes) {
    auto in = simple_input();

    for (unsigned i = 0; i < 4; i++) {
        auto spikes = in.get_spikes(i);

        std::vector<double> expected;
        for (auto j = 0; j < 5; j++) {
            expected.push_back(j*15 + i*15);
        }

        EXPECT_EQ(expected, spikes);
    }
    {
        auto spikes = in.get_spikes(4);
        std::vector<double> expected;
        expected.push_back(37);
        expected.push_back(54);

        EXPECT_EQ(expected, spikes);
    }
    {
        auto spikes = in.get_spikes(5);
        EXPECT_TRUE(spikes.empty());
    }
}

TEST(io_desc, clamps) {
    auto in = simple_input();

    auto clamps = in.get_current_clamps(0);
    EXPECT_EQ(1, clamps.size());
    EXPECT_EQ(arb::segment_location({0,0.5}), clamps.front().stim_loc);
    EXPECT_EQ(0, clamps.front().delay);
    EXPECT_EQ(0.4, clamps.front().amplitude);
    EXPECT_EQ(5, clamps.front().duration);
}