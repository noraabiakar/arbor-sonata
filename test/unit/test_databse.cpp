#include <arbor/cable_cell.hpp>

#include "hdf5_lib.hpp"
#include "csv_lib.hpp"
#include "data_management_lib.hpp"
#include "sonata_exceptions.hpp"

#include "../gtest.h"

//                  ________________
//                 |                v
//  pop_e:  n0     n1      n2       n3
//           |     ^       |
//           |     |       |
//           |     |       |
//  pop_i:   |___> n4 <____|

database simple_network() {
    std::string datadir{DATADIR};

    auto nodes0 = datadir + "/nodes_0.h5";
    auto nodes1 = datadir + "/nodes_1.h5";

    auto n0 = std::make_shared<h5_file>(nodes0);
    auto n1 = std::make_shared<h5_file>(nodes1);

    h5_record nodes({n0, n1});


    auto edges0 = datadir + "/edges_0.h5";
    auto edges1 = datadir + "/edges_1.h5";
    auto edges2 = datadir + "/edges_2.h5";

    auto e0 = std::make_shared<h5_file>(edges0);
    auto e1 = std::make_shared<h5_file>(edges1);
    auto e2 = std::make_shared<h5_file>(edges2);

    h5_record edges({e0, e1, e2});

    auto edges3 = datadir + "/edges.csv";
    auto ef = csv_file(edges3);
    auto e_base = csv_edge_record({ef});

    auto nodes2 = datadir + "/nodes.csv";
    auto nf = csv_file(nodes2);
    auto n_base = csv_node_record({nf});

    database db(nodes, edges, n_base, e_base, {}, {});

    return std::move(db);
}


TEST(database, helper_functions) {

    auto db = simple_network();

    EXPECT_EQ(5, db.num_cells());

    EXPECT_EQ(std::vector<unsigned>({0,4,5}), db.pop_partitions());
    EXPECT_EQ(std::vector<std::string>({"pop_e", "pop_i"}), db.pop_names());

    EXPECT_EQ("pop_e", db.population_of(0));
    EXPECT_EQ("pop_e", db.population_of(1));
    EXPECT_EQ("pop_e", db.population_of(2));
    EXPECT_EQ("pop_e", db.population_of(3));
    EXPECT_EQ("pop_i", db.population_of(4));

    EXPECT_EQ(0, db.population_id_of(0));
    EXPECT_EQ(1, db.population_id_of(1));
    EXPECT_EQ(2, db.population_id_of(2));
    EXPECT_EQ(3, db.population_id_of(3));
    EXPECT_EQ(0, db.population_id_of(4));
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

        EXPECT_EQ(0, tgts.size());

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

        srcs.clear();
        tgts.clear();
        db.get_sources_and_targets(2, srcs, tgts);

        EXPECT_EQ(1, srcs.size());
        EXPECT_EQ(3, srcs[0].segment);
        EXPECT_NEAR(0.2, srcs[0].position, 1e-5);

        EXPECT_EQ(0, tgts.size());

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

    auto decomp = arb::group_description(arb::cell_kind::cable, {0,1,2,3,4}, arb::backend_kind::multicore);
    verify_src_tgt({decomp});
}

TEST(database, connections) {
    auto db = simple_network();

    auto decomp = arb::group_description(arb::cell_kind::cable, {0,1,2,3,4}, arb::backend_kind::multicore);
    db.build_source_and_target_maps({decomp});

    std::vector<arb::cell_connection> conns;
    db.get_connections(0, conns);
    EXPECT_EQ(0, conns.size());

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
    EXPECT_EQ(0, conns.size());

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
    EXPECT_NEAR(0.04,conns[0].weight,1e-5);
    EXPECT_NEAR(0.3,conns[0].delay,1e-5);

    EXPECT_EQ(2,conns[1].source.gid);
    EXPECT_EQ(0,conns[1].source.index);
    EXPECT_EQ(4,conns[1].dest.gid);
    EXPECT_EQ(1,conns[1].dest.index);
    EXPECT_NEAR(0.04,conns[1].weight,1e-5);
    EXPECT_NEAR(0.3,conns[1].delay,1e-5);

}