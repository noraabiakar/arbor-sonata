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

database simple_network() {
    std::string datadir{DATADIR};

    auto nodes0 = datadir + "/nodes_0.h5";
    auto nodes1 = datadir + "/nodes_1.h5";
    auto nodes2 = datadir + "/nodes_2.h5";
    auto spikes0 = datadir + "/spikes_0.h5";
    auto spikes1 = datadir + "/spikes_1.h5";
    auto clamp_input = datadir + "/clamp_input.csv";
    auto clamp_electrode = datadir + "/clamp_electrode.csv";

    auto n0 = std::make_shared<h5_file>(nodes0);
    auto n1 = std::make_shared<h5_file>(nodes1);
    auto n2 = std::make_shared<h5_file>(nodes2);

    h5_record nodes({n0, n1, n2});


    auto edges0 = datadir + "/edges_0.h5";
    auto edges1 = datadir + "/edges_1.h5";
    auto edges2 = datadir + "/edges_2.h5";
    auto edges3 = datadir + "/edges_3.h5";

    auto e0 = std::make_shared<h5_file>(edges0);
    auto e1 = std::make_shared<h5_file>(edges1);
    auto e2 = std::make_shared<h5_file>(edges2);
    auto e3 = std::make_shared<h5_file>(edges3);

    h5_record edges({e0, e1, e2, e3});

    auto edges4 = datadir + "/edges.csv";
    auto ef = csv_file(edges4);
    auto e_base = csv_edge_record({ef});

    auto nodes3 = datadir + "/nodes.csv";
    auto nf = csv_file(nodes3);
    auto n_base = csv_node_record({nf});

    h5_wrapper spike_gp0(h5_file(spikes0).top_group_);
    h5_wrapper spike_gp1(h5_file(spikes1).top_group_);

    std::vector<spike_info> spike_vec = {{spike_gp0, "pop_e"}, {spike_gp1, "pop_i"}};

    csv_file clamp_in(clamp_input);
    csv_file clamp_elec(clamp_electrode);

    std::vector<current_clamp_info> clamp_vec = {{clamp_elec, clamp_in}};

    database db(nodes, edges, n_base, e_base, spike_vec, clamp_vec);

    return std::move(db);
}


TEST(database, helper_functions) {

    auto db = simple_network();

    EXPECT_EQ(6, db.num_cells());

    EXPECT_EQ(std::vector<unsigned>({0,4,5,6}), db.pop_partitions());
    EXPECT_EQ(std::vector<std::string>({"pop_e", "pop_i" ,"pop_ext"}), db.pop_names());

    EXPECT_EQ("pop_e", db.population_of(0));
    EXPECT_EQ("pop_e", db.population_of(1));
    EXPECT_EQ("pop_e", db.population_of(2));
    EXPECT_EQ("pop_e", db.population_of(3));
    EXPECT_EQ("pop_i", db.population_of(4));
    EXPECT_EQ("pop_ext", db.population_of(5));

    EXPECT_EQ(0, db.population_id_of(0));
    EXPECT_EQ(1, db.population_id_of(1));
    EXPECT_EQ(2, db.population_id_of(2));
    EXPECT_EQ(3, db.population_id_of(3));
    EXPECT_EQ(0, db.population_id_of(4));
    EXPECT_EQ(0, db.population_id_of(5));
}

