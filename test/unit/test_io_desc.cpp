#include <arbor/cable_cell.hpp>

#include "hdf5_lib.hpp"
#include "csv_lib.hpp"
#include "data_management_lib.hpp"
#include "sonata_exceptions.hpp"

#include "../gtest.h"

// pop_ext         n5
//           ______|_______
//          |       _______|________
//          |      |       |        |
//          v      |       v        v
//  pop_e:  n0     n1      n2       n3
//          |      ^       |
//          |      |       |
//          |      |       |
//  pop_i:  |____> n4 <____|


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

    std::vector<probe_info> probe_vec;
    probe_vec.emplace_back("v", "pop_e", std::vector<unsigned>{0,2}, 0, 0.5, "file0");
    probe_vec.emplace_back("v", "pop_ext", std::vector<unsigned>{0}, 1, 0.2, "file0");
    probe_vec.emplace_back("i", "pop_e",std::vector<unsigned> {0,3}, 1, 0.1, "file1");
    probe_vec.emplace_back("i", "pop_i",   std::vector<unsigned>{0}, 0, 0.3, "file1");

    io_desc in(nodes, spike_vec, clamp_vec, probe_vec);

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
    auto loc = arb::mlocation{0,0.5};

    EXPECT_EQ(1, clamps.size());
    EXPECT_EQ(loc, clamps.front().stim_loc);
    EXPECT_EQ(0, clamps.front().delay);
    EXPECT_EQ(0.4, clamps.front().amplitude);
    EXPECT_EQ(5, clamps.front().duration);
}


TEST(io_desc, probes) {
    auto in = simple_input();

    EXPECT_EQ(2, in.get_num_probes(0));
    EXPECT_EQ(0, in.get_num_probes(1));
    EXPECT_EQ(1, in.get_num_probes(2));
    EXPECT_EQ(1, in.get_num_probes(3));
    EXPECT_EQ(1, in.get_num_probes(4));
    EXPECT_EQ(1, in.get_num_probes(5));

    auto probe_gps = in.get_probe_groups();
    EXPECT_EQ(2, probe_gps.size());
    EXPECT_TRUE(probe_gps.find("file0") != probe_gps.end());
    EXPECT_TRUE(probe_gps.find("file1") != probe_gps.end());

    EXPECT_EQ(3, probe_gps.at("file0").size());
    EXPECT_EQ(3, probe_gps.at("file1").size());

    EXPECT_EQ(cell_member_type({0, 0}), probe_gps.at("file0")[0]);
    EXPECT_EQ(cell_member_type({2, 0}), probe_gps.at("file0")[1]);
    EXPECT_EQ(cell_member_type({5, 0}), probe_gps.at("file0")[2]);

    EXPECT_EQ(cell_member_type({0, 1}), probe_gps.at("file1")[0]);
    EXPECT_EQ(cell_member_type({3, 0}), probe_gps.at("file1")[1]);
    EXPECT_EQ(cell_member_type({4, 0}), probe_gps.at("file1")[2]);

    EXPECT_THROW(in.get_probe({0,2}), sonata_exception);

//    auto p = in.get_probe(0);
//    auto add = arb::util::any_cast<arb::cable_probe_membrane_voltage>(p.address);
//    auto loc = arb::mlocation{0,0.5};
//    EXPECT_EQ(loc, add.locations.front());

//    p = in.get_probe(2);
//    add = arb::util::any_cast<arb::cable_probe_membrane_voltage>(p.address);
//    EXPECT_EQ(loc, add.locations.front());

//    p = in.get_probe(5);
//    add = arb::util::any_cast<arb::cable_probe_membrane_voltage>(p.address);
//    loc = arb::mlocation{1,0.2};
//    EXPECT_EQ(loc, add.locations.front());

//    p = in.get_probe(0);
//    add = arb::util::any_cast<arb::cable_probe_axial_current>(p.address);
//    loc = arb::mlocation{1,0.1};
//    EXPECT_EQ(loc, add.locations.front());

//    p = in.get_probe(3);
//    add = arb::util::any_cast<arb::cable_probe_axial_current>(p.address);
//    EXPECT_EQ(loc, add.locations.front());

//    p = in.get_probe(4);
//    add = arb::util::any_cast<arb::cable_probe_axial_current>(p.address);
//    loc = arb::mlocation{0,0.3};
//    EXPECT_EQ(loc, add.locations.front());
}