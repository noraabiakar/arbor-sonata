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

    arb::cable_cell cell = arb::make_cable_cell(morph);

    for (auto& segment: cell.segments()) {
        segment->rL = 100;
        for (auto mech: mechs[segment->kind()]) {
            segment->add_mechanism(mech);
        }
        if (segment->kind() == section_kind::dendrite || segment->kind() == section_kind::axon) {
            segment->set_compartments(200);
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