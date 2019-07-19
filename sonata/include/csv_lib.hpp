#pragma once

#include <arbor/common_types.hpp>
#include <arbor/swcio.hpp>

#include <string>
#include <fstream>

#include "density_mech_helper.hpp"
#include "dynamics_params_helper.hpp"

struct type_pop_id {
    unsigned type_tag;
    std::string pop_name;

    type_pop_id(unsigned tag, std::string name) : type_tag(tag), pop_name(name){}
};

inline bool operator==(const type_pop_id& lhs, const type_pop_id& rhs) {
    return lhs.type_tag == rhs.type_tag && lhs.pop_name == rhs.pop_name;
}

namespace std {
    template<> struct hash<type_pop_id>
    {
        std::size_t operator()(const type_pop_id& t) const noexcept
        {
            std::size_t const h1(std::hash<unsigned>{}(t.type_tag));
            std::size_t const h2(std::hash<std::string>{}(t.pop_name));
            return h1 ^ (h2 << 1);
        }
    };
}

class csv_file {
    std::string filename;
    char delimeter;
    std::vector<std::vector<std::string>> data;

public:
    csv_file(std::string name, char delm = ',');
    std::vector<std::vector<std::string>> get_data();
    std::string name();
};

////////////////////////////////////////////////////////

class csv_record {
public:
    csv_record(std::vector<csv_file> files);

    std::vector<type_pop_id> unique_ids() const;
    std::unordered_map<std::string, std::string> fields(type_pop_id id) const;

protected:
    std::vector<type_pop_id> ids_;

    // Map from type_pop_id to map of fields and values
    std::unordered_map<type_pop_id, std::unordered_map<std::string, std::string>> fields_;
};

///////////////////////////////////////////////////////

class csv_node_record : public csv_record {
public:
    csv_node_record(std::vector<csv_file> files);

    arb::cell_kind cell_kind(type_pop_id id);

    arb::morphology morph(type_pop_id id);

    // Returns a map from mechanism names to list of variables and their overrides for a unique node
    std::unordered_map<std::string, variable_map> dynamic_params(type_pop_id id);

    // Returns a map from section names to mechanism decriptions for a unique node with
    // parameter overrides applied
    std::unordered_map<std::string, std::vector<arb::mechanism_desc>> density_mech_desc(type_pop_id id, std::unordered_map<std::string, variable_map> override = {});

    void override_density_params(type_pop_id id, std::unordered_map<std::string, variable_map> override);

private:
    // Map from type_pop_id to mechanism descriptions
    std::unordered_map<type_pop_id, std::unordered_map<std::string, mech_groups>> density_params_;

    // Map from type_pop_id to map of fields and values
    std::unordered_map<type_pop_id, arb::morphology> morphologies_;
};

class csv_edge_record : public csv_record {
public:
    csv_edge_record(std::vector<csv_file> files);

    arb::mechanism_desc point_mech_desc(type_pop_id id) const;

    // Given a target population, return a pair of edge populations and source populations that are connected to the target population
    std::vector<std::pair<std::string, std::string>> edge_to_source_of_target(std::string target_pop) const;

    // Given a target population, return all edge populations that are connected to the target population
    std::unordered_set<std::string> edges_of_target(std::string target_pop) const;

    // Given a source population, return all edge populations that are connected to the source population
    std::unordered_set<std::string> edges_of_source(std::string source_pop) const;
private:

    // Map from type_pop_id to point_mechanisms_desc
    std::unordered_map<type_pop_id, arb::mechanism_desc> point_params_;
};
