#include <arbor/common_types.hpp>
#include <arbor/util/optional.hpp>

#include <string.h>
#include <stdio.h>
#include <hdf5.h>
#include <assert.h>
#include <unordered_set>

#include "hdf5_lib.hpp"
#include "csv_lib.hpp"
#include "mpi_helper.hpp"

using arb::cell_gid_type;
using arb::cell_lid_type;
using arb::cell_size_type;
using arb::cell_member_type;
using arb::segment_location;

struct source_type {
    cell_lid_type segment;
    double position;
    double threshold;

    source_type(): segment(0), position(0), threshold(0) {}
    source_type(cell_lid_type s, double p, double t) : segment(s), position(p), threshold(t) {}
};

struct target_type {
    cell_lid_type segment;
    double position;
    std::string synapse;

    target_type(cell_lid_type s, double p, std::string m) : segment(s), position(p), synapse(m) {}

    bool operator==(const target_type& rhs) {
        return (position == rhs.position) && (segment == rhs.segment) && (synapse == rhs.synapse);
    }
};

inline bool operator==(const source_type& lhs, const source_type& rhs) {
    return lhs.segment   == rhs.segment && lhs.position  == rhs.position && lhs.threshold == rhs.threshold;
}

inline bool operator==(const target_type& lhs, const target_type& rhs) {
    return lhs.position == rhs.position && lhs.segment == rhs.segment && lhs.synapse == rhs.synapse;
}

template<> struct std::hash<source_type>
{
    std::size_t operator()(const source_type& s) const noexcept
    {
        std::size_t const h1(std::hash<unsigned>{}(s.segment));
        std::size_t const h2(std::hash<double>{}(s.position));
        std::size_t const h3(std::hash<double>{}(s.threshold));
        auto h1_2 = h1 ^ (h2 << 1);
        return (h1_2 >> 1) ^ (h3 << 1);
    }
};

template<> struct std::hash<target_type>
{
    std::size_t operator()(const target_type& s) const noexcept
    {
        std::size_t const h1(std::hash<unsigned>{}(s.segment));
        std::size_t const h2(std::hash<double>{}(s.position));
        std::size_t const h3(std::hash<std::string>{}(s.synapse));
        auto h1_2 = h1 ^ (h2 << 1);
        return (h1_2 >> 1) ^ (h3 << 1);
    }
};

class database {
public:
    database(hdf5_record nodes, hdf5_record edges, csv_record node_types, csv_record edge_types):
            nodes_(nodes), edges_(edges), node_types_(node_types), edge_types_(edge_types) {}

    cell_size_type num_cells() {
        return nodes_.num_elements();
    }
    cell_size_type num_edges() {
        return edges_.num_elements();
    }
    void build_source_and_target_maps(std::pair<unsigned, unsigned>);
    void get_connections(cell_gid_type gid, std::vector<arb::cell_connection>& conns);
    void get_sources_and_targets(cell_gid_type gid,
                                 std::vector<std::pair<segment_location, double>>& src,
                                 std::vector<std::pair<segment_location, arb::mechanism_desc>>& tgt);
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

    std::unordered_map <unsigned, unsigned> edge_to_source_of_target(unsigned target_pop) {
        std::unordered_map <unsigned, unsigned> edge_to_source;
        unsigned src_vec_id = edge_types_.map()["source_pop_name"];
        unsigned tgt_vec_id = edge_types_.map()["target_pop_name"];
        unsigned edge_vec_id = edge_types_.map()["pop_name"];

        for (unsigned i = 0; i < edge_types_.data()[src_vec_id].size(); i++) {
            if (edge_types_.data()[tgt_vec_id][i] == nodes_[target_pop].name()) {
                auto edge_pop = edge_types_.data()[edge_vec_id][i];
                auto source_pop = edge_types_.data()[src_vec_id][i];
                edge_to_source[edges_.map()[edge_pop]] = nodes_.map()[source_pop];
            }
        }
        return edge_to_source;
    }