TEST(database, source_target_maps) {
    auto db = simple_network();

    auto verify_src_tgt = [&db](const std::vector<arb::group_description>& decomp) {
        db.build_source_and_target_maps(decomp);

        std::vector<segment_location> srcs;
        std::vector<std::pair<segment_location, arb::mechanism_desc>> tgts;
        db.get_sources_and_targets(0, srcs, tgts);

        EXPECT_EQ(1, srcs.size());
        EXPECT_EQ(1, srcs[0].segment);
        EXPECT_NEAR(0.3, srcs[0].position, 1e-5);

        EXPECT_EQ(1, tgts.size());
        EXPECT_EQ(0, tgts[0].first.segment);
        EXPECT_NEAR(0, tgts[0].first.position, 1e-5);
        EXPECT_EQ("expsyn", tgts[0].second.name());

        srcs.clear();
        tgts.clear();
        db.get_sources_and_targets(1, srcs, tgts);

        EXPECT_EQ(1, srcs.size());
        EXPECT_EQ(1, srcs[0].segment);
        EXPECT_NEAR(0.2, srcs[0].position, 1e-5);

        EXPECT_EQ(1, tgts.size());
        EXPECT_EQ(0, tgts[0].first.segment);
        EXPECT_NEAR(0.5, tgts[0].first.position, 1e-5);
        EXPECT_EQ("expsyn", tgts[0].second.name());
        EXPECT_NEAR(0.51, tgts[0].second.values().at("e"), 1e-5);


        srcs.clear();
        tgts.clear();
        db.get_sources_and_targets(2, srcs, tgts);

        EXPECT_EQ(1, srcs.size());
        EXPECT_EQ(3, srcs[0].segment);
        EXPECT_NEAR(0.2, srcs[0].position, 1e-5);

        EXPECT_EQ(1, tgts.size());
        EXPECT_EQ(0, tgts[0].first.segment);
        EXPECT_NEAR(0, tgts[0].first.position, 1e-5);
        EXPECT_EQ("expsyn", tgts[0].second.name());

        srcs.clear();
        tgts.clear();
        db.get_sources_and_targets(3, srcs, tgts);

        EXPECT_EQ(0, srcs.size());

        EXPECT_EQ(1, tgts.size());
        EXPECT_EQ(5, tgts[0].first.segment);
        EXPECT_NEAR(0.6, tgts[0].first.position, 1e-5);
        EXPECT_EQ("expsyn", tgts[0].second.name());

        srcs.clear();
        tgts.clear();
        db.get_sources_and_targets(4, srcs, tgts);

        EXPECT_EQ(1, srcs.size());
        EXPECT_EQ(0, srcs[0].segment);
        EXPECT_NEAR(0.9, srcs[0].position, 1e-5);

        EXPECT_EQ(2, tgts.size());
        EXPECT_EQ(0, tgts[0].first.segment);
        EXPECT_NEAR(0.4, tgts[0].first.position, 1e-5);
        EXPECT_EQ("exp2syn", tgts[0].second.name());
        EXPECT_EQ(2, tgts[1].first.segment);
        EXPECT_NEAR(0.1, tgts[1].first.position, 1e-5);
        EXPECT_EQ("exp2syn", tgts[1].second.name());
    };

    auto decomp = arb::group_description(arb::cell_kind::cable, {0,1,2,3,4,5}, arb::backend_kind::multicore);
    verify_src_tgt({decomp});
}

TEST(database, connections) {
    auto db = simple_network();

    auto decomp = arb::group_description(arb::cell_kind::cable, {0,1,2,3,4,5}, arb::backend_kind::multicore);
    db.build_source_and_target_maps({decomp});

    std::vector<arb::cell_connection> conns;
    db.get_connections(0, conns);
    EXPECT_EQ(1, conns.size());
    EXPECT_EQ(5,conns[0].source.gid);
    EXPECT_EQ(0,conns[0].source.index);
    EXPECT_EQ(0,conns[0].dest.gid);
    EXPECT_EQ(0,conns[0].dest.index);
    EXPECT_NEAR(0.01,conns[0].weight,1e-5);
    EXPECT_NEAR(0.1,conns[0].delay,1e-5);

    conns.clear();
    db.get_connections(1, conns);
    EXPECT_EQ(1, conns.size());
    EXPECT_EQ(4,conns[0].source.gid);
    EXPECT_EQ(0,conns[0].source.index);
    EXPECT_EQ(1,conns[0].dest.gid);
    EXPECT_EQ(0,conns[0].dest.index);
    EXPECT_NEAR(-0.02,conns[0].weight,1e-5);
    EXPECT_NEAR(0.1,conns[0].delay,1e-5);

    conns.clear();
    db.get_connections(2, conns);
    EXPECT_EQ(1, conns.size());
    EXPECT_EQ(5,conns[0].source.gid);
    EXPECT_EQ(0,conns[0].source.index);
    EXPECT_EQ(2,conns[0].dest.gid);
    EXPECT_EQ(0,conns[0].dest.index);
    EXPECT_NEAR(0.01,conns[0].weight,1e-5);
    EXPECT_NEAR(0.1,conns[0].delay,1e-5);

    conns.clear();
    db.get_connections(3, conns);
    EXPECT_EQ(1, conns.size());
    EXPECT_EQ(1,conns[0].source.gid);
    EXPECT_EQ(0,conns[0].source.index);
    EXPECT_EQ(3,conns[0].dest.gid);
    EXPECT_EQ(0,conns[0].dest.index);
    EXPECT_NEAR(0.05,conns[0].weight,1e-5);
    EXPECT_NEAR(0.2,conns[0].delay,1e-5);

    conns.clear();
    db.get_connections(4, conns);
    EXPECT_EQ(2, conns.size());
    EXPECT_EQ(0,conns[0].source.gid);
    EXPECT_EQ(0,conns[0].source.index);
    EXPECT_EQ(4,conns[0].dest.gid);
    EXPECT_EQ(0,conns[0].dest.index);
    EXPECT_NEAR(0.0235,conns[0].weight,1e-5);
    EXPECT_NEAR(0.3,conns[0].delay,1e-5);

    EXPECT_EQ(2,conns[1].source.gid);
    EXPECT_EQ(0,conns[1].source.index);
    EXPECT_EQ(4,conns[1].dest.gid);
    EXPECT_EQ(1,conns[1].dest.index);
    EXPECT_NEAR(0.04,conns[1].weight,1e-5);
    EXPECT_NEAR(0.3,conns[1].delay,1e-5);

}

