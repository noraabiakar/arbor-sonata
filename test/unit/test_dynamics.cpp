#include "../gtest.h"

#include "dynamics_params_helper.hpp"

TEST(dynamics_params_helper, point_mech) {
    std::string datadir{DATADIR};
    std::string expsyn_file = datadir + "/expsyn.json";
    std::string exp2syn_file = datadir + "/exp2syn.json";

    auto expsyn = read_dynamics_params_point(expsyn_file);

    EXPECT_EQ("expsyn", expsyn.name());
    EXPECT_EQ(0, expsyn.values().at("e"));
    EXPECT_EQ(2.0, expsyn.values().at("tau"));

    auto exp2syn = read_dynamics_params_point(exp2syn_file);

    EXPECT_EQ("exp2syn", exp2syn.name());
    EXPECT_EQ(0.1, exp2syn.values().at("e"));
    EXPECT_EQ(0.5, exp2syn.values().at("tau1"));
    EXPECT_EQ(0.6, exp2syn.values().at("tau2"));
}

TEST(dynamics_params_helper, density_mech) {
    std::string datadir{DATADIR};
    std::string pas_hh_file = datadir + "/set_pas.json";

    auto pas_hh = read_dynamics_params_density_base(pas_hh_file);

    EXPECT_EQ(2, pas_hh.size());
    EXPECT_TRUE(pas_hh.find("pas_0") != pas_hh.end());
    EXPECT_TRUE(pas_hh.find("hh_0") != pas_hh.end());

    auto pas_0_group = pas_hh.at("pas_0");
    EXPECT_EQ(2, pas_0_group.variables.size());

    EXPECT_TRUE(pas_0_group.variables.find("e_pas") !=  pas_0_group.variables.end());
    EXPECT_TRUE(pas_0_group.variables.find("g_pas") !=  pas_0_group.variables.end());

    EXPECT_EQ(0.002, pas_0_group.variables.at("g_pas"));
    EXPECT_EQ(-70, pas_0_group.variables.at("e_pas"));

    auto pas_0_details = pas_0_group.mech_details;
    EXPECT_EQ(2, pas_0_details.size());

    EXPECT_EQ(arb::section_kind::soma, pas_0_details[0].section);
    EXPECT_EQ(0, pas_0_details[0].param_alias.size());
    EXPECT_EQ("pas", pas_0_details[0].mech.name());
    EXPECT_EQ(2, pas_0_details[0].mech.values().size());
    EXPECT_EQ(-65, pas_0_details[0].mech.values().at("e"));
    EXPECT_EQ(0, pas_0_details[0].mech.values().at("g"));

    EXPECT_EQ(arb::section_kind::dendrite, pas_0_details[1].section);
    EXPECT_EQ(1, pas_0_details[1].param_alias.size());
    EXPECT_TRUE(pas_0_details[1].param_alias.find("e") != pas_0_details[1].param_alias.end());
    EXPECT_EQ("e_pas", pas_0_details[1].param_alias.at("e"));
    EXPECT_EQ("pas", pas_0_details[1].mech.name());
    EXPECT_EQ(1, pas_0_details[1].mech.values().size());
    EXPECT_EQ(0.001, pas_0_details[1].mech.values().at("g"));

    auto hh_0_group = pas_hh.at("hh_0");
    EXPECT_EQ(2, hh_0_group.variables.size());

    EXPECT_TRUE(hh_0_group.variables.find("el_hh") !=  hh_0_group.variables.end());
    EXPECT_TRUE(hh_0_group.variables.find("gl_hh") !=  hh_0_group.variables.end());

    EXPECT_EQ(0.002, hh_0_group.variables.at("gl_hh"));
    EXPECT_EQ(-54, hh_0_group.variables.at("el_hh"));

    auto hh_0_details = hh_0_group.mech_details;
    EXPECT_EQ(1, hh_0_details.size());

    EXPECT_EQ(arb::section_kind::soma, hh_0_details[0].section);
    EXPECT_EQ(2, hh_0_details[0].param_alias.size());
    EXPECT_TRUE(hh_0_details[0].param_alias.find("el") != hh_0_details[0].param_alias.end());
    EXPECT_EQ("el_hh", hh_0_details[0].param_alias.at("el"));
    EXPECT_TRUE(hh_0_details[0].param_alias.find("gl") != hh_0_details[0].param_alias.end());
    EXPECT_EQ("gl_hh", hh_0_details[0].param_alias.at("gl"));
    EXPECT_EQ("hh", hh_0_details[0].mech.name());
    EXPECT_EQ(0, hh_0_details[0].mech.values().size());
}

TEST(dynamics_params_helper, override_density_mech) {
    std::string datadir{DATADIR};
    std::string pas_file = datadir + "/pas.json";
    std::string pas_hh_file = datadir + "/pas_hh.json";

    auto pas_override = read_dynamics_params_density_override(pas_file);
    auto pas_hh_override = read_dynamics_params_density_override(pas_hh_file);

    EXPECT_EQ(1, pas_override.size());
    EXPECT_TRUE(pas_override.find("pas_0") != pas_override.end());
    EXPECT_EQ(1, pas_override.at("pas_0").size());
    EXPECT_EQ(0.001, pas_override.at("pas_0").at("g_pas"));

    EXPECT_EQ(2, pas_hh_override.size());
    EXPECT_TRUE(pas_hh_override.find("pas_0") != pas_hh_override.end());
    EXPECT_EQ(2, pas_hh_override.at("pas_0").size());
    EXPECT_EQ(0.001, pas_hh_override.at("pas_0").at("g_pas"));
    EXPECT_EQ(-65, pas_hh_override.at("pas_0").at("e_pas"));

    EXPECT_TRUE(pas_hh_override.find("hh_0") != pas_hh_override.end());
    EXPECT_EQ(1, pas_hh_override.at("hh_0").size());
    EXPECT_EQ(0.003, pas_hh_override.at("hh_0").at("gl_hh"));
}