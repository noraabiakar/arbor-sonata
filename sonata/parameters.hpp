#include <iostream>
#include <fstream>
#include <set>

#include <hdf5.h>

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
    : nodes(std::move(other.nodes)),
      nodes_types(std::move(other.nodes_types)),
      edges(std::move(other.edges)),
      edges_types(std::move(other.edges_types)) {}

    network_params(const network_params& other)
            : nodes(other.nodes), nodes_types(other.nodes_types), edges(other.edges), edges_types(other.edges_types) {}
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

struct spike_out_info {
    std::string file_name;
    std::string sort_by;
};

struct probe_info {
    std::string kind;
    std::string population;
    std::vector<unsigned> node_ids;
    unsigned sec_id;
    double sec_pos;

    std::string file_name;
};

struct sonata_params {
    network_params network;
    sim_conditions conditions;
    run_params run;
    std::vector<current_clamp_info> current_clamps;
    std::vector<spike_info> spikes_input;
    spike_out_info spike_output;
    std::vector<probe_info> probes_info;

    sonata_params(network_params&& n,
                  sim_conditions&& s,
                  run_params&& r,
                  std::vector<current_clamp_info>&& clamps,
                  std::vector<spike_info>&& spikes,
                  spike_out_info&& output,
                  std::vector<probe_info>&& probes):
    network(std::move(n)),
    conditions(std::move(s)),
    run(std::move(r)),
    current_clamps(std::move(clamps)),
    spikes_input(std::move(spikes)),
    spike_output(std::move(output)),
    probes_info(std::move(probes)) {}
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

std::vector<current_clamp_info> read_clamps(std::unordered_map<std::string, nlohmann::json>& stim_json) {
    using sup::param_from_json;
    std::vector<current_clamp_info> ret;

    for (auto input: stim_json) {
        if (input.second["input_type"] == "current_clamp") {
            csv_file elec_file(input.second["electrode_file"].get<std::string>());
            csv_file input_file(input.second["input_file"].get<std::string>());
            ret.push_back({elec_file, input_file});
        }
    }
    return ret;
}

std::vector<spike_info> read_spikes(std::unordered_map<std::string, nlohmann::json>& spike_json, const nlohmann::json& node_set_json) {
    using sup::param_from_json;
    std::vector<spike_info> ret;

    for (auto input: spike_json) {
        if (input.second["input_type"] == "spikes") {
            h5_wrapper rec(h5_file(input.second["input_file"].get<std::string>()).top_group_);

            std::string given_set = input.second["node_set"].get<std::string>();
            auto node_set_params = node_set_json[given_set];

            std::string pop = node_set_params["population"].get<std::string>();
            ret.push_back({rec, pop});
        }
    }
    return ret;
}

std::vector<probe_info> read_probes(std::unordered_map<std::string, nlohmann::json>& reports_json, const nlohmann::json& node_set_json) {
    using sup::param_from_json;
    std::vector<probe_info> ret;

    for (auto report: reports_json) {
        probe_info probe;
        probe.file_name = report.second["report_file"].get<std::string>();
        probe.kind = report.second["variable_name"].get<std::string>();
        probe.sec_id = report.second["section_id"].get<unsigned>();
        probe.sec_pos = report.second["section_pos"].get<double>();

        std::string given_node_set = report.second["node_set"].get<std::string>();
        auto node_set_params = node_set_json[given_node_set];

        probe.population = node_set_params["population"].get<std::string>();
        probe.node_ids = node_set_params["ids"].get<std::vector<unsigned>>();
        std::sort(probe.node_ids.begin(), probe.node_ids.end());
        ret.push_back(probe);
    }
    return ret;
}

sonata_params read_options(int argc, char** argv) {
    if (argc>2) {
        throw std::runtime_error("More than one command line option not permitted.");
    }
    if (argc<2) {
        throw std::runtime_error("Simulation configuration file required.");
    }

    std::string sim_file = argv[1];
    std::cout << "Loading parameters from file: " << sim_file << "\n";
    std::ifstream sim_config(sim_file);

    if (!sim_config.good()) {
        throw std::runtime_error("Unable to open input simulation config file: " + sim_file);
    }

    nlohmann::json sim_json;
    sim_json << sim_config;

    /// Node_set_file
    // Node set file name
    auto node_set_name = sim_json.find("node_sets_file");
    std::string node_set_file = *node_set_name;

    nlohmann::json node_set_json;

    std::ifstream node_set(node_set_file);
    if (!node_set.good()) {
        throw std::runtime_error("Unable to open node_set_file: "+ node_set_file);
    }
    node_set_json << node_set;


    /// Simulation Conditions
    // Read simulation conditions
    auto conditions_field = sim_json.find("conditions");
    sim_conditions conditions(read_sim_conditions(*conditions_field));

    /// Simulation Run parameters
    // Read run parameters
    auto run_field = sim_json.find("run");
    run_params run(read_run_params(*run_field));

    /// Network
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

    /// Inputs (stimuli)
    // Get json of inputs
    auto inputs_fields = sim_json["inputs"].get<std::unordered_map<std::string, nlohmann::json>>();

    // Read stimulus parameters
    auto clamps = read_clamps(inputs_fields);
    auto spikes = read_spikes(inputs_fields, node_set_json);

    /// Outputs (spikes)
    // Get json of outputs
    auto output_field = sim_json["outputs"];

    // Read output parameters
    spike_out_info output{output_field["spikes_file"], output_field["spikes_sort_order"]};

    /// Reports (probes)
    // Get json of reports
    auto reports_field = sim_json["reports"].get<std::unordered_map<std::string, nlohmann::json>>();

    // Read report(probe) parameters
    auto probes = read_probes(reports_field, node_set_json);

    sonata_params params(std::move(network), std::move(conditions), std::move(run), std::move(clamps), std::move(spikes), std::move(output), std::move(probes));

    return params;
}

void write_spikes(std::vector<arb::spike>& spikes,
                  bool sort_by_time, std::string file_name,
                  const std::vector<std::string>& pop_names,
                  const std::vector<unsigned>& pop_parts) {
    //Sort spikes by gid
    std::sort(spikes.begin(), spikes.end(), [](const arb::spike& a, const arb::spike& b) -> bool
    {
        return a.source < b.source;
    });

    //Split by population
    std::vector<int> spike_part = {0};
    for (unsigned i = 1; i < pop_parts.size(); i++) {
        auto it = std::lower_bound(spikes.begin(), spikes.end(), pop_parts[i], [](const arb::spike& a, unsigned b) -> bool
        {
            return a.source.gid < b;
        });
        spike_part.push_back(it - spikes.begin());
    }

    //If we need to sort by time, sort by partition
    if (sort_by_time) {
        for (unsigned i = 1; i < spike_part.size(); i++){
            std::sort(spikes.begin() + spike_part[i-1], spikes.begin() + spike_part[i],
                      [](const arb::spike& a, const arb::spike& b) -> bool
                      {
                          return a.time < b.time;
                      });
        }
    }

    auto file = H5Fcreate(file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    auto group = H5Gcreate(file, "spikes", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    for (unsigned p = 0; p < pop_names.size(); p++) {
        if (spike_part[p+1] > spike_part[p]) {
            std::string full_group_name = "/spikes/" + pop_names[p];
            std::string full_dset_gid_name = full_group_name + "/node_ids";
            std::string full_dset_time_name = full_group_name + "/timestamps";

            auto pop_group = H5Gcreate(file, full_group_name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

            hsize_t size = spike_part[p + 1] - spike_part[p];

            int spike_gids[size];
            double spike_times[size];

            for (unsigned i = spike_part[p]; i < spike_part[p + 1]; i++) {
                spike_gids[i - spike_part[p]] = spikes[i].source.gid - pop_parts[p];
                spike_times[i - spike_part[p]] = spikes[i].time;
            }

            auto space = H5Screate_simple(1, &size, NULL);
            auto dset_gid = H5Dcreate(file, full_dset_gid_name.c_str(), H5T_NATIVE_INT, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            auto dset_time = H5Dcreate(file, full_dset_time_name.c_str(), H5T_NATIVE_DOUBLE, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

            H5Dwrite(dset_gid, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, spike_gids);
            H5Dwrite(dset_time, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, spike_times);

            H5Dclose(dset_gid);
            H5Dclose(dset_time);
            H5Sclose(space);
            H5Gclose(pop_group);
        }
    }
    H5Gclose (group);
    H5Fclose (file);
}

void write_trace(const std::unordered_map<cell_member_type, trace_info>& trace,
                 const std::unordered_map<std::string, std::vector<cell_member_type>>& trace_groups,
                 const std::vector<std::string>& pop_names,
                 const std::vector<unsigned>& pop_parts) {

    for (auto t: trace_groups) {
        std::string file_name = t.first;
        auto traced_probes = t.second;

        //Sort traced_probes by gid
        std::sort(traced_probes.begin(), traced_probes.end(), [](const arb::cell_member_type& a, const arb::cell_member_type& b) -> bool
        {
            return a.gid < b.gid;
        });

        //Split by population
        std::vector<int> trace_part = {0};
        for (unsigned i = 1; i < pop_parts.size(); i++) {
            auto it = std::lower_bound(traced_probes.begin(), traced_probes.end(), pop_parts[i], [](const arb::cell_member_type& a, unsigned b) -> bool
            {
                return a.gid < b;
            });
            trace_part.push_back(it - traced_probes.begin());
        }

        auto file = H5Fcreate(file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
        auto group = H5Gcreate(file, "reports", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

        for (unsigned p = 0; p < pop_names.size(); p++) {
            if (trace_part[p+1] > trace_part[p]) {
                std::string full_group_name = "/reports/" + pop_names[p];
                std::string full_mapping_group = full_group_name + "/mapping";

                auto pop_group = H5Gcreate(file, full_group_name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                auto mapping_group = H5Gcreate (file, full_mapping_group.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

                hsize_t num_traces = trace_part[p+1] - trace_part[p];
                hsize_t size_trace = trace.at(traced_probes.front()).data.size();

                hsize_t dims_data[2] = {num_traces, size_trace};
                hsize_t dims_time[1] = {size_trace};
                hsize_t dims_values[1] = {num_traces};

                double trace_data[num_traces][size_trace];
                double trace_time[size_trace];
                unsigned seg_id[num_traces];
                double seg_pos[num_traces];

                std::vector<unsigned> unique_gids;
                std::vector<unsigned> gid_parts = {0};

                unsigned i;
                for (i = trace_part[p]; i < trace_part[p+1]; i++) {
                    auto& info = trace.at(traced_probes[i]);
                    for (unsigned j = 0; j < info.data.size(); j++) {
                        trace_data[i - trace_part[p]][j] = info.data[j].v;
                    }
                    seg_id[i - trace_part[p]] = info.seg_id;
                    seg_pos[i - trace_part[p]] = info.seg_pos;

                    if (i > trace_part[p]) {
                        if (traced_probes[i].gid != traced_probes[i-1].gid) {
                            gid_parts.push_back(i);
                            unique_gids.push_back(traced_probes[i-1].gid - pop_parts[p]);
                        }
                    }
                }
                gid_parts.push_back(i);
                unique_gids.push_back(traced_probes[i-1].gid - pop_parts[p]);

                auto& info = trace.at(traced_probes[trace_part[p]]);
                for (unsigned j = 0; j < info.data.size(); j++) {
                    trace_time[j] = info.data[j].t;
                }

                //Write values
                std::string full_dset_data_name = full_group_name + "/data";
                auto space = H5Screate_simple(2, dims_data, NULL);
                auto dataset = H5Dcreate(file, full_dset_data_name.c_str(), H5T_NATIVE_DOUBLE, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                auto status = H5Dwrite(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, trace_data);

                H5Dclose (dataset);
                H5Sclose (space);

                //Write times
                std::string full_dset_time_name = full_mapping_group + "/time";
                space = H5Screate_simple(1, dims_time, NULL);
                dataset = H5Dcreate(file, full_dset_time_name.c_str(), H5T_NATIVE_DOUBLE, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                status = H5Dwrite(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, trace_time);

                H5Dclose (dataset);
                H5Sclose (space);

                //Write seg_ids
                std::string full_dset_id_name = full_mapping_group + "/element_ids";
                space = H5Screate_simple(1, dims_values, NULL);
                dataset = H5Dcreate(file, full_dset_id_name.c_str(), H5T_NATIVE_INT, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                status = H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, seg_id);

                H5Dclose (dataset);
                H5Sclose (space);

                //Write seg_pos
                std::string full_dset_pos_name = full_mapping_group + "/element_pos";
                space = H5Screate_simple(1, dims_values, NULL);
                dataset = H5Dcreate(file, full_dset_pos_name.c_str(), H5T_NATIVE_DOUBLE, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                status = H5Dwrite(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, seg_pos);

                H5Dclose (dataset);
                H5Sclose (space);

                hsize_t dims_nodes[1] = {unique_gids.size()};
                hsize_t dims_idx[1] = {gid_parts.size()};

                unsigned nodes[unique_gids.size()];
                unsigned indices[gid_parts.size()];

                std::copy(unique_gids.begin(), unique_gids.end(), nodes);
                std::copy(gid_parts.begin(), gid_parts.end(), indices);

                //Write node_ids
                std::string full_dset_node_name = full_mapping_group + "/node_ids";
                space = H5Screate_simple(1, dims_nodes, NULL);
                dataset = H5Dcreate(file, full_dset_node_name.c_str(), H5T_NATIVE_INT, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                status = H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, nodes);

                H5Dclose (dataset);
                H5Sclose (space);

                //Write index_pointers
                std::string full_dset_idx_name = full_mapping_group + "/index_pointers";
                space = H5Screate_simple(1, dims_idx, NULL);
                dataset = H5Dcreate(file, full_dset_idx_name.c_str(), H5T_NATIVE_INT, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                status = H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, indices);

                H5Dclose (dataset);
                H5Sclose (space);

                H5Gclose (mapping_group);
                H5Gclose (pop_group);
            }
        }

        H5Gclose (group);
        H5Fclose (file);
    }
}