TEST(database, morphologies) {
    auto db = simple_network();

    for (auto i = 0; i < 4; i++) {
        auto morph = db.get_cell_morphology(i);
        EXPECT_TRUE(morph.has_soma());
        EXPECT_DOUBLE_EQ(6.30785, morph.soma.r);
        EXPECT_EQ(1, morph.sections.size());
    }

    auto morph = db.get_cell_morphology(4);
    EXPECT_TRUE(morph.has_soma());
    EXPECT_DOUBLE_EQ(3.5, morph.soma.r);
    EXPECT_EQ(0, morph.sections.size());

    EXPECT_THROW(db.get_cell_morphology(5), sonata_exception);
}

TEST(database, cell_kinds) {
    auto db = simple_network();

    for (unsigned i = 0; i < 5; i++) {
        auto kind = db.get_cell_kind(i);
        EXPECT_EQ(arb::cell_kind::cable, kind);
    }
    auto kind = db.get_cell_kind(5);
    EXPECT_EQ(arb::cell_kind::spike_source, kind);
}

TEST(database, density_mechs) {
    auto db = simple_network();

    for (unsigned i = 0; i < 5; i++) {
        auto mechs_per_sec = db.get_density_mechs(i);
        EXPECT_EQ(2, mechs_per_sec.size());
        EXPECT_TRUE(mechs_per_sec.find("soma") != mechs_per_sec.end());
        EXPECT_TRUE(mechs_per_sec.find("dend") != mechs_per_sec.end());

        auto mech_soma = mechs_per_sec.at("soma");
        auto mech_dend = mechs_per_sec.at("dend");

        EXPECT_EQ(2, mech_soma.size());
        EXPECT_EQ(1, mech_dend.size());

        EXPECT_EQ("hh", mech_soma[0].name());
        EXPECT_EQ("pas", mech_soma[1].name());

        EXPECT_EQ("pas", mech_dend[0].name());

        if (i < 4) {
            EXPECT_NEAR(0.0003, mech_soma[0].values().at("gl"), 1e-5);
            EXPECT_NEAR(-54.3, mech_soma[0].values().at("el"), 1e-5);
            EXPECT_NEAR(0, mech_soma[1].values().at("g"), 1e-5);
            EXPECT_NEAR(-65, mech_soma[1].values().at("e"), 1e-5);

            EXPECT_NEAR(0.001, mech_dend[0].values().at("g"), 1e-5);
            EXPECT_NEAR(-65.1, mech_dend[0].values().at("e"), 1e-5);
        } else {
            EXPECT_NEAR(0.003, mech_soma[0].values().at("gl"), 1e-5);
            EXPECT_NEAR(-54, mech_soma[0].values().at("el"), 1e-5);
            EXPECT_NEAR(0, mech_soma[1].values().at("g"), 1e-5);
            EXPECT_NEAR(-65, mech_soma[1].values().at("e"), 1e-5);

            EXPECT_NEAR(0.001, mech_dend[0].values().at("g"), 1e-5);
            EXPECT_NEAR(-65, mech_dend[0].values().at("e"), 1e-5);
        }
    }

    auto mechs_per_sec = db.get_density_mechs(5);
    EXPECT_EQ(0, mechs_per_sec.size());
}

TEST(database, spikes) {
    auto db = simple_network();

    for (unsigned i = 0; i < 4; i++) {
        auto spikes = db.get_spikes(i);

        std::vector<double> expected;
        for (auto j = 0; j < 5; j++) {
            expected.push_back(j*15 + i*15);
        }

        EXPECT_EQ(expected, spikes);
    }
    {
        auto spikes = db.get_spikes(4);
        std::vector<double> expected;
        expected.push_back(37);
        expected.push_back(54);

        EXPECT_EQ(expected, spikes);
    }
    {
        auto spikes = db.get_spikes(5);
        EXPECT_TRUE(spikes.empty());
    }
}

TEST(database, clamps) {
    auto db = simple_network();

    auto clamps = db.get_current_clamps(0);
    EXPECT_EQ(1, clamps.size());
    EXPECT_EQ(arb::segment_location({0,0.5}), clamps.front().stim_loc);
    EXPECT_EQ(0, clamps.front().delay);
    EXPECT_EQ(0.4, clamps.front().amplitude);
    EXPECT_EQ(5, clamps.front().duration);
}