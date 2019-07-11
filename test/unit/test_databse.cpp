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
//  pop_i:   |___> n5 <____|


TEST(database, helper_functions) {
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

    EXPECT_EQ(5, db.num_cells());
    EXPECT_EQ(4, db.num_edges());

    EXPECT_EQ(std::vector<unsigned>({0,4,5}), db.pop_partitions());
    EXPECT_EQ(std::vector<std::string>({"pop_e", "pop_i"}), db.pop_names());

    EXPECT_EQ("pop_e", db.population_of(0));
    EXPECT_EQ("pop_e", db.population_of(1));
    EXPECT_EQ("pop_e", db.population_of(2));
    EXPECT_EQ("pop_e", db.population_of(3));
    EXPECT_EQ("pop_i", db.population_of(4));

//    auto l_cell = db.localize_cell(0);
//    EXPECT_EQ(0, l_cell.pop_id);
//    EXPECT_EQ(0, l_cell.el_id);
//
//    l_cell = db.localize_cell(4);
//    EXPECT_EQ(1, l_cell.pop_id);
//    EXPECT_EQ(0, l_cell.el_id);
}