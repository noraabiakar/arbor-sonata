#include <iostream>
#include <fstream>
#include <set>

#include <common/json_params.hpp>

struct sonata_params {
    std::set<std::string> nodes;
    std::set<std::string> nodes_types;

    std::set<std::string> edges;
    std::set<std::string> edges_types;
};

sonata_params read_options(int argc, char** argv) {
    using sup::param_from_json;

    sonata_params params;
    if (argc<2) {
        std::cout << "Using default parameters.\n";
        return params;
    }
    if (argc>2) {
        throw std::runtime_error("More than one command line option not permitted.");
    }

    std::string fname = argv[1];
    std::cout << "Loading parameters from file: " << fname << "\n";
    std::ifstream f(fname);

    if (!f.good()) {
        throw std::runtime_error("Unable to open input parameter file: "+fname);
    }

    nlohmann::json json;
    json << f;

    /**********************************************************************************/
    auto input_fields = json.get<std::unordered_map<std::string, nlohmann::json>>();

    auto node_edge_files = input_fields["network"].get<std::unordered_map<std::string, nlohmann::json>>();
    auto node_files = node_edge_files["nodes"].get<std::vector<nlohmann::json>>();
    auto edge_files = node_edge_files["edges"].get<std::vector<nlohmann::json>>();

    for (auto n: node_files) {
        std::string node_h5, node_csv;
        param_from_json(node_h5, "nodes_file", n);
        param_from_json(node_csv, "node_types_file", n);

        params.nodes.insert(node_h5);
        params.nodes_types.insert(node_csv);
    }

    for (auto n: edge_files) {
        std::string edge_h5, edge_csv;
        param_from_json(edge_h5, "edges_file", n);
        param_from_json(edge_csv, "edge_types_file", n);

        params.edges.insert(edge_h5);
        params.edges_types.insert(edge_csv);
    }

    return params;
}
