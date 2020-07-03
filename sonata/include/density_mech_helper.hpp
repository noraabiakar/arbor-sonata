#pragma once

#include <iostream>
#include <unordered_map>
#include <vector>

#include <arbor/morph/morphology.hpp>
#include <arbor/cable_cell_param.hpp>

enum section_kind {
    soma,
    dend,
    axon,
    none
};

using variable_map = std::unordered_map<std::string, double>;

// mech_param is a descrtiption of a mechanism `mech` and related attributes:
// `section` is the name of the sections where `mech` is located (eg. soma, dend etc);
// some of the parameters of `mech` aren't set yet because they aren't assigned a numerical value
// instead, the parameters are assigned aliases, which they should get their numerical values from
// these alieases are stored in `param_alias`
struct mech_params {
    section_kind section;
    std::unordered_map<std::string, std::string> param_alias;     // Map from mechansim parameter to alias
    arb::mechanism_desc mech;

    mech_params(std::string sec, std::unordered_map<std::string, std::string> map, arb::mechanism_desc full_mech) :
            param_alias(std::move(map)), mech(full_mech) {
        if (sec == "soma") {
            section = section_kind::soma;
        } else if (sec == "dend") {
            section = section_kind::dend;
        } else if (sec == "axon") {
            section = section_kind::axon;
        } else {
            section = section_kind::none;
        }
    }

    void print() {
        switch (section) {
            case section_kind::soma: std::cout << "soma" << std::endl; break;
            case section_kind::dend: std::cout << "dend" << std::endl; break;
            case section_kind::axon: std::cout << "axon" << std::endl; break;
            default: std::cout << "dend" << std::endl;
        }
        std::cout << mech.name() << std::endl;
        for (auto p: mech.values()) {
            std::cout << p.first << " = " << p.second << std::endl;
        }
        for (auto a: param_alias) {
            std::cout << a.first << " -> "<< a.second << std::endl;
        }
    }
};

// A mech_group is a group of mechanisms that share `variables` that they can access.
// A mech_group has a list of mechanism descriptions `mech_params` that have mechanisms with
// parameters that can potenitally get their values from the `variables` map
struct mech_groups {
    variable_map variables; // variables that can be overwritten
    std::vector<mech_params> mech_details;

    mech_groups(std::unordered_map<std::string, double> vars, std::vector<mech_params> mech_det) :
            variables(std::move(vars)), mech_details(std::move(mech_det)) {}

    void print() {
        for (auto i: variables) {
            std::cout << i.first << " " << i.second << std::endl;
        }
        for (auto p: mech_details) {
            std::cout << "--------" << std::endl;
            p.print();
            std::cout << "--------" << std::endl;
        }
    }

    void apply_variables() {
        for (auto& det: mech_details) {
            auto& mech_desc = det.mech;
            // Get the aliases and set values from overrides
            for (auto alias: det.param_alias) {
                if (variables.find(alias.second) != variables.end()) {
                    mech_desc.set(alias.first, variables.at(alias.second));
                }
            }
        }
    }
};
