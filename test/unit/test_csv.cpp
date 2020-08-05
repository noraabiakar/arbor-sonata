#include <arbor/cable_cell.hpp>

#include "csv_lib.hpp"
#include "sonata_exceptions.hpp"

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

TEST(csv_node_record, constructor) {
    std::string datadir{DATADIR};
    auto filename = datadir + "/nodes.csv";
    auto f = csv_file(filename);
    auto r = csv_node_record({f});

    type_pop_id t0({100, "pop_e"});
    type_pop_id t1({101, "pop_i"});
    type_pop_id t2({200, "pop_ext"});

    auto ids = r.unique_ids();
    EXPECT_EQ(3, ids.size());
    EXPECT_EQ(t0, ids[0]);
    EXPECT_EQ(t1, ids[1]);
    EXPECT_EQ(t2, ids[2]);

    EXPECT_EQ(arb::cell_kind::cable, r.cell_kind(t0));
    EXPECT_EQ(arb::cell_kind::cable, r.cell_kind(t1));
    EXPECT_EQ(arb::cell_kind::spike_source, r.cell_kind(t2));

    auto m0 = r.morph(t0);
    auto m1 = r.morph(t1);

    EXPECT_EQ(m0.num_branches(), m1.num_branches());

    EXPECT_EQ(1, m0.num_branches());

    EXPECT_THROW(r.morph(t2), sonata_exception);

    auto d0 = r.dynamic_params(t0);
    auto d1 = r.dynamic_params(t1);
    auto d2 = r.dynamic_params(t2);

    EXPECT_EQ(0, d2.size());

    EXPECT_EQ(0.001, d0["pas_0"]["g_pas"]);
    EXPECT_EQ(-70,   d0["pas_0"]["e_pas"]);
    EXPECT_EQ(0.001, d0["hh_0"]["gl_hh"]);
    EXPECT_EQ(-65,   d0["hh_0"]["el_hh"]);

    EXPECT_EQ(0.001, d1["pas_0"]["g_pas"]);
    EXPECT_EQ(-65,   d1["pas_0"]["e_pas"]);
    EXPECT_EQ(0.003, d1["hh_0"]["gl_hh"]);
    EXPECT_EQ(-54,   d1["hh_0"]["el_hh"]);

    auto l0 = r.density_mech_desc(t0);
    auto l1 = r.density_mech_desc(t1);
    auto l2 = r.density_mech_desc(t2);

    EXPECT_EQ(0, l2.size());

    auto soma_mechs = l0.at(section_kind::soma);
    for (auto m: soma_mechs) {
        if (m.name() == "pas") {
            EXPECT_EQ(-65, m.get("e"));
            EXPECT_EQ(0, m.get("g"));
        } else if(m.name() == "hh") {
            EXPECT_EQ(-65, m.get("el"));
            EXPECT_EQ(0.001, m.get("gl"));
        }
    }

    auto dend_mechs = l0.at(section_kind::dend);
    for (auto m: dend_mechs) {
        if (m.name() == "pas") {
            EXPECT_EQ(-70, m.get("e"));
            EXPECT_EQ(0.001, m.get("g"));
        } else if(m.name() == "hh") {
            FAIL() << "dendrite shouldn't have hh mechanism";
        }
    }

    soma_mechs = l1.at(section_kind::soma);
    for (auto m: soma_mechs) {
        if (m.name() == "pas") {
            EXPECT_EQ(-65, m.get("e"));
            EXPECT_EQ(0, m.get("g"));
        } else if(m.name() == "hh") {
            EXPECT_EQ(-54, m.get("el"));
            EXPECT_EQ(0.003, m.get("gl"));
        }
    }

    dend_mechs = l1.at(section_kind::dend);
    for (auto m: dend_mechs) {
        if (m.name() == "pas") {
            EXPECT_EQ(-65, m.get("e"));
            EXPECT_EQ(0.001, m.get("g"));
        } else if(m.name() == "hh") {
            FAIL() << "dendrite shouldn't have hh mechanism";
        }
    }

    std::unordered_map<std::string, variable_map> overrides;
    overrides["pas_0"]["e_pas"] = -80;

    r.override_density_params(t0, overrides);
    l0 = r.density_mech_desc(t0);

    soma_mechs = l0.at(section_kind::soma);
    for (auto m: soma_mechs) {
        if (m.name() == "pas") {
            EXPECT_EQ(-65, m.get("e"));
            EXPECT_EQ(0, m.get("g"));
        } else if(m.name() == "hh") {
            EXPECT_EQ(-65, m.get("el"));
            EXPECT_EQ(0.001, m.get("gl"));
        }
    }

    dend_mechs = l0.at(section_kind::dend);
    for (auto m: dend_mechs) {
        if (m.name() == "pas") {
            EXPECT_EQ(-80, m.get("e"));
            EXPECT_EQ(0.001, m.get("g"));
        } else if(m.name() == "hh") {
            FAIL() << "dendrite shouldn't have hh mechanism";
        }
    }

}

