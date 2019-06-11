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

#include "parameters.hpp"
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

// Generate a cell.
arb::cable_cell dummy_cell(
        arb::morphology,
        std::unordered_map<std::string, std::vector<arb::mechanism_desc>>,
        std::vector<std::pair<arb::segment_location, double>>,
        std::vector<std::pair<arb::segment_location, arb::mechanism_desc>>);

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

int main(int argc, char **argv)
{
    try {
        bool root = true;

        arb::proc_allocation resources;
        if (auto nt = arbenv::get_env_num_threads()) {
            resources.num_threads = nt;
        }
        else {
            resources.num_threads = arbenv::thread_concurrency();
        }

#ifdef ARB_MPI_ENABLED
        arbenv::with_mpi guard(argc, argv, false);
        resources.gpu_id = arbenv::find_private_gpu(MPI_COMM_WORLD);
        auto context = arb::make_context(resources, MPI_COMM_WORLD);
        root = arb::rank(context) == 0;
#else
        resources.gpu_id = arbenv::default_gpu();
        auto context = arb::make_context(resources);
#endif

#ifdef ARB_PROFILE_ENABLED
        arb::profile::profiler_initialize(context);
#endif

        // Print a banner with information about hardware configuration
        std::cout << "gpu:      " << (has_gpu(context)? "yes": "no") << "\n";
        std::cout << "threads:  " << num_threads(context) << "\n";
        std::cout << "mpi:      " << (has_mpi(context)? "yes": "no") << "\n";
        std::cout << "ranks:    " << num_ranks(context) << "\n" << std::endl;

        auto params = read_options(argc, argv);

        arb::profile::meter_manager meters;
        meters.start(context);

        // Create an instance of our recipe.
        sonata_recipe recipe(params);

        auto decomp = arb::partition_load_balance(recipe, context);

        recipe.build_local_maps(decomp);

        // Construct the model.
        arb::simulation sim(recipe, decomp, context);

        // Set up the probes that will measure voltages in the cells.
        std::unordered_map<cell_member_type, trace_info> traces;
        std::unordered_map<std::string, std::vector<cell_member_type>> trace_groups;
        auto sched = arb::regular_schedule(0.1);

        for (auto g : decomp.groups) {
            for (auto i : g.gids) {
                auto num_probes = recipe.num_probes(i);
                for (unsigned j = 0; j < num_probes; j++) {
                    auto probe = recipe.get_probe({i,j});
                    auto probe_address = arb::util::any_cast<cell_probe_address>(probe.address);

                    trace_info t;

                    t.is_voltage = probe_address.kind == cell_probe_address::membrane_voltage;
                    t.seg_id = probe_address.location.segment;
                    t.seg_pos = probe_address.location.position;

                    trace_groups[recipe.get_probe_file({i,j})].push_back({i,j});
                    traces[{i, j}] = t;
                    sim.add_sampler(arb::one_probe({i, j}), sched, arb::make_simple_sampler(traces[{i, j}].data));
                }
            }
        }

        // Set up recording of spikes to a vector on the root process.
        std::vector<arb::spike> recorded_spikes;
        if (root) {
            sim.set_global_spike_callback(
                    [&recorded_spikes](const std::vector<arb::spike>& spikes) {
                        recorded_spikes.insert(recorded_spikes.end(), spikes.begin(), spikes.end());
                    });
        }

        meters.checkpoint("model-init", context);

        std::cout << "running simulation" << std::endl;
        // Run the simulation for 100 ms, with time steps of 0.025 ms.
        sim.run(params.run.duration, params.run.dt);

        meters.checkpoint("model-run", context);

        auto ns = sim.num_spikes();


        // Write spikes to file
        if (root) {
            std::cout << "\n" << ns << " spikes generated \n";
            write_spikes(recorded_spikes, params.spike_output.sort_by == "time", params.spike_output.file_name, recipe.get_pop_names(), recipe.get_pop_partitions());
        }

        // Write the samples to a json file.
        if (root) write_trace(traces, trace_groups, recipe.get_pop_names(), recipe.get_pop_partitions());

        auto report = arb::profile::make_meter_report(meters, context);
        std::cout << report;

        auto summary = arb::profile::profiler_summary();
        std::cout << summary << "\n";

    }
    catch (std::exception& e) {
        std::cerr << "exception caught in SONATA miniapp: " << e.what() << "\n";
        return 1;
    }
    return 0;
}


arb::cable_cell dummy_cell(
        arb::morphology morph,
        std::unordered_map<std::string, std::vector<arb::mechanism_desc>> mechs,
        std::vector<std::pair<arb::segment_location, double>> detectors,
        std::vector<std::pair<arb::segment_location, arb::mechanism_desc>> synapses) {

    arb::cable_cell cell = arb::make_cable_cell(morph);

    for (auto& segment: cell.segments()) {
        switch (segment->kind()) {
            case arb::section_kind::soma : {
                segment->rL = 100;
                for (auto mech: mechs["soma"]) {
                    segment->add_mechanism(mech);
                }
                break;
            }
            case arb::section_kind::axon : {
                for (auto mech: mechs["axon"]) {
                    segment->add_mechanism(mech);
                }
                segment->set_compartments(200);
                break;
            }
            case arb::section_kind::dendrite : {
                for (auto mech: mechs["dend"]) {
                    segment->add_mechanism(mech);
                }
                segment->set_compartments(200);
                break;
            }
        }
    }

    // Add spike threshold detector at the soma.
    for (auto d: detectors) {
        cell.add_detector(d.first, d.second);
    }

    for (auto s: synapses) {
        cell.add_synapse(s.first, s.second);
    }

    return cell;
}