    std::unordered_set<unsigned> edges_of_target(unsigned target_pop) {
        std::unordered_set<unsigned> target_edge_pops;
        unsigned tgt_vec_id = edge_types_.map()["target_pop_name"];
        unsigned edge_vec_id = edge_types_.map()["pop_name"];

        for (unsigned i = 0; i < edge_types_.data()[tgt_vec_id].size(); i++) {
            if (edge_types_.data()[tgt_vec_id][i] == nodes_[target_pop].name()) {
                auto e_pop = edge_types_.data()[edge_vec_id][i];
                target_edge_pops.insert(edges_.map()[e_pop]);
            }
        }
        return target_edge_pops;
    }

    std::unordered_set<unsigned> edges_of_source(unsigned source_pop) {
        std::unordered_set<unsigned> source_edge_pops;
        unsigned src_vec_id = edge_types_.map()["source_pop_name"];
        unsigned edge_vec_id = edge_types_.map()["pop_name"];

        for (unsigned i = 0; i < edge_types_.data()[src_vec_id].size(); i++) {
            if (edge_types_.data()[src_vec_id][i] == nodes_[source_pop].name()) {
                auto e_pop = edge_types_.data()[edge_vec_id][i];
                source_edge_pops.insert(edges_.map()[e_pop]);
            }
        }
        return source_edge_pops;
    }

    hdf5_record nodes_;
    hdf5_record edges_;
    csv_record node_types_;
    csv_record edge_types_;

    std::unordered_map<cell_gid_type, std::vector<source_type>> source_maps_;
    std::unordered_map<cell_gid_type, std::vector<std::pair<target_type, unsigned>>> target_maps_;
};

void database::build_source_and_target_maps(std::pair<unsigned, unsigned> range) {

    // Build loc_source_gids and loc_source_sizes
    std::vector<cell_gid_type> loc_source_gids(range.second - range.first);
    std::iota(loc_source_gids.begin(), loc_source_gids.end(), range.first);

    std::vector<unsigned> loc_source_sizes;
    std::vector<source_type> loc_sources;

    for (unsigned gid = range.first; gid < range.second; gid++) {
        std::unordered_set<source_type> src_set;
        std::vector<std::pair<target_type, unsigned>> tgt_vec;

        auto loc_node = localize_cell(gid);
        auto source_edge_pops = edges_of_source(loc_node.pop_id);
        auto target_edge_pops = edges_of_target(loc_node.pop_id);

        for (auto i: source_edge_pops) {
            auto ind_id = edges_[i].find_group("indicies");
            auto s2t_id = edges_[i][ind_id].find_group("source_to_target");
            auto n2r_range = edges_[i][ind_id][s2t_id].int_pair_at("node_id_to_ranges", loc_node.el_id);

            for (auto j = n2r_range.first; j< n2r_range.second; j++) {
                auto r2e = edges_[i][ind_id][s2t_id].int_pair_at("range_to_edge_id", j);
                auto src_rng = source_range(i, r2e);
                for (auto s: src_rng) {
                    auto loc = src_set.find(s);
                    if (loc == src_set.end()) {
                        src_set.insert(s);
                    }
                }
            }
        }

        for (auto i: target_edge_pops) {
            auto ind_id = edges_[i].find_group("indicies");
            auto t2s_id = edges_[i][ind_id].find_group("target_to_source");
            auto n2r = edges_[i][ind_id][t2s_id].int_pair_at("node_id_to_ranges", loc_node.el_id);
            for (auto j = n2r.first; j< n2r.second; j++) {
                auto r2e = edges_[i][ind_id][t2s_id].int_pair_at("range_to_edge_id", j);

                auto tgt_rng = target_range(i, r2e);

                std::vector<unsigned> edge_rng(r2e.second - r2e.first);
                std::iota(edge_rng.begin(), edge_rng.end(), r2e.first);

                for (unsigned k = 0; k < tgt_rng.size(); k++) {
                    tgt_vec.push_back(std::make_pair(tgt_rng[k], globalize_edge({i, (cell_gid_type)edge_rng[k]})));
                }
            }
        }

        // Build loc_sources
        std::vector<source_type> src_vec(src_set.begin(), src_set.end());
        std::sort(src_vec.begin(), src_vec.end(), [](const auto &a, const auto& b) -> bool
        {
            return std::tie(a.segment, a.position, a.threshold) < std::tie(b.segment, b.position, b.threshold);
        });

        loc_sources.insert(loc_sources.end(), src_vec.begin(), src_vec.end());
        loc_source_sizes.push_back(src_vec.size());

        // Build target_maps_
        std::sort(tgt_vec.begin(), tgt_vec.end(), [](const auto &a, const auto& b) -> bool
        {
            return a.second < b.second;
        });
        target_maps_[gid].insert(target_maps_[gid].end(), tgt_vec.begin(), tgt_vec.end());
    }

#ifdef ARB_MPI_ENABLED
    auto glob_source_gids = gather_all(loc_source_gids, MPI_COMM_WORLD);
    auto glob_source_sizes = gather_all(loc_source_sizes, MPI_COMM_WORLD);
    auto glob_sources = gather_all(loc_sources, MPI_COMM_WORLD);
#else
    auto glob_source_gids = loc_source_gids;
    auto glob_source_sizes = loc_source_sizes;
    auto glob_sources = loc_sources;
#endif

    // Build source_maps
    std::vector<unsigned> glob_source_divs(glob_source_sizes.size() + 1);
    glob_source_divs[0] = 0;
    for (unsigned i = 1; i < glob_source_sizes.size() + 1; i++) {
        glob_source_divs[i] = glob_source_sizes[i-1] + glob_source_divs[i-1];
    }

    for (unsigned i = 0; i < glob_source_gids.size(); i++) {
        auto r0 = glob_source_divs[i];
        auto r1 = glob_source_divs[i + 1];
        auto sub_v = std::vector<source_type>(glob_sources.begin() + r0, glob_sources.begin() + r1);

        source_maps_[glob_source_gids[i]] = std::move(sub_v);
    }

}

