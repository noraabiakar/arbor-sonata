#include <iostream>
#include <fstream>
#include <unordered_map>

#include <arbor/segment.hpp>
#include <common/json_params.hpp>

#include "sonata_excpetions.hpp"

std::unordered_map<std::string, double> read_dynamics_params_single(std::string fname) {
    using sup::param_from_json;

    std::unordered_map<std::string, double> params;
    std::ifstream f(fname);

    if (!f.good()) {
        throw std::runtime_error("Unable to open input parameter file: "+fname);
    }
    nlohmann::json json;
    json << f;
    return json.get<std::unordered_map<std::string, double>>();
}

arb::mechanism_desc read_dynamics_params_point(std::string fname) {
    using sup::param_from_json;

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

std::unordered_map<std::string, std::vector<arb::mechanism_desc>> read_dynamics_params_density(std::string fname) {
    using sup::param_from_json;

    std::unordered_map<std::string, std::vector<arb::mechanism_desc>> section_map;
    std::ifstream f(fname);

    if (!f.good()) {
        throw std::runtime_error("Unable to open input parameter file: "+fname);
    }
    nlohmann::json json;
    json << f;

    auto mechs = json.get<std::unordered_map<std::string, nlohmann::json>>();

    for (auto mech: mechs) {
        auto mech_name = mech.first;
        auto mech_params = mech.second;

        for (auto params: mech_params) {
            std::string sec_name;
            auto syn = arb::mechanism_desc(mech_name);

            for (auto it = params.begin(); it != params.end(); it++) {
                if(it.key() == "section") {
                    sec_name = (it.value()).get<std::string>();
                } else {
                    syn.set(it.key(), it.value());
                }
            }
            section_map[sec_name].push_back(syn);
        }
    }

    return section_map;
}

