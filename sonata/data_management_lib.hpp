#pragma once

#include <arbor/common_types.hpp>
#include <arbor/cable_cell.hpp>
#include <arbor/domain_decomposition.hpp>
#include <arbor/recipe.hpp>

#include <string>
#include <unordered_set>

#include "include/hdf5_lib.hpp"
#include "include/csv_lib.hpp"
#include "include/sonata_excpetions.hpp"
#include "include/common_structs.hpp"


using arb::cell_gid_type;
using arb::cell_lid_type;
using arb::cell_size_type;
using arb::cell_member_type;
using arb::segment_location;

class database {
public:
    database(hdf5_record nodes,
             hdf5_record edges,
             csv_record node_types,
             csv_record edge_types,
             std::vector<spike_info> spikes,
             std::vector<current_clamp_info> current_clamp):
    nodes_(nodes), edges_(edges), node_types_(node_types), edge_types_(edge_types), spikes_(spikes) {
        build_current_clamp_map(current_clamp);
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

    /* Helper functions */
    struct local_element{
        cell_gid_type pop_id;
        cell_gid_type el_id;
    };

    local_element localize_cell(cell_gid_type gid) {
        unsigned i = 0;
        for (; i < nodes_.partitions().size(); i++) {
            if (gid < nodes_.partitions()[i]) {
                return {i-1, gid - nodes_.partitions()[i-1]};
            }
        }
        return local_element();
    }

    cell_gid_type globalize_cell(local_element n) {
        return n.el_id + nodes_.partitions()[n.pop_id];
    }

    cell_gid_type globalize_edge(local_element e) {
        return e.el_id + edges_.partitions()[e.pop_id];
    }

    std::unordered_map<unsigned, unsigned> edge_to_source_of_target(unsigned target_pop) {
        std::unordered_map <unsigned, unsigned> edge_to_source;

        for (auto edge_type: edge_types_.data()) {
            auto type = edge_type.second;
            if (type["target_pop_name"] == nodes_[target_pop].name()) {
                edge_to_source[edges_.map()[type["pop_name"]]] = nodes_.map()[type["source_pop_name"]];
            }
        }
        return edge_to_source;
    }

    std::unordered_set<unsigned> edges_of_target(unsigned target_pop) {
        std::unordered_set<unsigned> target_edge_pops;

        for (auto edge_type: edge_types_.data()) {
            auto type = edge_type.second;
            if (type["target_pop_name"] == nodes_[target_pop].name()) {
                target_edge_pops.insert(edges_.map()[type["pop_name"]]);
            }
        }
        return target_edge_pops;
    }

    std::unordered_set<unsigned> edges_of_source(unsigned source_pop) {
        std::unordered_set<unsigned> source_edge_pops;

        for (auto edge_type: edge_types_.data()) {
            auto type = edge_type.second;
            if (type["source_pop_name"] == nodes_[source_pop].name()) {
                source_edge_pops.insert(edges_.map()[type["pop_name"]]);
            }
        }
        return source_edge_pops;
    }

    hdf5_record nodes_;
    hdf5_record edges_;
    csv_record node_types_;
    csv_record edge_types_;

    std::unordered_map<cell_gid_type, std::vector<current_clamp>> current_clamps_;
    std::vector<spike_info> spikes_;

    std::unordered_map<cell_gid_type, std::vector<source_type>> source_maps_;
    std::unordered_map<cell_gid_type, std::vector<std::pair<target_type, unsigned>>> target_maps_;
};

