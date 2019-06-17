#include <arbor/common_types.hpp>
#include <arbor/swcio.hpp>
#include <arbor/segment.hpp>

#include <string>
#include <fstream>

#include "include/sonata_excpetions.hpp"
#include "include/density_mech_helper.hpp"
#include "include/csv_lib.hpp"

arb::mechanism_desc read_dynamics_params_point(std::string fname);
std::unordered_map<std::string, mech_groups> read_dynamics_params_density_base(std::string fname);
std::unordered_map<std::string, variable_map> read_dynamics_params_density_override(std::string fname);

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
                    is_edge_ = col_names[loc].find("edge") != std::string::npos ? true : false;
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
        }
    }

    for (auto type: fields_) {
        if (!is_edge_) {
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
                    morphologies_[type.first] = arb::swc_as_morphology(arb::parse_swc_file(f));
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
                        override_density_params(type.first, dyn_params);
                    }
                }
            }
        } else {
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
}


std::unordered_map<type_pop_id, std::unordered_map<std::string, std::string>> csv_record::data() {
    return fields_;
}

std::unordered_map<std::string, std::string> csv_record::fields(type_pop_id id) {
    if (fields_.find(id) != fields_.end()) {
        return fields_[id];
    }
    throw sonata_exception("Requested CSV column not found");
}

arb::morphology csv_record::morph(type_pop_id id) {
    if (morphologies_.find(id) != morphologies_.end()) {
        return morphologies_[id];
    }
    throw sonata_exception("Requested morphology not found");
}

arb::mechanism_desc csv_record::point_mech_desc(type_pop_id id) {
    if (point_params_.find(id) != point_params_.end()) {
        return point_params_.at(id);
    }
    throw sonata_exception("Requested CSV dynamics_params not available");
}

arb::cell_kind csv_record::get_cell_kind(type_pop_id id) {
    if (fields_[id]["model_type"] == "virtual") {
        return arb::cell_kind::spike_source;
    }
    return arb::cell_kind::cable;
}

std::unordered_map<std::string, variable_map> csv_record::dynamic_params(type_pop_id id) {
    std::unordered_map<std::string, variable_map> ret;
    if (density_params_.find(id) != density_params_.end()) {
        for (auto& mech: density_params_.at(id)) {
            ret[mech.first] = mech.second.variables;
        }
    }
    return ret;
}

std::unordered_map<std::string, std::vector<arb::mechanism_desc>> csv_record::density_mech_desc(
        type_pop_id id, std::unordered_map<std::string, variable_map> overrides) {
    std::unordered_map<std::string, std::vector<arb::mechanism_desc>> ret;

    auto mech_type_id =  density_params_[id];

    // For every mech_id
    for (auto mech_id: mech_type_id) {
        // For every instance of the mech_id
        for (auto density_mech: mech_id.second.mech_details) {
            auto mech_desc = density_mech.mech;
            // Get the aliases and set values from overrides
            for (auto alias: density_mech.param_alias) {
                mech_desc.set(alias.first, overrides[mech_id.first][alias.second]);
            }
            ret[density_mech.section].push_back(std::move(mech_desc));
        }
    }
    return ret;
}

void csv_record::print_point() {
    for (auto p: point_params_) {
        std::cout << p.first.type_tag << " " << p.first.pop_name  << std::endl;
        std::cout << "\t" << p.second.name() << std::endl;
        for (auto i: p.second.values()) {
            std::cout << "\t\t" << i.first <<  " = " << i.second << std::endl;
        }
    }
};

void csv_record::print_density() {
    for (auto p: density_params_) {
        std::cout << p.first.type_tag << " " << p.first.pop_name  << std::endl;
        for (auto i: p.second) {
            std::cout << "\t" << i.first << std::endl;
            (i.second).print();
        }
    }
};

void csv_record::print() {
    for (auto f: fields_) {
        std::cout << f.first.type_tag << " " << f.first.pop_name << std::endl;
        for (auto m: f.second) {
            std::cout << "\t" << m.first << "->" << m.second << std::endl;
        }
    }
};

void csv_record::override_density_params(type_pop_id id, std::unordered_map<std::string, variable_map>& override) {
    auto& base = density_params_[id];

    // For every segment in overrides, look for matching segment in base
    for (auto seg_overrides: override) {
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

