#include <iostream>
#include <fstream>

#include <common/json_params.hpp>

struct sonata_params {

    std::string nodes_0 = "../../network/nodes_0.h5";
    std::string nodes_1 = "../../network/nodes_1.h5";
    std::string nodes_csv = "../../network/node_types.csv";

    std::string edges_0 = "../../network/edges_0.h5";
    std::string edges_1 = "../../network/edges_1.h5";
    std::string edges_2 = "../../network/edges_2.h5";
    std::string edges_3 = "../../network/edges_3.h5";
    std::string edges_csv = "../../network/edge_types.csv";

};

sonata_params read_options(int argc, char** argv) {
    using sup::param_from_json;

    sonata_params params;
    if (argc<4) {
        std::cout << "Using default parameters.\n";
        return params;
    }
    if (argc>4) {
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

    param_from_json(params.nodes_0, "nodes-0", json);
    param_from_json(params.nodes_1, "nodes-1", json);
    param_from_json(params.nodes_csv, "nodes-csv", json);
    param_from_json(params.edges_0, "edges-0", json);
    param_from_json(params.edges_1, "edges-1", json);
    param_from_json(params.edges_2, "edges-2", json);
    param_from_json(params.edges_3, "edges-3", json);
    param_from_json(params.edges_csv, "edges-csv", json);

    if (!json.empty()) {
        for (auto it=json.begin(); it!=json.end(); ++it) {
            std::cout << "  Warning: unused input parameter: \"" << it.key() << "\"\n";
        }
        std::cout << "\n";
    }
    return params;
}
