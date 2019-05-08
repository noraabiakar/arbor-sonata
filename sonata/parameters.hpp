#include <iostream>
#include <fstream>
#include <set>

#include <common/json_params.hpp>

#include "data_management_lib.hpp"

using h5_file_handle = std::shared_ptr<h5_file>;

struct network_params {
    hdf5_record nodes;
    csv_record nodes_types;

    hdf5_record edges;
    csv_record edges_types;

    network_params(std::vector<h5_file_handle> nodes_h5,
                   std::vector<csv_file> nodes_csv,
                   std::vector<h5_file_handle> edges_h5,
                   std::vector<csv_file> edges_csv):
    nodes(nodes_h5), edges(edges_h5), nodes_types(nodes_csv), edges_types(edges_csv)
    {
        nodes.verify_nodes();
        edges.verify_edges();
    }

    network_params(network_params&& other)
    : nodes(other.nodes), nodes_types(other.nodes_types), edges(other.edges), edges_types(other.edges_types)
    {}

    network_params(const network_params& other)
            : nodes(other.nodes), nodes_types(other.nodes_types), edges(other.edges), edges_types(other.edges_types)
    {}
};

struct sim_conditions {
    double temp_c;
    double v_init;
};

struct run_params {
    double duration;
    double dt;
    double threshold;
};

struct sonata_params {
    network_params network;
    sim_conditions conditions;
    run_params run;
    current_clamp_info current_clamp;
    spike_info spikes;

    sonata_params(const network_params& n, const sim_conditions& s, const run_params& r, const current_clamp_info& stim, const spike_info& sp):
    network(n), conditions(s), run(r), current_clamp(stim), spikes(sp) {}
};

network_params read_network_params(nlohmann::json network_json) {
    using sup::param_from_json;

    auto node_files = network_json["nodes"].get<std::vector<nlohmann::json>>();
    auto edge_files = network_json["edges"].get<std::vector<nlohmann::json>>();

    std::set<std::string> nodes_h5_names, nodes_csv_names;
    for (auto n: node_files) {
        std::string node_h5, node_csv;
        param_from_json(node_h5, "nodes_file", n);
        param_from_json(node_csv, "node_types_file", n);

        nodes_h5_names.insert(node_h5);
        nodes_csv_names.insert(node_csv);
    }

    std::set<std::string> edges_h5_names, edges_csv_names;
    for (auto n: edge_files) {
        std::string edge_h5, edge_csv;
        param_from_json(edge_h5, "edges_file", n);
        param_from_json(edge_csv, "edge_types_file", n);

        edges_h5_names.insert(edge_h5);
        edges_csv_names.insert(edge_csv);
    }

    std::vector<h5_file_handle> nodes_h5, edges_h5;
    std::vector<csv_file> nodes_csv, edges_csv;

    for (auto f: nodes_h5_names) {
        nodes_h5.emplace_back(std::make_shared<h5_file>(f));
    }

    for (auto f: edges_h5_names) {
        edges_h5.emplace_back(std::make_shared<h5_file>(f));
    }

    for (auto f: nodes_csv_names) {
        nodes_csv.emplace_back(f);
    }

    for (auto f: edges_csv_names) {
        edges_csv.emplace_back(f);
    }

    network_params params_network(nodes_h5, nodes_csv, edges_h5, edges_csv);

    return params_network;
}

sim_conditions read_sim_conditions(nlohmann::json condition_json) {
    using sup::param_from_json;

    sim_conditions conditions;

    param_from_json(conditions.temp_c, "celsius", condition_json);
    param_from_json(conditions.v_init, "v_init", condition_json );

    return conditions;
}

run_params read_run_params(nlohmann::json run_json) {
    using sup::param_from_json;

    run_params run;

    param_from_json(run.duration, "tstop", run_json);
    param_from_json(run.dt, "dt", run_json );
    param_from_json(run.threshold, "spike_threshold", run_json );

    return run;
}

current_clamp_info read_stimuli(std::unordered_map<std::string, nlohmann::json>& stim_json) {
    using sup::param_from_json;

    for (auto input: stim_json) {
        if (input.second["input_type"] == "current_clamp") {
            csv_file elec_file(input.second["electrode_file"].get<std::string>());
            csv_file input_file(input.second["input_file"].get<std::string>());
            return {elec_file, input_file};
        }
    }
}

spike_info read_spikes(std::unordered_map<std::string, nlohmann::json>& spike_json, std::string node_set_file) {
    using sup::param_from_json;

    for (auto input: spike_json) {
        if (input.second["input_type"] == "spikes") {
            h5_wrapper rec(h5_file(input.second["input_file"].get<std::string>()).top_group_);

            nlohmann::json node_set_json;

            std::ifstream node_set(node_set_file);
            if (!node_set.good()) {
                throw std::runtime_error("Unable to open node_set_file: "+ node_set_file);
            }
            node_set_json << node_set;

            std::string given_set = input.second["node_set"].get<std::string>();
            auto node_set_params = node_set_json[given_set];

            std::string pop = node_set_params["population"].get<std::string>();
            return {rec, pop};
        }
    }
}

sonata_params read_options(int argc, char** argv) {
    if (argc>2) {
        throw std::runtime_error("More than one command line option not permitted.");
    }

    std::string sim_file = argv[1];
    std::cout << "Loading parameters from file: " << sim_file << "\n";
    std::ifstream sim_config(sim_file);

    if (!sim_config.good()) {
        throw std::runtime_error("Unable to open input simulation config file: " + sim_file);
    }

    nlohmann::json sim_json;
    sim_json << sim_config;

    // Read simulation conditions
    auto conditions_field = sim_json.find("conditions");
    sim_conditions conditions(read_sim_conditions(*conditions_field));

    // Read run parameters
    auto run_field = sim_json.find("run");
    run_params run(read_run_params(*run_field));

    // Read circuit_config file name from the "network" field
    auto network_field = sim_json.find("network");
    std::string network_file = *network_field;

    // Open circuit_config
    std::ifstream circuit_config(network_file);
    if (!circuit_config.good()) {
        throw std::runtime_error("Unable to open input circuit config file: "+ network_file);
    }

    nlohmann::json circuit_json;
    circuit_json << circuit_config;

    // Get json of network parameters
    auto circuit_config_map = circuit_json.get<std::unordered_map<std::string, nlohmann::json>>();

    // Read network parameters
    network_params network(read_network_params(circuit_config_map["network"]));

    // Get json of inputs
    auto inputs_fields = sim_json["inputs"].get<std::unordered_map<std::string, nlohmann::json>>();

    // Node set file name
    auto node_set = sim_json.find("node_sets_file");
    std::string node_set_file = *node_set;

    // Read network parameters
    auto stimuli = read_stimuli(inputs_fields);
    auto spikes = read_spikes(inputs_fields, node_set_file);

    sonata_params params(network, conditions, run, stimuli, spikes);

    return params;
}
