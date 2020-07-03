#include <iostream>
#include <fstream>
#include <unordered_map>

#include "include/json/json_params.hpp"
#include "include/sonata_exceptions.hpp"
#include "include/density_mech_helper.hpp"

arb::mechanism_desc read_dynamics_params_point(std::string fname) {
    std::ifstream f(fname);

    if (!f.good()) {
        throw std::runtime_error("Unable to open input parameter file: "+fname);
    }
    nlohmann::json json;
    json << f;

    auto mechs = json.get<std::unordered_map<std::string, nlohmann::json>>();

    if (mechs.size() > 1) {
        throw sonata_file_exception("Point mechanism description must contains only one mechanism, file: ", fname);
    }

    std::string mech_name = mechs.begin()->first;
    nlohmann::json mech_params = mechs.begin()->second;

    auto syn = arb::mechanism_desc(mech_name);
    auto params = mech_params.get<std::unordered_map<std::string, double>>();
    for (auto p: params) {
        syn.set(p.first, p.second);
    }

    return syn;
}

std::unordered_map<std::string, mech_groups> read_dynamics_params_density_base(std::string fname) {
    std::ifstream f(fname);

    if (!f.good()) {
        throw std::runtime_error("Unable to open input parameter file: "+fname);
    }
    nlohmann::json json;
    json << f;

    auto mech_definitions = json.get<std::unordered_map<std::string, nlohmann::json>>();

    std::unordered_map<std::string, mech_groups> mech_map;

    for (auto mech_def: mech_definitions) {
        std::string mech_id = mech_def.first; //key: eg. pas_0
        std::unordered_map<std::string, double> variables; // key -value pairs, can be overwritten
        std::vector<mech_params> mech_details;

        auto mech_features = mech_def.second;
        for (auto params: mech_features) { //iterate through json

            for (auto it = params.begin(); it != params.end(); it++) {
                if(!it->is_structured()) {
                    variables[it.key()] = it.value();
                } else {
                    // Mechanism instance
                    std::string section_name;
                    std::string mech_name;
                    std::unordered_map<std::string, double> mech_params;
                    std::unordered_map<std::string, std::string> mech_aliases;

                    for (auto mech_it = it->begin(); mech_it != it->end(); mech_it++) {
                        if (mech_it.key() == "section") {
                            section_name = (mech_it.value()).get<std::string>();
                        } else if (mech_it.key() == "mech") {
                            mech_name = (mech_it.value()).get<std::string>();
                        } else if ((*mech_it).type() == nlohmann::json::value_t::string) {
                            mech_aliases[mech_it.key()] = (mech_it.value()).get<std::string>();
                        } else if ((*mech_it).type() == nlohmann::json::value_t::number_float ||
                                   (*mech_it).type() == nlohmann::json::value_t::number_integer) {
                            mech_params[mech_it.key()] = (mech_it.value()).get<double>();
                        }
                    }

                    auto base_mechanism = arb::mechanism_desc(mech_name);
                    for (auto p: mech_params) {
                        base_mechanism.set(p.first, p.second);
                    }

                    mech_details.emplace_back(section_name, mech_aliases, base_mechanism);
                }
            }
        }
        mech_map.insert({mech_id, mech_groups(variables, mech_details)});
    }
    return mech_map;
}

std::unordered_map<std::string, variable_map> read_dynamics_params_density_override(std::string fname) {
    std::ifstream f(fname);

    if (!f.good()) {
        throw std::runtime_error("Unable to open input parameter file: "+fname);
    }
    nlohmann::json json;
    json << f;

    auto mech_overrides = json.get<std::unordered_map<std::string, nlohmann::json>>();

    std::unordered_map<std::string, variable_map> var_overrides;

    for (auto mech_def: mech_overrides) {
        std::string mech_id = mech_def.first; //key
        variable_map mech_variables = (mech_def.second).get<variable_map>();

        var_overrides.insert({mech_id, mech_variables});
    }
    return var_overrides;
}

