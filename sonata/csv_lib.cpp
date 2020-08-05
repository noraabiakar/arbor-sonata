#include <arbor/common_types.hpp>
#include <arbor/swcio.hpp>

#include <string>
#include <fstream>

#include "include/sonata_exceptions.hpp"
#include "include/density_mech_helper.hpp"
#include "include/csv_lib.hpp"

csv_file::csv_file(std::string name, char delm) :
        filename(name), delimeter(delm) {
    std::ifstream file(name);

    if (!file.good()) {
        throw std::runtime_error("Unable to open csv file: " + name);
    }

    std::string line;

    while (getline(file, line)) {
        std::vector<std::string> vec;
        std::stringstream ss(line);
        std::string value;

        while(getline(ss, value, delimeter)) {
            vec.push_back(value);
        }
        data.push_back(vec);
    }
    file.close();
}

std::vector<std::vector<std::string>> csv_file::get_data() {
    return data;
}

std::string csv_file::name() {
    return filename;
}

////////////////////////////////////////////////////////

csv_record::csv_record(std::vector<csv_file> files) {
    for (auto f: files) {
        auto csv_data = f.get_data();
        auto col_names = csv_data.front();

        for(auto it = csv_data.begin()+1; it < csv_data.end(); it++) {
            std::unordered_map<std::string, std::string> type_fields(csv_data.front().size() - 1);
            unsigned type_tag, loc = 0;
            std::string pop_name;

            for (auto field: *it) {
                if (col_names[loc].find("type_id") != std::string::npos) {
                    type_tag = std::atoi(field.c_str());
                }
                else {
                    if (col_names[loc] == "pop_name") {
                        pop_name = field;
                    }
                    type_fields[col_names[loc]] = field;
                }
                loc++;
            }
            fields_[type_pop_id(type_tag, pop_name)] = type_fields;
            ids_.emplace_back(type_tag, pop_name);
        }
    }
}

std::unordered_map<std::string, std::string> csv_record::fields(type_pop_id id) const {
    if (fields_.find(id) != fields_.end()) {
        return fields_.at(id);
    }
    throw sonata_exception("Requested CSV column not found");
}

std::vector<type_pop_id> csv_record::unique_ids() const {
    return ids_;
}

////////////////////////////////////////////////////////

csv_node_record::csv_node_record(std::vector<csv_file> files) : csv_record(files) {
    for (auto type: fields_) {
        if (type.second.find("model_type") == type.second.end()) {
            throw sonata_exception("Model_type not found in node csv description");
        }
        std::string model_type = type.second["model_type"];

        if (model_type != "virtual") {
            if (type.second.find("morphology") != type.second.end()) {
                if (type.second["morphology"] == "NULL") {
                    throw sonata_exception("Morphology of non-virtual cell can not be NULL");
                }
                std::ifstream f(type.second["morphology"]);
                if (!f) throw sonata_exception("Unable to open SWC file");
                morphologies_[type.first] = arb::morphology(arb::swc_as_segment_tree(arb::parse_swc_file(f)));
            } else {
                throw sonata_exception("Morphology not found in node csv description");
            }

            if (type.second.find("model_template") != type.second.end()) {
                if (type.second["model_template"] == "NULL") {
                    throw sonata_exception("Model_template of non-virtual cell can not be NULL");
                }
                density_params_.insert(
                        {type.first, std::move(read_dynamics_params_density_base(type.second["model_template"]))});
            } else {
                throw sonata_exception("Model_template not found in node csv description");
            }

            if (type.second.find("dynamics_params") != type.second.end()) {
                if (type.second["dynamics_params"] != "NULL") {
                    auto dyn_params = std::move(
                            read_dynamics_params_density_override(type.second["dynamics_params"]));
                    override_density_params(type.first, std::move(dyn_params));
                }
            }
        }
    }
}

arb::morphology csv_node_record::morph(type_pop_id id) {
    if (morphologies_.find(id) != morphologies_.end()) {
        return morphologies_[id];
    }
    throw sonata_exception("Requested morphology not found");
}

arb::cell_kind csv_node_record::cell_kind(type_pop_id id) {
    if (fields_[id]["model_type"] == "virtual") {
        return arb::cell_kind::spike_source;
    }
    return arb::cell_kind::cable;
}

