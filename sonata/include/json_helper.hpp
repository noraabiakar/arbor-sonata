#include <iostream>
#include <fstream>
#include <unordered_map>

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

auto read_dynamics_params_multiple(std::string fname) {
    using sup::param_from_json;

    std::unordered_map<std::string, std::unordered_map<std::string, double>> params;
    std::ifstream f(fname);

    if (!f.good()) {
        throw std::runtime_error("Unable to open input parameter file: "+fname);
    }
    nlohmann::json json;
    json << f;

    auto mechs = json.get<std::unordered_map<std::string, nlohmann::json>>();

    for (auto m: mechs) {
        params[m.first] = m.second.get<std::unordered_map<std::string, double>>();
    }
    return params;
}

