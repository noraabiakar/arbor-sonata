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

class sonata_recipe: public arb::recipe {
public:
    sonata_recipe(sonata_params params):
            model_desc_(params.network.nodes,
                       params.network.edges,
                       params.network.nodes_types,
                       params.network.edges_types),
            io_desc_(params.network.nodes,
                        params.spikes_input,
                        params.current_clamps,
                        params.probes_info),
            run_params_(params.run),
            sim_cond_(params.conditions),
            probe_info_(params.probes_info),
            num_cells_(model_desc_.num_cells()) {}

    cell_size_type num_cells() const override {
        return num_cells_;
    }

    void build_local_maps(const arb::domain_decomposition& decomp) {
        std::lock_guard<std::mutex> l(mtx_);
        model_desc_.build_source_and_target_maps(decomp.groups);
    }

    arb::util::unique_any get_cell_description(cell_gid_type gid) const override {
        if (get_cell_kind(gid) == cell_kind::cable) {
            std::vector<arb::mlocation> src_locs;
            std::vector<std::pair<arb::mlocation, arb::mechanism_desc>> tgt_types;

            std::lock_guard<std::mutex> l(mtx_);
            auto morph = model_desc_.get_cell_morphology(gid);
            auto mechs = model_desc_.get_density_mechs(gid);

            model_desc_.get_sources_and_targets(gid, src_locs, tgt_types);

            std::vector<std::pair<arb::mlocation, double>> src_types;
            for (auto s: src_locs) {
                src_types.push_back(std::make_pair(s, run_params_.threshold));
            }

            auto cell = dummy_cell(morph, mechs, src_types, tgt_types);

            auto stims = io_desc_.get_current_clamps(gid);
            for (auto s: stims) {
                arb::i_clamp stim(s.delay, s.duration, s.amplitude);
                cell.place(s.stim_loc, stim);
            }

            return cell;
        }
        else if (get_cell_kind(gid) == cell_kind::spike_source) {
            std::lock_guard<std::mutex> l(mtx_);
            std::vector<double> time_sequence = io_desc_.get_spikes(gid);
            return arb::util::unique_any(arb::spike_source_cell{arb::explicit_schedule(time_sequence)});
        }
        return {};
    }

    cell_kind get_cell_kind(cell_gid_type gid) const override {
        std::lock_guard<std::mutex> l(mtx_);
        return model_desc_.get_cell_kind(gid);
    }

    cell_size_type num_sources(cell_gid_type gid) const override {
        std::lock_guard<std::mutex> l(mtx_);
        return model_desc_.num_sources(gid);
    }

    cell_size_type num_targets(cell_gid_type gid) const override {
        std::lock_guard<std::mutex> l(mtx_);
        return model_desc_.num_targets(gid);
    }

    std::vector<arb::cell_connection> connections_on(cell_gid_type gid) const override {
        std::vector<arb::cell_connection> conns;

        std::lock_guard<std::mutex> l(mtx_);
        model_desc_.get_connections(gid, conns);

        return conns;
    }

    std::vector<arb::event_generator> event_generators(cell_gid_type gid) const override {
        std::vector<arb::event_generator> gens;
        return gens;
    }

    std::vector<arb::probe_info> get_probes(cell_gid_type gid) const override {
        return io_desc_.get_probe(gid);
    }

    std::unordered_map<std::string, std::vector<cell_gid_type>> get_probe_groups() {
        return io_desc_.get_probe_groups();
    }

    arb::util::any get_global_properties(cell_kind k) const override {
        arb::cable_cell_global_properties gprop;
        gprop.default_parameters = arb::neuron_parameter_defaults;
        gprop.default_parameters.temperature_K = sim_cond_.temp_c + 273.15;
        gprop.default_parameters.init_membrane_potential = sim_cond_.v_init;
        return gprop;
    }

    std::vector<unsigned> get_pop_partitions() const {
        return model_desc_.pop_partitions();
    }

    std::vector<std::string> get_pop_names() const {
        return model_desc_.pop_names();
    }

private:
    mutable std::mutex mtx_;
    mutable model_desc model_desc_;
    mutable io_desc io_desc_;

    run_params run_params_;
    sim_conditions sim_cond_;
    std::vector<probe_info> probe_info_;

    cell_size_type num_cells_;
};