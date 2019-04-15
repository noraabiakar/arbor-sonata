#include <iostream>
#include <fstream>
#include <unordered_map>

#include <arbor/segment.hpp>
#include <common/json_params.hpp>

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

std::unordered_map<std::string, arb::mechanism_desc>
read_dynamics_params_point(std::string fname) {
    using sup::param_from_json;

    std::unordered_map<std::string, arb::mechanism_desc> point_mechs;
    std::ifstream f(fname);

    if (!f.good()) {
        throw std::runtime_error("Unable to open input parameter file: "+fname);
    }
    nlohmann::json json;
    json << f;

    auto mechs = json.get<std::unordered_map<std::string, nlohmann::json>>();

    for (auto m: mechs) {
        auto syn = arb::mechanism_desc(m.first);
        auto params = m.second.get<std::vector<std::unordered_map<std::string, double>>>();
        for (auto p: params.front()) {
            syn.set(p.first, p.second);
        }
        point_mechs.insert({m.first, std::move(syn)});
    }

    return point_mechs;
}

