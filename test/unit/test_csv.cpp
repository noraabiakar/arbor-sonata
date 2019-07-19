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

    EXPECT_EQ(arb::cell_kind::cable, r.cell_kind(t0));
    EXPECT_EQ(arb::cell_kind::cable, r.cell_kind(t1));
    EXPECT_EQ(arb::cell_kind::spike_source, r.cell_kind(t2));

    auto m0 = r.morph(t0);
    auto m1 = r.morph(t1);

    EXPECT_EQ(m0, m1);
    EXPECT_TRUE(m0.has_soma());
    EXPECT_EQ(2, m0.components());

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

    auto soma_mechs = l0.at("soma");
    for (auto m: soma_mechs) {
        if (m.name() == "pas") {
            EXPECT_EQ(-65, m.get("e"));
            EXPECT_EQ(0, m.get("g"));
        } else if(m.name() == "hh") {
            EXPECT_EQ(-65, m.get("el"));
            EXPECT_EQ(0.001, m.get("gl"));
        }
    }

    auto dend_mechs = l0.at("dend");
    for (auto m: dend_mechs) {
        if (m.name() == "pas") {
            EXPECT_EQ(-70, m.get("e"));
            EXPECT_EQ(0.001, m.get("g"));
        } else if(m.name() == "hh") {
            FAIL() << "dendrite shouldn't have hh mechanism";
        }
    }

    soma_mechs = l1.at("soma");
    for (auto m: soma_mechs) {
        if (m.name() == "pas") {
            EXPECT_EQ(-65, m.get("e"));
            EXPECT_EQ(0, m.get("g"));
        } else if(m.name() == "hh") {
            EXPECT_EQ(-54, m.get("el"));
            EXPECT_EQ(0.003, m.get("gl"));
        }
    }

    dend_mechs = l1.at("dend");
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

    soma_mechs = l0.at("soma");
    for (auto m: soma_mechs) {
        if (m.name() == "pas") {
            EXPECT_EQ(-65, m.get("e"));
            EXPECT_EQ(0, m.get("g"));
        } else if(m.name() == "hh") {
            EXPECT_EQ(-65, m.get("el"));
            EXPECT_EQ(0.001, m.get("gl"));
        }
    }

    dend_mechs = l0.at("dend");
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

    type_pop_id t0({100, "pop_e_e"});
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

    EXPECT_EQ(p2.name(), p3.name());
    EXPECT_EQ(p2.values(), p3.values());

    EXPECT_EQ(p2.name(), p0.name());
    EXPECT_EQ(p2.values(), p0.values());

    EXPECT_EQ(0, p0.get("e"));
    EXPECT_EQ(2.0, p0.get("tau"));
}