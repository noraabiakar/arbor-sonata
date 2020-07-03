#pragma once

#include <arbor/common_types.hpp>
#include <arbor/cable_cell.hpp>
#include <arbor/simple_sampler.hpp>


#include "csv_lib.hpp"
#include "hdf5_lib.hpp"

#include <string>

using arb::cell_gid_type;
using arb::cell_lid_type;
using arb::cell_size_type;
using arb::cell_member_type;
using arb::mlocation;

struct sim_conditions {
    double temp_c;
    double v_init;
};

struct run_params {
    double duration;
    double dt;
    double threshold;
};

struct probe_info {
    std::string kind;
    std::string population;
    std::vector<unsigned> node_ids;
    unsigned sec_id;
    double sec_pos;
    std::string file_name;

    probe_info() {};

    probe_info(std::string k, std::string pop, std::vector<unsigned> ids, unsigned sid, double spos, std::string file):
    kind(k), population(pop), node_ids(ids), sec_id(sid), sec_pos(spos), file_name(file) {};
};

struct current_clamp_info {
    csv_file stim_params;
    csv_file stim_loc;
};

struct spike_out_info {
    std::string file_name;
    std::string sort_by;
};

struct spike_in_info {
    h5_wrapper data;
    std::string population;
};

struct current_clamp_desc {
    double duration;
    double amplitude;
    double delay;
    arb::mlocation stim_loc;

    current_clamp_desc(double dur, double amp, double del, arb::mlocation loc):
            duration(dur), amplitude(amp), delay(del), stim_loc(loc){}
};

struct source_type {
    cell_lid_type segment;
    double position;

    source_type(): segment(0), position(0) {}
    source_type(cell_lid_type s, double p) : segment(s), position(p) {}
};

inline bool operator==(const source_type& lhs, const source_type& rhs) {
    return lhs.segment == rhs.segment && lhs.position == rhs.position;
}

struct target_type {
    cell_lid_type segment;
    double position;
    arb::mechanism_desc synapse;

    target_type(cell_lid_type s, double p, arb::mechanism_desc m) : segment(s), position(p), synapse(m) {}
};

inline bool operator==(const target_type& lhs, const target_type& rhs) {
    return lhs.position == rhs.position &&
           lhs.segment == rhs.segment &&
           lhs.synapse.name() == rhs.synapse.name() &&
           lhs.synapse.values() == rhs.synapse.values();
}

struct trace_info {
    bool is_voltage;
    arb::mlocation loc;

    arb::trace_vector<double> data;

    trace_info() {};

    trace_info(bool v, arb::mlocation l): is_voltage(v), loc(l) {};
};

struct trace_index_and_info {
    unsigned idx;
    trace_info info;

    trace_index_and_info(unsigned idx, trace_info t): idx(idx), info(t) {};
};


namespace std {
    template<> struct hash<source_type>
    {
        std::size_t operator()(const source_type& s) const noexcept
        {
            std::size_t const h1(std::hash<unsigned>{}(s.segment));
            std::size_t const h2(std::hash<double>{}(s.position));
            return h1 ^ (h2 << 1);
        }
    };
}