TEST(csv_edge_record, constructor) {
    std::string datadir{DATADIR};
    auto filename = datadir + "/edges.csv";
    auto f = csv_file(filename);
    auto r = csv_edge_record({f});

    type_pop_id t0({105, "pop_e_e"});
    type_pop_id t1({101, "pop_i_e"});
    type_pop_id t2({102, "pop_i_i"});
    type_pop_id t3({103, "pop_e_i"});
    type_pop_id t4({200, "pop_ext_e"});

    auto p0 = r.point_mech_desc(t0);
    auto p1 = r.point_mech_desc(t1);
    auto p2 = r.point_mech_desc(t2);
    auto p3 = r.point_mech_desc(t3);
    auto p4 = r.point_mech_desc(t4);

    EXPECT_EQ(p0.name(), p1.name());
    EXPECT_EQ(p0.values(), p1.values());

    EXPECT_EQ(p0.name(), p4.name());
    EXPECT_EQ(0, p4.values().size());

    EXPECT_EQ(p2.name(), p3.name());
    EXPECT_EQ(p2.values(), p3.values());

    EXPECT_EQ(0, p0.get("e"));
    EXPECT_EQ(2.0, p0.get("tau"));

    EXPECT_EQ(0.1, p2.get("e"));
    EXPECT_EQ(0.5, p2.get("tau1"));
    EXPECT_EQ(0.6, p2.get("tau2"));


    std::vector<std::pair<std::string, std::string>> expected_e2s_popi= {{"pop_e_i", "pop_e"}, {"pop_i_i", "pop_i"}};
    std::vector<std::pair<std::string, std::string>> expected_e2s_pope= {{"pop_e_e", "pop_e"}, {"pop_i_e", "pop_i"}, {"pop_ext_e","pop_ext"}};

    auto e2s_popi = r.edge_to_source_of_target("pop_i");
    EXPECT_EQ(2, e2s_popi.size());

    auto e2s_pope = r.edge_to_source_of_target("pop_e");
    EXPECT_EQ(3, e2s_pope.size());

    auto e2s_popext = r.edge_to_source_of_target("pop_ext");
    EXPECT_EQ(0, e2s_popext.size());

    for (auto i : e2s_pope) {
        bool found = 0;
        for (auto j: expected_e2s_pope) {
            if (i == j) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found);
    }

    for (auto i : e2s_popi) {
        bool found = 0;
        for (auto j: expected_e2s_popi) {
            if (i == j) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found);
    }

    std::unordered_set<std::string> expected_s_pope  = {{"pop_e_e"}, {"pop_e_i"}};
    std::unordered_set<std::string> expected_s_popi  = {{"pop_i_e"}, {"pop_i_i"}};
    std::unordered_set<std::string> expected_s_popext  = {{"pop_ext_e"}};

    auto s_popi = r.edges_of_source("pop_i");
    EXPECT_EQ(2, s_popi.size());

    auto s_pope = r.edges_of_source("pop_e");
    EXPECT_EQ(2, s_pope.size());

    auto s_popext = r.edges_of_source("pop_ext");
    EXPECT_EQ(1, s_popext.size());

    for (auto i : s_pope) {
        bool found = 0;
        for (auto j: expected_s_pope) {
            if (i == j) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found);
    }

    for (auto i : s_popi) {
        bool found = 0;
        for (auto j: expected_s_popi) {
            if (i == j) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found);
    }

    for (auto i : s_popext) {
        bool found = 0;
        for (auto j: expected_s_popext) {
            if (i == j) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found);
    }

    std::unordered_set<std::string> expected_t_pope  = {{"pop_e_e"}, {"pop_i_e"}, {"pop_ext_e"}};
    std::unordered_set<std::string> expected_t_popi  = {{"pop_e_i"}, {"pop_i_i"}};

    auto t_popi = r.edges_of_target("pop_i");
    EXPECT_EQ(2, t_popi.size());

    auto t_pope = r.edges_of_target("pop_e");
    EXPECT_EQ(3, t_pope.size());

    auto t_popext = r.edges_of_target("pop_ext");
    EXPECT_EQ(0, t_popext.size());

    for (auto i : t_pope) {
        bool found = 0;
        for (auto j: expected_t_pope) {
            if (i == j) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found);
    }

    for (auto i : t_popi) {
        bool found = 0;
        for (auto j: expected_t_popi) {
            if (i == j) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found);
    }
}
