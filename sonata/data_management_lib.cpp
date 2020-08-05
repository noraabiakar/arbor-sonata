#include <arbor/version.hpp>
#include <arbor/mechcat.hpp>

#include "include/data_management_lib.hpp"
#include "mpi_helper.hpp"

using arb::cell_gid_type;
using arb::cell_lid_type;
using arb::cell_size_type;
using arb::cell_member_type;
using arb::mlocation;

model_desc::model_desc(h5_record nodes,
                       h5_record edges,
                       csv_node_record node_types,
                       csv_edge_record edge_types):
nodes_(nodes), edges_(edges), node_types_(node_types), edge_types_(edge_types) {}


cell_size_type model_desc::num_cells() const {
    return nodes_.num_elements();
}

cell_size_type model_desc::num_sources(cell_gid_type gid) const {
    return source_maps_.at(gid).size();
}

cell_size_type model_desc::num_targets(cell_gid_type gid) const {
    return target_maps_.at(gid).size();
}

std::vector<unsigned> model_desc::pop_partitions() const {
    return nodes_.partitions();
}

std::vector<std::string> model_desc::pop_names() const {
    return nodes_.pop_names();
}

std::string model_desc::population_of(cell_gid_type gid) const {
    for (unsigned i = 0; i < nodes_.partitions().size(); i++) {
        if (gid < nodes_.partitions()[i]) {
            return nodes_.populations()[i-1].name();
        }
    }
    return {};
}

unsigned model_desc::population_id_of(cell_gid_type gid) const {
    for (unsigned i = 0; i < nodes_.partitions().size(); i++) {
        if (gid < nodes_.partitions()[i]) {
            return gid - nodes_.partitions()[i-1];
        }
    }
    return {};
}

