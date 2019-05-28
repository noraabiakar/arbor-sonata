#include <arbor/version.hpp>
#include <arbor/mechcat.hpp>

#include "data_management_lib.hpp"
#include "include/mpi_helper.hpp"

using arb::cell_gid_type;
using arb::cell_lid_type;
using arb::cell_size_type;
using arb::cell_member_type;
using arb::segment_location;

inline bool operator==(const source_type& lhs, const source_type& rhs) {
    return lhs.segment   == rhs.segment && lhs.position  == rhs.position && lhs.threshold == rhs.threshold;
}

inline bool operator==(const target_type& lhs, const target_type& rhs) {
    return lhs.position == rhs.position && lhs.segment == rhs.segment && lhs.synapse == rhs.synapse;
}

bool operator==(const arb::mechanism_desc &lhs, const arb::mechanism_desc &rhs)
{
    return lhs.name() == rhs.name() && lhs.values() == rhs.values();
}

namespace std {
    template<> struct hash<source_type>
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
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void database::build_source_and_target_maps(const std::vector<arb::group_description>& groups) {
    // Build loc_source_gids and loc_source_sizes
    std::vector<cell_gid_type> loc_source_gids;
    std::vector<unsigned> loc_source_sizes;
    std::vector<source_type> loc_sources;

    for (auto group: groups) {
        loc_source_gids.insert(loc_source_gids.end(), group.gids.begin(), group.gids.end());
        for (auto gid: group.gids) {
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
                        throw sonata_exception("source maps initialized incorrectly");
                    }
                }
                else {
                    throw sonata_exception("source maps initialized incorrectly");
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
                        throw sonata_exception("target maps initialized incorrectly");
                    }
                }
                else {
                    throw sonata_exception("target maps initialized incorrectly");
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
        tgt.push_back(std::make_pair(segment_location(t.first.segment, t.first.position), t.first.synapse));
    }
}

arb::morphology database::get_cell_morphology(cell_gid_type gid) {
    auto loc_node = localize_cell(gid);
    auto node_pop_id = loc_node.pop_id;
    auto node_id = loc_node.el_id;

    auto group_id = nodes_[node_pop_id].int_at("node_group_id", node_id);
    auto group_idx = nodes_[node_pop_id].int_at("node_group_index", node_id);

    auto node_type_tag = nodes_[node_pop_id].int_at("node_type_id", node_id);
    auto node_pop_name = nodes_[node_pop_id].name();

    if (nodes_[node_pop_id].find_group(std::to_string(group_id)) != -1) {
        auto lgi = nodes_[node_pop_id].find_group(std::to_string(group_id));
        auto group = nodes_[node_pop_id][lgi];
        if (group.find_dataset("morphology") != -1) {
            auto file = group.string_at("morphology", group_idx);

            std::ifstream f(file);
            if (!f) throw sonata_exception("Unable to open SWC file");
            return arb::swc_as_morphology(arb::parse_swc_file(f));
        }
    }
    return node_types_.morph(type_pop_id(node_type_tag, node_pop_name));
}

