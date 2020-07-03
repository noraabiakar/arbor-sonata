#pragma once

#include <arbor/common_types.hpp>
#include <arbor/cable_cell.hpp>
#include <arbor/domain_decomposition.hpp>
#include <arbor/recipe.hpp>

#include <string>
#include <unordered_set>
#include <set>

#include "sonata_exceptions.hpp"
#include "common_structs.hpp"

using arb::cell_gid_type;
using arb::cell_lid_type;
using arb::cell_size_type;
using arb::cell_member_type;
using arb::mlocation;

class model_desc {
public:
    model_desc(h5_record nodes,
               h5_record edges,
               csv_node_record node_types,
               csv_edge_record edge_types);

    /// Simple queries

    cell_size_type num_cells() const;

    cell_size_type num_sources(cell_gid_type gid) const;

    cell_size_type num_targets(cell_gid_type gid) const;

    std::vector<unsigned> pop_partitions() const;

    std::vector<std::string> pop_names() const;

    std::string population_of(cell_gid_type gid) const;

    unsigned population_id_of(cell_gid_type gid) const;


    /// Fill member maps

    // Queries hdf5/csv records as needed to build (with correct overrides)all needed
    // information to form a cell_connection.
    // This includes source and target locations (gid, branch id, branch position)
    // weight/delay of the connection and point mechanism with all parameters set
    void build_source_and_target_maps(const std::vector<arb::group_description>&);

    /// Read maps

    void get_sources_and_targets(cell_gid_type gid, std::vector<mlocation>& src,
                                 std::vector<std::pair<mlocation, arb::mechanism_desc>>& tgt) const;


    // Look for morphology file in hdf5 file; if not found, use the default morphology from the node csv file
    arb::morphology get_cell_morphology(cell_gid_type gid);

    // Get cell_kind from the node csv file
    arb::cell_kind get_cell_kind(cell_gid_type gid);

    // Looks for edges with target == gid, creates the connections
    void get_connections(cell_gid_type gid, std::vector<arb::cell_connection>& conns);

    // Queries csv/hdf5 records as needed to get a cell's density mechanisms (with correct parameter overrides)
    // Returns a map from section kind (soma, dend, etc) to a vector of mechanism_desc
    std::unordered_map<section_kind, std::vector<arb::mechanism_desc>> get_density_mechs(cell_gid_type);

    /// Read relevant information from the relevant hdf5 file in ranges and aggregate in convenient structs

    std::vector<source_type> source_range(unsigned edge_pop_id, std::pair<unsigned, unsigned> edge_range);
    std::vector<target_type> target_range(unsigned edge_pop_id, std::pair<unsigned, unsigned> edge_range);
    std::vector<double> weight_range(unsigned edge_pop_id, std::pair<unsigned, unsigned> edge_range);
    std::vector<double> delay_range(unsigned edge_pop_id, std::pair<unsigned, unsigned> edge_range);


private:
    h5_record nodes_;
    h5_record edges_;
    csv_node_record node_types_;
    csv_edge_record edge_types_;

    // Map from gid to vector of source_type on the cell
    std::unordered_map<cell_gid_type, std::vector<source_type>> source_maps_;

    // Map from gid to vector of target_type, gid on the cell
    std::unordered_map<cell_gid_type, std::vector<std::pair<target_type, unsigned>>> target_maps_;
};

class io_desc {
public:
    io_desc(h5_record nodes,
               std::vector<spike_in_info> spikes,
               std::vector<current_clamp_info> current_clamp,
               std::vector<probe_info> probes);

    /// Fill member maps

    // Queries hdf5/csv records as needed to build (with correct overrides)all needed
    // information to form a cell_connection.

    void build_current_clamp_map(std::vector<current_clamp_info> current);

    void build_spike_map(std::vector<spike_in_info> spikes);

    void build_probe_map(std::vector<probe_info> probes);

    /// Read maps

    std::vector<current_clamp_desc> get_current_clamps(cell_gid_type gid) const;

    std::vector<double> get_spikes(cell_gid_type gid) const;

    cell_size_type get_num_probes(cell_gid_type gid) const;

    std::vector<trace_index_and_info> get_probe(cell_gid_type gid) const;

    std::unordered_map<std::string, std::vector<cell_member_type>> get_probe_groups() const;

private:
    h5_record nodes_;

    // Map from gid to vector of current_clamp descriptors
    std::unordered_map<cell_gid_type, std::vector<current_clamp_desc>> current_clamp_map_;

    // Map from gid to vector of time stamps of input spikes
    std::unordered_map<cell_gid_type, std::vector<double>> spike_map_;

    // Map from cell_gid_type to vector of time stamps of input spikes
    std::unordered_map<cell_gid_type, std::vector<trace_index_and_info>> probe_map_;

    // Map from file name to vector of probes (cell_member_types) to be recorded in the file
    std::unordered_map<std::string, std::vector<cell_member_type>> probe_groups_;

};