std::unordered_map<std::string, variable_map> csv_node_record::dynamic_params(type_pop_id id) {
    std::unordered_map<std::string, variable_map> ret;
    if (density_params_.find(id) != density_params_.end()) {
        for (auto& mech: density_params_.at(id)) {
            ret[mech.first] = mech.second.variables;
        }
    }
    return ret;
}

std::unordered_map<section_kind, std::vector<arb::mechanism_desc>> csv_node_record::density_mech_desc(
        type_pop_id id, std::unordered_map<std::string, variable_map> overrides) {
    std::unordered_map<section_kind, std::vector<arb::mechanism_desc>> ret;

    std::unordered_map<std::string, mech_groups> density_mechs = density_params_[id];

    for (auto seg_overrides: overrides) {
        if (density_mechs.find(seg_overrides.first) != density_mechs.end()) {
            auto& base_id = density_mechs.at(seg_overrides.first);
            auto& base_vars = base_id.variables;
            for (auto var: seg_overrides.second) {
                if (base_vars.find(var.first) != base_vars.end()) {
                    base_vars[var.first] = var.second;
                }
            }
        }
    }

    // For every mech_id
    for (auto mech: density_mechs) {
        auto mech_id = mech.first;
        auto mech_gp = mech.second;

        mech_gp.apply_variables();

        for (auto mech_instance: mech_gp.mech_details) {
            ret[mech_instance.section].push_back(mech_instance.mech);
        }
    }
    return ret;
}

void csv_node_record::override_density_params(type_pop_id id, std::unordered_map<std::string, variable_map> overrides) {
    auto& base = density_params_[id];

    // For every segment in overrides, look for matching segment in base
    for (auto seg_overrides: overrides) {
        if (base.find(seg_overrides.first) != base.end()) {
            auto& base_id = base.at(seg_overrides.first);
            auto& base_vars = base_id.variables;
            for (auto var: seg_overrides.second) {
                if (base_vars.find(var.first) != base_vars.end()) {
                    base_vars[var.first] = var.second;
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

csv_edge_record::csv_edge_record(std::vector<csv_file> files) : csv_record(files) {
    for (auto type: fields_) {
        if (type.second.find("model_template") == type.second.end()) {
            throw sonata_exception("Model_template not found in node csv description");
        }

        if (type.second.find("dynamics_params") != type.second.end()) {
            if (type.second["dynamics_params"] != "NULL") {
                auto mech = read_dynamics_params_point(type.second["dynamics_params"]);

                if (mech.name() != type.second["model_template"]) {
                    throw sonata_exception("point mechanism in \'dynamics_params\' does not match \'model_template\'");
                }

                point_params_.insert({type.first, std::move(mech)});
            } else {
                point_params_.insert({type.first, arb::mechanism_desc(type.second["model_template"])});
            }
        } else {
            point_params_.insert({type.first, arb::mechanism_desc(type.second["model_template"])});
        }
    }
}

std::vector<std::pair<std::string, std::string>> csv_edge_record::edge_to_source_of_target(std::string target_pop) const {
    std::vector<std::pair<std::string, std::string>> edge_to_source;

    for (auto id: ids_) {
        const auto& type = fields(id);
        if (type.at("target_pop_name") == target_pop) {
            edge_to_source.emplace_back(std::make_pair(type.at("pop_name"), type.at("source_pop_name")));
        }
    }
    return edge_to_source;
}

std::unordered_set<std::string> csv_edge_record::edges_of_target(std::string target_pop) const {
    std::unordered_set<std::string> target_edge_pops;

    for (auto id: ids_) {
        const auto& type = fields(id);
        if (type.at("target_pop_name") == target_pop) {
            target_edge_pops.insert(type.at("pop_name"));
        }
    }
    return target_edge_pops;
}

std::unordered_set<std::string> csv_edge_record::edges_of_source(std::string source_pop) const {
    std::unordered_set<std::string> source_edge_pops;

    for (auto id: ids_) {
        const auto& type = fields(id);
        if (type.at("source_pop_name") == source_pop) {
            source_edge_pops.insert(type.at("pop_name"));
        }
    }
    return source_edge_pops;
}

arb::mechanism_desc csv_edge_record::point_mech_desc(type_pop_id id) const {
    if (point_params_.find(id) != point_params_.end()) {
        return point_params_.at(id);
    }
    throw sonata_exception("Requested CSV dynamics_params not available");
}