std::unordered_map<std::string, std::vector<arb::mechanism_desc>> database::get_density_mechs(cell_gid_type gid) {
    auto loc_node = localize_cell(gid);
    auto node_pop_id = loc_node.pop_id;
    auto node_id = loc_node.el_id;

    auto nodes_grp_id = nodes_[node_pop_id].int_at("node_group_id", node_id);
    auto nodes_grp_idx = nodes_[node_pop_id].int_at("node_group_index", node_id);

    auto nodes_type_tag = nodes_[node_pop_id].int_at("node_type_id", node_id);
    auto nodes_pop_name = nodes_[node_pop_id].name();

    auto density_vars = node_types_.dynamic_params(type_pop_id(nodes_type_tag, nodes_pop_name));

    for (auto id: density_vars) {
        auto gp_id = id.first;
        for (auto var: id.second) {
            auto var_id = var.first;
            auto gp_var_id = gp_id + "." + var_id;

            if (nodes_[node_pop_id].find_group(std::to_string(nodes_grp_id)) != -1) {
                auto lgi = nodes_[node_pop_id].find_group(std::to_string(nodes_grp_id));
                auto group = nodes_[node_pop_id][lgi];
                if (group.find_dataset(gp_var_id) != -1) {
                    auto value = group.double_at(gp_var_id, nodes_grp_idx);
                    density_vars[gp_id][var_id] = value;
                }

            }
        }
    }
    return node_types_.density_mech_desc(type_pop_id(nodes_type_tag, nodes_pop_name), std::move(density_vars));
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
    auto edges_type_tag = edges_[edge_pop_id].int_range("edge_type_id", edge_range.first, edge_range.second);
    auto edges_pop_name = edges_[edge_pop_id].name();

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
        auto e_fields = edge_types_.fields(type_pop_id(edges_type_tag[i], edges_pop_name));

        if (!found_source_branch) {
            source_branch = std::atoi(e_fields["efferent_section_id"].c_str());
        }
        if (!found_source_pos) {
            source_pos = std::atof(e_fields["efferent_section_pos"].c_str());
        }
        if (!found_threshold) {
            threshold = std::atof(e_fields["threshold"].c_str());
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
    auto edges_type_tag = edges_[edge_pop_id].int_range("edge_type_id", edge_range.first, edge_range.second);
    auto edges_pop_name = edges_[edge_pop_id].name();

    auto cat = arb::global_default_catalogue();

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
        auto e_fields = edge_types_.fields(type_pop_id(edges_type_tag[i], edges_pop_name));

        if (!found_target_branch) {
            if (e_fields.find("afferent_section_id") != e_fields.end()) {
                target_branch = std::atoi(e_fields["afferent_section_id"].c_str());
            } else {
                throw sonata_exception("Afferent Section ID missing");
            }
        }
        if (!found_target_pos) {
            if (e_fields.find("afferent_section_pos") != e_fields.end()) {
                target_pos = std::atof(e_fields["afferent_section_pos"].c_str());
            } else {
                throw sonata_exception("Afferent Section pos missing");
            }
        }
        if (!found_synapse) {
            if (e_fields.find("model_template") != e_fields.end()) {
                synapse = e_fields["model_template"];
            } else {
                throw sonata_exception("Model Template missing");
            }
        }

        // After finding the synapse, set the parameters
        std::unordered_map<std::string, double> syn_params;

        arb::mechanism_desc syn(synapse);
        auto mech = edge_types_.point_mech_desc(type_pop_id(edges_type_tag[i], edges_pop_name));

        if (mech.name() == synapse) {
            for (auto v: mech.values()) {
                syn.set(v.first, v.second);
            };
        }

        for (auto p: cat[synapse].parameters) {
            if (edges_[edge_pop_id].find_group(std::to_string(loc_grp_id)) != -1) {
                auto lgi = edges_[edge_pop_id].find_group(std::to_string(loc_grp_id));
                auto group = edges_[edge_pop_id][lgi];
                auto loc_grp_idx = edges_grp_idx[i];
                if (group.find_dataset(p.first) != -1) {
                    syn_params[p.first] = group.double_at(p.first, loc_grp_idx);
                }
            }
        }

        for (auto p: syn_params) {
            syn.set(p.first, p.second);
        }

        ret.emplace_back((unsigned)target_branch, target_pos, syn);
    }
    return ret;
}

std::vector<double> database::weight_range(unsigned edge_pop_id, std::pair<unsigned, unsigned> edge_range) {
    std::vector<double> ret;

    // First read edge_group_id and edge_group_index and edge_type
    auto edges_grp_id = edges_[edge_pop_id].int_range("edge_group_id", edge_range.first, edge_range.second);
    auto edges_grp_idx = edges_[edge_pop_id].int_range("edge_group_index", edge_range.first, edge_range.second);
    auto edges_type_tag = edges_[edge_pop_id].int_range("edge_type_id", edge_range.first, edge_range.second);
    auto edges_pop_name = edges_[edge_pop_id].name();

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
        auto e_fields = edge_types_.fields(type_pop_id(edges_type_tag[i], edges_pop_name));

        if (!found_weight) {
            if (e_fields.find("syn_weight") != e_fields.end()) {
                weight = std::atof(e_fields["syn_weight"].c_str());
            } else {
                throw sonata_exception("Synapse weight missing");
            }
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
    auto edges_type_tag = edges_[edge_pop_id].int_range("edge_type_id", edge_range.first, edge_range.second);
    auto edges_pop_name = edges_[edge_pop_id].name();

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
        auto e_fields = edge_types_.fields(type_pop_id(edges_type_tag[i], edges_pop_name));

        if (!found_delay) {

            if (e_fields.find("delay") != e_fields.end()) {
                delay = std::atof(e_fields["delay"].c_str());
            } else {
                throw sonata_exception("Synapse delay missing");
            }
        }
        ret.emplace_back(delay);
    }
    return ret;
}
