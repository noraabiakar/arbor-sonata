#pragma once

#include <arbor/common_types.hpp>
#include <arbor/cable_cell.hpp>
#include <arbor/domain_decomposition.hpp>
#include <arbor/recipe.hpp>

#include <string>
#include <unordered_set>

#include "hdf5_lib.hpp"
#include "csv_lib.hpp"
#include "sonata_exceptions.hpp"
#include "common_structs.hpp"

using arb::cell_gid_type;
using arb::cell_lid_type;
using arb::cell_size_type;
using arb::cell_member_type;
using arb::segment_location;

class database {
public:
    database(h5_record nodes,
             h5_record edges,
             csv_node_record node_types,
             csv_edge_record edge_types,
             std::vector<spike_info> spikes,
             std::vector<current_clamp_info> current_clamp):
    nodes_(nodes), edges_(edges), node_types_(node_types), edge_types_(edge_types), spikes_(spikes) {
        build_current_clamp_map(current_clamp);
    }

    std::vector<unsigned> pop_partitions() const {
        return nodes_.partitions();
    }

    std::vector<std::string> pop_names() const {
        return nodes_.pop_names();
    }

    std::string population_of(cell_gid_type gid) {
        for (unsigned i = 0; i < nodes_.partitions().size(); i++) {
            if (gid < nodes_.partitions()[i]) {
                return nodes_.populations()[i-1].name();
            }
        }
    }

    unsigned population_id_of(cell_gid_type gid) {
        for (unsigned i = 0; i < nodes_.partitions().size(); i++) {
            if (gid < nodes_.partitions()[i]) {
                return gid - nodes_.partitions()[i-1];
            }
        }
    }

    cell_size_type num_cells() {
        return nodes_.num_elements();
    }

    cell_size_type num_edges() {
        return edges_.num_elements();
    }

    void build_source_and_target_maps(const std::vector<arb::group_description>&);

    void build_current_clamp_map(std::vector<current_clamp_info> current);

    void get_connections(cell_gid_type gid, std::vector<arb::cell_connection>& conns);

    void get_sources_and_targets(cell_gid_type gid,
                                 std::vector<segment_location>& src,
                                 std::vector<std::pair<segment_location, arb::mechanism_desc>>& tgt);

    std::vector<current_clamp> get_current_clamps(cell_gid_type gid) {
        if (current_clamps_.find(gid) != current_clamps_.end()) {
            return current_clamps_.at(gid);
        }
        return {};
    };

    std::vector<double> get_spikes(cell_gid_type gid);

    arb::morphology get_cell_morphology(cell_gid_type gid);

    arb::cell_kind get_cell_kind(cell_gid_type gid);

    // Returns section -> mechanisms
    std::unordered_map<std::string, std::vector<arb::mechanism_desc>> get_density_mechs(cell_gid_type);

    unsigned num_sources(cell_gid_type gid);
    unsigned num_targets(cell_gid_type gid);

private:

    /* Read relevant information from HDF5 or CSV */
    std::vector<source_type> source_range(unsigned edge_pop_id, std::pair<unsigned, unsigned> edge_range);
    std::vector<target_type> target_range(unsigned edge_pop_id, std::pair<unsigned, unsigned> edge_range);
    std::vector<double> weight_range(unsigned edge_pop_id, std::pair<unsigned, unsigned> edge_range);
    std::vector<double> delay_range(unsigned edge_pop_id, std::pair<unsigned, unsigned> edge_range);


    h5_record nodes_;
    h5_record edges_;
    csv_node_record node_types_;
    csv_edge_record edge_types_;

    std::unordered_map<cell_gid_type, std::vector<current_clamp>> current_clamps_;
    std::vector<spike_info> spikes_;

    std::unordered_map<cell_gid_type, std::vector<source_type>> source_maps_;
    std::unordered_map<cell_gid_type, std::vector<std::pair<target_type, unsigned>>> target_maps_;
};