void model_desc::build_source_and_target_maps(const std::vector<arb::group_description>& groups) {
    // Build loc_source_gids and loc_source_sizes
    std::vector<cell_gid_type> loc_source_gids;
    std::vector<unsigned> loc_source_sizes;
    std::vector<source_type> loc_sources;

    for (auto group: groups) {
        loc_source_gids.insert(loc_source_gids.end(), group.gids.begin(), group.gids.end());
        for (auto gid: group.gids) {
            std::unordered_set<source_type> src_set;
            std::vector<std::pair<target_type, unsigned>> tgt_vec;

            auto loc_node = nodes_.localize(gid);
            auto source_edge_pops = edge_types_.edges_of_source(loc_node.pop_name);
            auto target_edge_pops = edge_types_.edges_of_target(loc_node.pop_name);

            for (auto edge_pop_name: source_edge_pops) {
                if (edges_.find_population(edge_pop_name)) {
                    auto edge_pop = edges_.map()[edge_pop_name];
                    auto ind_id = edges_[edge_pop].find_group("indicies");
                    auto s2t_id = edges_[edge_pop][ind_id].find_group("source_to_target");
                    auto n2r_range = edges_[edge_pop][ind_id][s2t_id].get<std::pair<int,int>>("node_id_to_ranges", loc_node.el_id);

                    for (auto j = n2r_range.first; j < n2r_range.second; j++) {
                        auto r2e = edges_[edge_pop][ind_id][s2t_id].get<std::pair<int,int>>("range_to_edge_id", j);
                        auto src_rng = source_range(edge_pop, r2e);
                        for (auto s: src_rng) {
                            auto loc = src_set.find(s);
                            if (loc == src_set.end()) {
                                src_set.insert(s);
                            }
                        }
                    }
                }
            }

            for (auto edge_pop_name: target_edge_pops) {
                if (edges_.find_population(edge_pop_name)) {
                    auto edge_pop = edges_.map()[edge_pop_name];

                    auto ind_id = edges_[edge_pop].find_group("indicies");
                    auto t2s_id = edges_[edge_pop][ind_id].find_group("target_to_source");
                    auto n2r = edges_[edge_pop][ind_id][t2s_id].get<std::pair<int,int>>("node_id_to_ranges", loc_node.el_id);
                    for (auto j = n2r.first; j < n2r.second; j++) {
                        auto r2e = edges_[edge_pop][ind_id][t2s_id].get<std::pair<int,int>>("range_to_edge_id", j);

                        auto tgt_rng = target_range(edge_pop, r2e);

                        std::vector<unsigned> edge_rng(r2e.second - r2e.first);
                        std::iota(edge_rng.begin(), edge_rng.end(), r2e.first);

                        for (unsigned k = 0; k < tgt_rng.size(); k++) {
                            tgt_vec.push_back(std::make_pair(tgt_rng[k], edges_.globalize({edge_pop_name, (cell_gid_type) edge_rng[k]})));
                        }
                    }
                }
            }

            // Build loc_sources
            std::vector<source_type> src_vec(src_set.begin(), src_set.end());
            std::sort(src_vec.begin(), src_vec.end(), [](const auto &a, const auto& b) -> bool
            {
                return std::tie(a.segment, a.position) < std::tie(b.segment, b.position);
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

void model_desc::get_sources_and_targets(cell_gid_type gid, std::vector<mlocation>& src,
        std::vector<std::pair<mlocation, arb::mechanism_desc>>& tgt) const {
    src.reserve(source_maps_.at(gid).size());
    for (auto s: source_maps_.at(gid)) {
        src.push_back(mlocation{s.segment, s.position});
    }

    tgt.reserve(target_maps_.at(gid).size());
    for (auto t: target_maps_.at(gid)) {
        tgt.push_back(std::make_pair(mlocation{t.first.segment, t.first.position}, t.first.synapse));
    }
}

arb::morphology model_desc::get_cell_morphology(cell_gid_type gid) {
    auto loc_node = nodes_.localize(gid);

    auto node_pop_name = loc_node.pop_name;
    auto node_pop_id = nodes_.map()[node_pop_name];
    auto node_id = loc_node.el_id;

    auto group_id = nodes_[node_pop_id].get<int>("node_group_id", node_id);
    auto group_idx = nodes_[node_pop_id].get<int>("node_group_index", node_id);

    auto node_type_tag = nodes_[node_pop_id].get<int>("node_type_id", node_id);

    if (nodes_[node_pop_id].find_group(std::to_string(group_id)) != -1) {
        auto lgi = nodes_[node_pop_id].find_group(std::to_string(group_id));
        auto group = nodes_[node_pop_id][lgi];
        if (group.find_dataset("morphology") != -1) {
            auto file = group.get<std::string>("morphology", group_idx);

            std::ifstream f(file);
            if (!f) throw sonata_exception("Unable to open SWC file");
            return arb::morphology(arb::swc_as_segment_tree(arb::parse_swc_file(f)));
        }
    }
    return node_types_.morph(type_pop_id(node_type_tag, node_pop_name));
}

arb::cell_kind model_desc::get_cell_kind(cell_gid_type gid) {
    auto loc_node = nodes_.localize(gid);

    auto node_pop_name = loc_node.pop_name;
    auto node_pop_id = nodes_.map()[node_pop_name];
    auto node_id = loc_node.el_id;

    auto node_type_tag = nodes_[node_pop_id].get<int>("node_type_id", node_id);

    return node_types_.cell_kind(type_pop_id(node_type_tag, node_pop_name));
}

void model_desc::get_connections(cell_gid_type gid, std::vector<arb::cell_connection>& conns) {
    // Find cell local index in population
    auto loc_node = nodes_.localize(gid);
    auto edge_to_source = edge_types_.edge_to_source_of_target(loc_node.pop_name);

    for (auto i: edge_to_source) {
        auto source_pop_name = i.second;
        auto edge_pop_name = i.first;
        if (edges_.find_population(edge_pop_name)) {
            auto edge_pop = edges_.map()[i.first];
            if (!nodes_.find_population(source_pop_name)) {
                throw sonata_exception("source population of edge population not available");
            }
            auto source_pop = nodes_.map()[i.second];

            auto ind_id = edges_[edge_pop].find_group("indicies");
            auto s2t_id = edges_[edge_pop][ind_id].find_group("target_to_source");
            auto n2r_range = edges_[edge_pop][ind_id][s2t_id].get<std::pair<int,int>>("node_id_to_ranges", loc_node.el_id);

            for (auto j = n2r_range.first; j < n2r_range.second; j++) {
                auto r2e = edges_[edge_pop][ind_id][s2t_id].get<std::pair<int,int>>("range_to_edge_id", j);
                auto src_rng = source_range(edge_pop, r2e);
                auto tgt_rng = target_range(edge_pop, r2e);
                auto weights = weight_range(edge_pop, r2e);
                auto delays = delay_range(edge_pop, r2e);

                auto src_id = edges_[edge_pop].get<std::vector<int>>("source_node_id", r2e.first, r2e.second);

                std::vector<cell_member_type> sources, targets;

                for (unsigned s = 0; s < src_rng.size(); s++) {
                    auto source_gid = nodes_.globalize({source_pop_name, (cell_gid_type) src_id[s]});

                    auto loc = std::lower_bound(source_maps_[source_gid].begin(), source_maps_[source_gid].end(),
                                                src_rng[s],
                                                [](const auto &lhs, const auto &rhs) -> bool {
                                                    return std::tie(lhs.segment, lhs.position) <
                                                           std::tie(rhs.segment, rhs.position);
                                                });

                    if (loc != source_maps_[source_gid].end()) {
                        if (*loc == src_rng[s]) {
                            unsigned index = loc - source_maps_[source_gid].begin();
                            sources.push_back({source_gid, index});
                        } else {
                            throw sonata_exception("source maps initialized incorrectly");
                        }
                    } else {
                        throw sonata_exception("source maps initialized incorrectly");
                    }
                }

                unsigned e = 0;
                for (unsigned t = r2e.first; t < r2e.second; t++, e++) {
                    auto loc = std::lower_bound(target_maps_[gid].begin(), target_maps_[gid].end(),
                                                std::make_pair(tgt_rng[e],
                                                               edges_.globalize({edge_pop_name, (cell_gid_type) t})),
                                                [](const auto &lhs, const auto &rhs) -> bool {
                                                    return lhs.second < rhs.second;
                                                });

                    if (loc != target_maps_[gid].end()) {
                        if ((*loc).second == edges_.globalize({edge_pop_name, (cell_gid_type) t})) {
                            unsigned index = loc - target_maps_[gid].begin();
                            targets.push_back({gid, index});
                        } else {
                            throw sonata_exception("target maps initialized incorrectly");
                        }
                    } else {
                        throw sonata_exception("target maps initialized incorrectly");
                    }
                }

                for (unsigned k = 0; k < sources.size(); k++) {
                    conns.emplace_back(sources[k], targets[k], weights[k], delays[k]);
                }
            }
        }
    }
}

std::unordered_map<section_kind, std::vector<arb::mechanism_desc>> model_desc::get_density_mechs(cell_gid_type gid) {
    auto loc_node = nodes_.localize(gid);
    auto node_pop_name = loc_node.pop_name;
    auto node_pop_id = nodes_.map()[node_pop_name];
    auto node_id = loc_node.el_id;

    auto nodes_grp_id = nodes_[node_pop_id].get<int>("node_group_id", node_id);
    auto nodes_grp_idx = nodes_[node_pop_id].get<int>("node_group_index", node_id);

    auto nodes_type_tag = nodes_[node_pop_id].get<int>("node_type_id", node_id);
    auto nodes_pop_name = nodes_[node_pop_id].name();

    auto node_unique_id = type_pop_id(nodes_type_tag, nodes_pop_name);

    auto density_vars = node_types_.dynamic_params(node_unique_id);

    for (auto id: density_vars) {
        auto gp_id = id.first;
        for (auto var: id.second) {
            auto var_id = var.first;
            auto gp_var_id = gp_id + "." + var_id;

            if (nodes_[node_pop_id].find_group(std::to_string(nodes_grp_id)) != -1) {
                auto lgi = nodes_[node_pop_id].find_group(std::to_string(nodes_grp_id));
                auto group = nodes_[node_pop_id][lgi];
                if (group.find_group("dynamics_params") != -1) {
                    auto dpi = group.find_group("dynamics_params");
                    auto dyn_params = group[dpi];
                    if (dyn_params.find_dataset(gp_var_id) != -1) {
                        auto value = dyn_params.get<double>(gp_var_id, nodes_grp_idx);
                        density_vars[gp_id][var_id] = value;
                    }
                }
            }
        }
    }

    return node_types_.density_mech_desc(node_unique_id, std::move(density_vars));
}

// Private helper functions

// Read from HDF5 file/ CSV file depending on where the information is available

std::vector<source_type> model_desc::source_range(unsigned edge_pop_id, std::pair<unsigned, unsigned> edge_range) {
    std::vector<source_type> ret;

    // First get edge_group_id and edge_group_index and edge_type
    auto edges_grp_id = edges_[edge_pop_id].get<std::vector<int>>("edge_group_id", edge_range.first, edge_range.second);
    auto edges_grp_idx = edges_[edge_pop_id].get<std::vector<int>>("edge_group_index", edge_range.first, edge_range.second);
    auto edges_type_tag = edges_[edge_pop_id].get<std::vector<int>>("edge_type_id", edge_range.first, edge_range.second);
    auto edges_pop_name = edges_[edge_pop_id].name();

    for (unsigned i = 0; i < edges_grp_id.size(); i++) {
        auto loc_grp_id = edges_grp_id[i];

        int source_branch;
        double source_pos;

        bool found_source_branch = false;
        bool found_source_pos = false;

        // if the edges are in groups, for each edge find the group, if it exists
        if (edges_[edge_pop_id].find_group(std::to_string(loc_grp_id)) != -1) {
            auto lgi = edges_[edge_pop_id].find_group(std::to_string(loc_grp_id));
            auto group = edges_[edge_pop_id][lgi];
            auto loc_grp_idx = edges_grp_idx[i];

            if (group.find_dataset("efferent_section_id") != -1) {
                source_branch = group.get<int>("efferent_section_id", loc_grp_idx);
                found_source_branch = true;
            }
            if (group.find_dataset("efferent_section_pos") != -1) {
                source_pos = group.get<double>("efferent_section_pos", loc_grp_idx);
                found_source_pos = true;
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

        ret.emplace_back((unsigned)source_branch, source_pos);
    }
    return ret;
}

std::vector<target_type> model_desc::target_range(unsigned edge_pop_id, std::pair<unsigned, unsigned> edge_range) {
    std::vector<target_type> ret;

    // First read edge_group_id and edge_group_index and edge_type
    auto edges_grp_id = edges_[edge_pop_id].get<std::vector<int>>("edge_group_id", edge_range.first, edge_range.second);
    auto edges_grp_idx = edges_[edge_pop_id].get<std::vector<int>>("edge_group_index", edge_range.first, edge_range.second);
    auto edges_type_tag = edges_[edge_pop_id].get<std::vector<int>>("edge_type_id", edge_range.first, edge_range.second);
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
                target_branch = group.get<int>("afferent_section_id", loc_grp_idx);
                found_target_branch = true;
            }
            if (group.find_dataset("afferent_section_pos") != -1) {
                target_pos = group.get<double>("afferent_section_pos", loc_grp_idx);
                found_target_pos = true;
            }
            if (group.find_dataset("model_template") != -1) {
                synapse = group.get<std::string>("model_template", loc_grp_idx);
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
            auto lgi = edges_[edge_pop_id].find_group(std::to_string(loc_grp_id));
            if (lgi != -1) {
                auto group = edges_[edge_pop_id][lgi];
                auto loc_grp_idx = edges_grp_idx[i];

                auto dpi = group.find_group("dynamics_params");
                if (dpi != -1) {
                    auto dyn_params = group[dpi];
                    if (dyn_params.find_dataset(p.first) != -1) {
                        syn_params[p.first] = dyn_params.get<double>(p.first, loc_grp_idx);
                    }
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

std::vector<double> model_desc::weight_range(unsigned edge_pop_id, std::pair<unsigned, unsigned> edge_range) {
    std::vector<double> ret;

    // First read edge_group_id and edge_group_index and edge_type
    auto edges_grp_id = edges_[edge_pop_id].get<std::vector<int>>("edge_group_id", edge_range.first, edge_range.second);
    auto edges_grp_idx = edges_[edge_pop_id].get<std::vector<int>>("edge_group_index", edge_range.first, edge_range.second);
    auto edges_type_tag = edges_[edge_pop_id].get<std::vector<int>>("edge_type_id", edge_range.first, edge_range.second);
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
                weight = group.get<double>("syn_weight", loc_grp_idx);
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

std::vector<double> model_desc::delay_range(unsigned edge_pop_id, std::pair<unsigned, unsigned> edge_range) {
    std::vector<double> ret;

    // First read edge_group_id and edge_group_index and edge_type
    auto edges_grp_id = edges_[edge_pop_id].get<std::vector<int>>("edge_group_id", edge_range.first, edge_range.second);
    auto edges_grp_idx = edges_[edge_pop_id].get<std::vector<int>>("edge_group_index", edge_range.first, edge_range.second);
    auto edges_type_tag = edges_[edge_pop_id].get<std::vector<int>>("edge_type_id", edge_range.first, edge_range.second);
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
                delay = group.get<double>("delay", loc_grp_idx);
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
io_desc::io_desc(h5_record nodes,
                       std::vector<spike_in_info> spikes,
                       std::vector<current_clamp_info> current_clamp,
                       std::vector<probe_info> probes) : nodes_(nodes){
    build_spike_map(spikes);
    build_current_clamp_map(current_clamp);
    build_probe_map(probes);
};

void io_desc::build_spike_map(std::vector<spike_in_info> spikes) {
    for (unsigned gid = 0; gid < nodes_.num_elements(); gid++) {
        std::vector<double> spike_times;

        auto loc_cell = nodes_.localize(gid);

        for (auto sp: spikes) {
            if (loc_cell.pop_name != sp.population) {
                continue;
            }

            auto spike_idx = sp.data.find_group("spikes");
            if (spike_idx == -1) {
                throw sonata_exception("Input spikes file doesn't have top level group \"spikes\"");
            }
            auto range = sp.data[spike_idx].get<std::pair<int,int>>("gid_to_range", loc_cell.el_id);

            auto spk_times = sp.data[spike_idx].get<std::vector<double>>("timestamps", range.first, range.second);

            spike_times.insert(spike_times.end(), spk_times.begin(), spk_times.end());
        }

        std::sort(spike_times.begin(), spike_times.end());
        spike_map_.insert({gid, std::move(spike_times)});
    }
}

void io_desc::build_current_clamp_map(std::vector<current_clamp_info> current) {

    struct param_info {
        double dur;
        double amp;
        double delay;
    };

    struct loc_info {
        cell_gid_type gid;
        std::string population;
        unsigned seg;
        double pos;
    };
    for (auto curr_clamp : current){
        std::unordered_map<unsigned, param_info> param_map;
        std::unordered_map<unsigned, loc_info> loc_map;

        auto stim_param_data = curr_clamp.stim_params.get_data();
        auto stim_param_cols = stim_param_data.front();

        for(auto it = stim_param_data.begin()+1; it < stim_param_data.end(); it++) {
            loc_info loc;
            unsigned pos = 0, id;

            for (auto field: *it) {
                if(stim_param_cols[pos] == "electrode_id") {
                    id = std::atoi(field.c_str());
                } else if (stim_param_cols[pos] == "node_id") {
                    loc.gid = std::atoi(field.c_str());
                } else if (stim_param_cols[pos] == "population") {
                    loc.population = field;
                } else if (stim_param_cols[pos] == "sec_id") {
                    loc.seg = std::atoi(field.c_str());
                } else if (stim_param_cols[pos] == "seg_x") {
                    loc.pos = std::atof(field.c_str());
                }
                pos++;
            }
            loc_map[id] = loc;
        }

        auto stim_loc_data = curr_clamp.stim_loc.get_data();
        auto stim_loc_cols = stim_loc_data.front();

        for(auto it = stim_loc_data.begin()+1; it < stim_loc_data.end(); it++) {
            param_info param;
            unsigned pos = 0, id;

            for (auto field: *it) {
                if(stim_loc_cols[pos] == "electrode_id") {
                    id = std::atoi(field.c_str());
                } else if (stim_loc_cols[pos] == "dur") {
                    param.dur = std::atof(field.c_str());
                } else if (stim_loc_cols[pos] == "amp") {
                    param.amp = std::atof(field.c_str());
                } else if (stim_loc_cols[pos] == "delay") {
                    param.delay = std::atof(field.c_str());
                }
                pos++;
            }
            param_map[id] = param;
        }

        for (auto i: loc_map) {
            if (param_map.find(i.first) != param_map.end()) {
                auto params = param_map.at(i.first);

                auto local_loc = i.second;
                auto global_gid = nodes_.globalize({local_loc.population, local_loc.gid});

                current_clamp_map_[global_gid].emplace_back(params.dur, params.amp, params.delay, arb::mlocation{local_loc.seg, local_loc.pos});
            }
            else {
                throw sonata_exception("Electrode id has no corresponding input description");
            }
        };
    }
}

void io_desc::build_probe_map(std::vector<probe_info> probes) {
    std::unordered_map<cell_gid_type, cell_size_type> probe_count;

    for (auto probe: probes) {
        for (auto i: probe.node_ids) {
            auto gid = nodes_.globalize({probe.population, i});

            if (probe_count.find(gid) == probe_count.end()) {
                probe_count[gid] = 0;
            }
            cell_lid_type idx = probe_count[gid]++;
            mlocation loc = {probe.sec_id, probe.sec_pos};

            if (probe.kind == "v") {
                probe_map_[gid].emplace_back(idx, trace_info(true, loc));
            } else {
                probe_map_[gid].emplace_back(idx, trace_info(false, loc));
            };
            probe_groups_[probe.file_name].push_back({gid, idx});
        }
    }
}

std::vector<current_clamp_desc> io_desc::get_current_clamps(cell_gid_type gid) const {
    if (current_clamp_map_.find(gid) != current_clamp_map_.end()) {
        return current_clamp_map_.at(gid);
    }
    return {};
};

std::vector<double> io_desc::get_spikes(cell_gid_type gid) const {
    if (spike_map_.find(gid) != spike_map_.end()) {
        return spike_map_.at(gid);
    }
    return {};
};

std::vector<trace_index_and_info> io_desc::get_probes(cell_gid_type gid) const {
    if (probe_map_.find(gid) != probe_map_.end()) {
        return probe_map_.at(gid);
    }
    return {};
};

std::unordered_map<std::string, std::vector<cell_member_type>> io_desc::get_probe_groups() const {
    return probe_groups_;
};