void database::get_connections(cell_gid_type gid, std::vector<arb::cell_connection>& conns) {
    // Find cell local index in population
    auto loc_node = localize_cell(gid);
    auto edge_to_source = edge_to_source_of_target(loc_node.pop_id);

    for (auto i: edge_to_source) {
        auto edge_pop = i.first;
        auto source_pop = i.second;

        auto ind_id = edges_[edge_pop].find_group("indicies");
        auto s2t_id = edges_[edge_pop][ind_id].find_group("target_to_source");
        auto n2r_range = edges_[edge_pop][ind_id][s2t_id].int_pair_at("node_id_to_ranges", loc_node.el_id);

        for (auto j = n2r_range.first; j< n2r_range.second; j++) {
            auto r2e = edges_[edge_pop][ind_id][s2t_id].int_pair_at("range_to_edge_id", j);

            auto src_rng = source_range(edge_pop, r2e);
            auto tgt_rng = target_range(edge_pop, r2e);
            auto weights = weight_range(edge_pop, r2e);
            auto delays = delay_range(edge_pop, r2e);

            auto src_id = edges_[edge_pop].int_range("source_node_id", r2e.first, r2e.second);

            std::vector<cell_member_type> sources, targets;

            for(unsigned s = 0; s < src_rng.size(); s++) {
                auto source_gid = globalize_cell({source_pop, (cell_gid_type)src_id[s]});

                auto loc = std::lower_bound(source_maps_[source_gid].begin(), source_maps_[source_gid].end(), src_rng[s],
                                [](const auto& lhs, const auto& rhs) -> bool
                                {
                                    return std::tie(lhs.segment, lhs.position, lhs.threshold) <
                                           std::tie(rhs.segment, rhs.position, rhs.threshold);
                                });

                if (loc != source_maps_[source_gid].end()) {
                    if (*loc == src_rng[s]) {
                        unsigned index = loc - source_maps_[source_gid].begin();
                        sources.push_back({source_gid, index});
                    }
                    else {
                        throw arb::sonata_exception("source maps initialized incorrectly");
                    }
                }
                else {
                    throw arb::sonata_exception("source maps initialized incorrectly");
                }
            }

            unsigned e = 0;
            for(unsigned t = r2e.first; t < r2e.second; t++, e++) {
                auto loc = std::lower_bound(target_maps_[gid].begin(), target_maps_[gid].end(),
                        std::make_pair(tgt_rng[e], globalize_edge({edge_pop, (cell_gid_type)t})),
                        [](const auto& lhs, const auto& rhs) -> bool
                        {
                             return lhs.second < rhs.second;
                        });

                if (loc != target_maps_[gid].end()) {
                    if ((*loc).second == globalize_edge({edge_pop, (cell_gid_type)t})) {
                        unsigned index = loc - target_maps_[gid].begin();
                        targets.push_back({gid, index});
                    }
                    else {
                        throw arb::sonata_exception("target maps initialized incorrectly");
                    }
                }
                else {
                    throw arb::sonata_exception("target maps initialized incorrectly");
                }
            }

            for (unsigned k = 0; k < sources.size(); k++) {
                conns.emplace_back(sources[k], targets[k], weights[k], delays[k]);
            }
        }
    }
}

