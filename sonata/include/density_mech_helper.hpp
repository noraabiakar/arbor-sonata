#pragma once

#include <iostream>
#include <unordered_map>
#include <vector>

using variable_map = std::unordered_map<std::string, double>;

struct mech_params {
    std::string section;
    std::unordered_map<std::string, std::string> param_alias;
    arb::mechanism_desc mech;

    mech_params(std::string sec, std::unordered_map<std::string, std::string> map, arb::mechanism_desc full_mech) :
            section(sec), param_alias(std::move(map)), mech(full_mech) {}

    void print() {
        std::cout << section << std::endl;
        std::cout << mech.name() << std::endl;
        for (auto p: mech.values()) {
            std::cout << p.first << " = " << p.second << std::endl;
        }
        for (auto a: param_alias) {
            std::cout << a.first << " -> "<< a.second << std::endl;
        }
    }
};

struct mech_groups {
    variable_map variables;
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
};
