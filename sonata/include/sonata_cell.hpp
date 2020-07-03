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
#include "data_management_lib.hpp"
#include "sonata_cell.hpp"

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

// Generate a cell.
arb::cable_cell dummy_cell(
        arb::morphology morph,
        std::unordered_map<section_kind, std::vector<arb::mechanism_desc>> mechs,
        std::vector<std::pair<arb::mlocation, double>> detectors,
        std::vector<std::pair<arb::mlocation, arb::mechanism_desc>> synapses) {
    arb::label_dict d;

    using arb::reg::tagged;
    d.set("soma", tagged(1));
    d.set("axon", tagged(2));
    d.set("dend", join(tagged(3), tagged(4)));

    arb::cable_cell cell = arb::cable_cell(morph, d);

    for (auto mech: mechs[section_kind::soma]) {
        cell.paint("soma", mech);
    }
    for (auto mech: mechs[section_kind::dend]) {
        cell.paint("dend", mech);
    }
    for (auto mech: mechs[section_kind::axon]) {
        cell.paint("axon", mech);
    }

    cell.default_parameters.discretization = arb::cv_policy_fixed_per_branch(200);

    // Add spike threshold detector at the soma.
    for (auto d: detectors) {
        cell.place(d.first, arb::threshold_detector{d.second});
    }

    for (auto s: synapses) {
        cell.place(s.first, s.second);
    }

    return cell;
}