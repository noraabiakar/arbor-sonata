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

#include "../sonata/include/sonata_io.hpp"
#include "../sonata/include/sonata_recipe.hpp"
#include "../sonata/include/sonata_cell.hpp"

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
        auto sched = arb::regular_schedule(0.1);


        for (unsigned gid = 0; gid < recipe.num_cells(); gid++) {
            auto pbs = recipe.get_probes_info(gid);
            for (auto p:pbs) {
                trace_info t(p.info.is_voltage, p.info.loc);
                traces[{gid, p.idx}] = t;

                sim.add_sampler(arb::one_probe({gid, p.idx}), sched, arb::make_simple_sampler(traces[{gid, p.idx}].data));
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
        if (root) write_trace(traces, recipe.get_probe_groups(), recipe.get_pop_names(), recipe.get_pop_partitions());

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
