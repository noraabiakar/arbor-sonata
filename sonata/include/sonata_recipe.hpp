#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>

#include <arbor/assert_macro.hpp>
#include <arbor/common_types.hpp>
#include <arbor/context.hpp>
#include <arbor/load_balance.hpp>
#include <arbor/cable_cell.hpp>
#include <arbor/spike_source_cell.hpp>
#include <arbor/profile/meter_manager.hpp>
#include <arbor/profile/profiler.hpp>
#include <arbor/simple_sampler.hpp>
#include <arbor/simulation.hpp>
#include <arbor/recipe.hpp>
#include <arbor/version.hpp>

#include <arborenv/concurrency.hpp>
#include <arborenv/gpu_env.hpp>

#include "sonata_io.hpp"
#include "sonata_cell.hpp"
#include "data_management_lib.hpp"

#ifdef ARB_MPI_ENABLED
#include <mpi.h>
#include <arborenv/with_mpi.hpp>
#endif

using arb::cell_gid_type;
using arb::cell_lid_type;
using arb::cell_size_type;
using arb::cell_member_type;
using arb::cell_kind;
using arb::time_type;
using arb::cell_probe_address;

class sonata_recipe: public arb::recipe {
public:
    sonata_recipe(sonata_params params):
            database_(params.network.nodes,
                      params.network.edges,
                      params.network.nodes_types,
                      params.network.edges_types,
                      params.spikes_input,
                      params.current_clamps),
            run_params_(params.run),
            sim_cond_(params.conditions),
            probe_info_(params.probes_info),
            num_cells_(database_.num_cells()) {}

    cell_size_type num_cells() const override {
        return num_cells_;
    }

    void build_local_maps(const arb::domain_decomposition& decomp) {
        std::lock_guard<std::mutex> l(mtx_);
        database_.build_source_and_target_maps(decomp.groups);
    }

    arb::util::unique_any get_cell_description(cell_gid_type gid) const override {
        if (get_cell_kind(gid) == cell_kind::cable) {
            std::vector<arb::segment_location> src_locs;
            std::vector<std::pair<arb::segment_location, arb::mechanism_desc>> tgt_types;

            std::lock_guard<std::mutex> l(mtx_);
            auto morph = database_.get_cell_morphology(gid);
            auto mechs = database_.get_density_mechs(gid);

            database_.get_sources_and_targets(gid, src_locs, tgt_types);

            std::vector<std::pair<arb::segment_location, double>> src_types;
            for (auto s: src_locs) {
                src_types.push_back(std::make_pair(s, run_params_.threshold));
            }

            auto cell = dummy_cell(morph, mechs, src_types, tgt_types);

            auto stims = database_.get_current_clamps(gid);
            for (auto s: stims) {
                arb::i_clamp stim(s.delay, s.duration, s.amplitude);
                cell.add_stimulus(s.stim_loc, stim);
            }

            return cell;
        }
        else if (get_cell_kind(gid) == cell_kind::spike_source) {
            std::lock_guard<std::mutex> l(mtx_);
            std::vector<double> time_sequence = database_.get_spikes(gid);
            return arb::util::unique_any(arb::spike_source_cell{arb::explicit_schedule(time_sequence)});
        }
    }

    cell_kind get_cell_kind(cell_gid_type gid) const override {
        std::lock_guard<std::mutex> l(mtx_);
        return database_.get_cell_kind(gid);
    }

    cell_size_type num_sources(cell_gid_type gid) const override {
        std::lock_guard<std::mutex> l(mtx_);
        return database_.num_sources(gid);
    }

    cell_size_type num_targets(cell_gid_type gid) const override {
        std::lock_guard<std::mutex> l(mtx_);
        return database_.num_targets(gid);
    }

    std::vector<arb::cell_connection> connections_on(cell_gid_type gid) const override {
        std::vector<arb::cell_connection> conns;

        std::lock_guard<std::mutex> l(mtx_);
        database_.get_connections(gid, conns);

        return conns;
    }

    std::vector<arb::event_generator> event_generators(cell_gid_type gid) const override {
        std::vector<arb::event_generator> gens;
        return gens;
    }

    cell_size_type num_probes(cell_gid_type gid)  const override {
        std::lock_guard<std::mutex> l(mtx_);
        std::string population = database_.population_of(gid);

        unsigned sum = 0;
        for (auto p: probe_info_) {
            if (population == p.population) {
                if (p.node_ids.empty()) {
                    sum++;
                } else {
                    if (std::binary_search(p.node_ids.begin(), p.node_ids.end(), database_.population_id_of(gid))) {
                        sum++;
                    }
                }
            }
        }
        return sum;
    }

    arb::probe_info get_probe(cell_member_type id) const override {
        std::lock_guard<std::mutex> l(mtx_);
        std::string population = database_.population_of(id.gid);

        unsigned loc = 0;
        for (auto p: probe_info_) {
            if (population == p.population) {
                if (p.node_ids.empty() || std::binary_search(p.node_ids.begin(), p.node_ids.end(), database_.population_id_of(id.gid))) {
                    if (loc == id.index) {
                        // Get the appropriate kind for measuring voltage.
                        cell_probe_address::probe_kind kind;
                        if (p.kind == "v") {
                            kind = cell_probe_address::membrane_voltage;
                        } else if (p.kind == "i") {
                            kind = cell_probe_address::membrane_current;
                        } else {
                            throw sonata_exception("Probe kind not supported");
                        }
                        arb::segment_location loc(p.sec_id, p.sec_pos);

                        probe_files_[id] = p.file_name;
                        return arb::probe_info{id, kind, cell_probe_address{loc, kind}};
                    }
                    loc++;
                }
            }
        }
    }

    arb::util::any get_global_properties(cell_kind k) const override {
        arb::cable_cell_global_properties a;
        a.temperature_K = sim_cond_.temp_c + 273.15;
        a.init_membrane_potential_mV = sim_cond_.v_init;
        return a;
    }

    std::string get_probe_file(cell_member_type id) const {
        return probe_files_[id];
    }

    std::vector<unsigned> get_pop_partitions() const {
        return database_.pop_partitions();
    }

    std::vector<std::string> get_pop_names() const {
        return database_.pop_names();
    }

private:
    mutable std::mutex mtx_;
    mutable database database_;

    run_params run_params_;
    sim_conditions sim_cond_;
    std::vector<probe_info> probe_info_;

    mutable std::unordered_map<cell_member_type, std::string> probe_files_;
    cell_size_type num_cells_;
};