void database::get_sources_and_targets(cell_gid_type gid,
                             std::vector<std::pair<segment_location, double>>& src,
                             std::vector<std::pair<segment_location, arb::mechanism_desc>>& tgt) {
    src.reserve(source_maps_[gid].size());
    for (auto s: source_maps_[gid]) {
        src.push_back(std::make_pair(segment_location(s.segment, s.position), s.threshold));
    }

    tgt.reserve(target_maps_[gid].size());
    for (auto t: target_maps_[gid]) {
        tgt.push_back(std::make_pair(segment_location(t.first.segment, t.first.position), arb::mechanism_desc(t.first.synapse)));
    }
}

unsigned database::num_sources(cell_gid_type gid) {
    return source_maps_[gid].size();
}

unsigned database::num_targets(cell_gid_type gid) {
    return target_maps_[gid].size();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Private helper functions

// Read from HDF5 file/ CSV file depending on where the information is available

std::vector<source_type> database::source_range(unsigned edge_pop_id, std::pair<unsigned, unsigned> edge_range) {
    std::vector<source_type> ret;

    // First read edge_group_id and edge_group_index and edge_type
    auto edges_grp_id = edges_[edge_pop_id].int_range("edge_group_id", edge_range.first, edge_range.second);
    auto edges_grp_idx = edges_[edge_pop_id].int_range("edge_group_index", edge_range.first, edge_range.second);
    auto edges_type = edges_[edge_pop_id].int_range("edge_type_id", edge_range.first, edge_range.second);

    for (unsigned i = 0; i < edges_grp_id.size(); i++) {
        auto loc_grp_id = edges_grp_id[i];

        int source_branch;
        double source_pos, threshold;

        bool found_source_branch = false;
        bool found_source_pos = false;
        bool found_threshold =false;

        // if the edges are in groups, for each edge find the group, if it exists
        if (edges_[edge_pop_id].find_group(std::to_string(loc_grp_id)) != -1) {
            auto lgi = edges_[edge_pop_id].find_group(std::to_string(loc_grp_id));
            auto group = edges_[edge_pop_id][lgi];
            auto loc_grp_idx = edges_grp_idx[i];

            if (group.find_dataset("efferent_section_id") != -1) {
                source_branch = group.int_at("efferent_section_id", loc_grp_idx);
                found_source_branch = true;
            }
            if (group.find_dataset("efferent_section_pos") != -1) {
                source_pos = group.double_at("efferent_section_pos", loc_grp_idx);
                found_source_pos = true;
            }
            if (group.find_dataset("threshold") != -1) {
                threshold = group.double_at("threshold", loc_grp_idx);
                found_threshold = true;
            }
        }

        // name and index of edge_type_id
        auto e_type = edges_type[i];
        unsigned type_idx = edge_types_.map()["edge_type_id"];

        // find specific index of edge_type in type_idx
        unsigned loc_type_idx;
        for (loc_type_idx = 0; loc_type_idx < edge_types_.data()[type_idx].size(); loc_type_idx++) {
            if (e_type == std::atoi(edge_types_.data()[type_idx][loc_type_idx].c_str())) {
                break;
            }
        }

        if (!found_source_branch) {
            unsigned source_branch_idx = edge_types_.map()["efferent_section_id"];
            source_branch = std::atoi(edge_types_.data()[source_branch_idx][loc_type_idx].c_str());
        }
        if (!found_source_pos) {
            unsigned source_pos_idx = edge_types_.map()["efferent_section_pos"];
            source_pos = std::atof(edge_types_.data()[source_pos_idx][loc_type_idx].c_str());
        }
        if (!found_threshold) {
            unsigned synapse_idx = edge_types_.map()["threshold"];
            threshold = std::atof(edge_types_.data()[synapse_idx][loc_type_idx].c_str());
        }

        ret.emplace_back((unsigned)source_branch, source_pos, threshold);
    }
    return ret;
}

std::vector<target_type> database::target_range(unsigned edge_pop_id, std::pair<unsigned, unsigned> edge_range) {
    std::vector<target_type> ret;

    // First read edge_group_id and edge_group_index and edge_type
    auto edges_grp_id = edges_[edge_pop_id].int_range("edge_group_id", edge_range.first, edge_range.second);
    auto edges_grp_idx = edges_[edge_pop_id].int_range("edge_group_index", edge_range.first, edge_range.second);
    auto edges_type = edges_[edge_pop_id].int_range("edge_type_id", edge_range.first, edge_range.second);

    for (unsigned i = 0; i < edges_grp_id.size(); i++) {
        auto loc_grp_id = edges_grp_id[i];

        int target_branch;
        double target_pos;
        std::string synapse;

        bool found_target_branch = false;
        bool found_target_pos = false;
        bool found_synapse = false;

        // if the edges are in groups, for each edge find the group, if it exists
        if (edges_[edge_pop_id].find_group(std::to_string(loc_grp_id)) != -1) {
            auto lgi = edges_[edge_pop_id].find_group(std::to_string(loc_grp_id));
            auto group = edges_[edge_pop_id][lgi];
            auto loc_grp_idx = edges_grp_idx[i];

            if (group.find_dataset("afferent_section_id") != -1) {
                target_branch = group.int_at("afferent_section_id", loc_grp_idx);
                found_target_branch = true;
            }
            if (group.find_dataset("afferent_section_pos") != -1) {
                target_pos = group.double_at("afferent_section_pos", loc_grp_idx);
                found_target_pos = true;
            }
            if (group.find_dataset("model_template") != -1) {
                synapse = group.string_at("model_template", loc_grp_idx);
                found_synapse = true;
            }
        }

        // name and index of edge_type_id
        auto e_type = edges_type[i];
        unsigned type_idx = edge_types_.map()["edge_type_id"];

        // find specific index of edge_type in type_idx
        unsigned loc_type_idx;
        for (loc_type_idx = 0; loc_type_idx < edge_types_.data()[type_idx].size(); loc_type_idx++) {
            if (e_type == std::atoi(edge_types_.data()[type_idx][loc_type_idx].c_str())) {
                break;
            }
        }

        if (!found_target_branch) {
            unsigned target_branch_idx = edge_types_.map()["afferent_section_id"];
            target_branch = std::atoi(edge_types_.data()[target_branch_idx][loc_type_idx].c_str());
        }
        if (!found_target_pos) {
            unsigned target_pos_idx = edge_types_.map()["afferent_section_pos"];
            target_pos = std::atof(edge_types_.data()[target_pos_idx][loc_type_idx].c_str());
        }
        if (!found_synapse) {
            unsigned synapse_idx = edge_types_.map()["model_template"];
            synapse = edge_types_.data()[synapse_idx][loc_type_idx];
        }

        ret.emplace_back((unsigned)target_branch, target_pos, synapse);
    }
    return ret;
}

std::vector<double> database::weight_range(unsigned edge_pop_id, std::pair<unsigned, unsigned> edge_range) {
    std::vector<double> ret;

    // First read edge_group_id and edge_group_index and edge_type
    auto edges_grp_id = edges_[edge_pop_id].int_range("edge_group_id", edge_range.first, edge_range.second);
    auto edges_grp_idx = edges_[edge_pop_id].int_range("edge_group_index", edge_range.first, edge_range.second);
    auto edges_type = edges_[edge_pop_id].int_range("edge_type_id", edge_range.first, edge_range.second);

    for (unsigned i = 0; i < edges_grp_id.size(); i++) {
        auto loc_grp_id = edges_grp_id[i];

        double weight;
        bool found_weight = false;

        // if the edges are in groups, for each edge find the group, if it exists
        if (edges_[edge_pop_id].find_group(std::to_string(loc_grp_id)) != -1) {
            auto lgi = edges_[edge_pop_id].find_group(std::to_string(loc_grp_id));
            auto group = edges_[edge_pop_id][lgi];
            auto loc_grp_idx = edges_grp_idx[i];

            if (group.find_dataset("syn_weight") != -1) {
                weight = group.double_at("syn_weight", loc_grp_idx);
                found_weight = true;
            }
        }

        // name and index of edge_type_id
        auto e_type = edges_type[i];
        unsigned type_idx = edge_types_.map()["edge_type_id"];

        // find specific index of edge_type in type_idx
        unsigned loc_type_idx;
        for (loc_type_idx = 0; loc_type_idx < edge_types_.data()[type_idx].size(); loc_type_idx++) {
            if (e_type == std::atoi(edge_types_.data()[type_idx][loc_type_idx].c_str())) {
                break;
            }
        }

        if (!found_weight) {
            unsigned weight_idx = edge_types_.map()["syn_weight"];
            weight = std::atof(edge_types_.data()[weight_idx][loc_type_idx].c_str());
        }
        ret.emplace_back(weight);
    }
    return ret;
}

std::vector<double> database::delay_range(unsigned edge_pop_id, std::pair<unsigned, unsigned> edge_range) {
    std::vector<double> ret;

    // First read edge_group_id and edge_group_index and edge_type
    auto edges_grp_id = edges_[edge_pop_id].int_range("edge_group_id", edge_range.first, edge_range.second);
    auto edges_grp_idx = edges_[edge_pop_id].int_range("edge_group_index", edge_range.first, edge_range.second);
    auto edges_type = edges_[edge_pop_id].int_range("edge_type_id", edge_range.first, edge_range.second);

    for (unsigned i = 0; i < edges_grp_id.size(); i++) {
        auto loc_grp_id = edges_grp_id[i];

        double delay;
        bool found_delay = false;

        // if the edges are in groups, for each edge find the group, if it exists
        if (edges_[edge_pop_id].find_group(std::to_string(loc_grp_id)) != -1) {
            auto lgi = edges_[edge_pop_id].find_group(std::to_string(loc_grp_id));
            auto group = edges_[edge_pop_id][lgi];
            auto loc_grp_idx = edges_grp_idx[i];

            if (group.find_dataset("delay") != -1) {
                delay = group.double_at("delay", loc_grp_idx);
                found_delay = true;
            }
        }

        // name and index of edge_type_id
        auto e_type = edges_type[i];
        unsigned type_idx = edge_types_.map()["edge_type_id"];

        // find specific index of edge_type in type_idx
        unsigned loc_type_idx;
        for (loc_type_idx = 0; loc_type_idx < edge_types_.data()[type_idx].size(); loc_type_idx++) {
            if (e_type == std::atoi(edge_types_.data()[type_idx][loc_type_idx].c_str())) {
                break;
            }
        }

        if (!found_delay) {
            unsigned delay_idx = edge_types_.map()["delay"];
            delay = std::atof(edge_types_.data()[delay_idx][loc_type_idx].c_str());
        }
        ret.emplace_back(delay);
    }
    return ret;
}
