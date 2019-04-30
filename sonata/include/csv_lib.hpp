#include <arbor/common_types.hpp>
#include <arbor/swcio.hpp>

#include <string>
#include <fstream>

#include "sonata_excpetions.hpp"
#include "density_mech_helper.hpp"

arb::mechanism_desc read_dynamics_params_point(std::string fname);
std::unordered_map<std::string, std::vector<arb::mechanism_desc>> read_dynamics_params_density(std::string fname);
std::unordered_map<std::string, mech_groups> read_dynamics_params_density_base(std::string fname);
std::unordered_map<std::string, variable_map> read_dynamics_params_density_override(std::string fname);

class csv_file {
    std::string fileName;
    char delimeter;

public:
    csv_file(std::string filename, char delm = ',') :
            fileName(filename), delimeter(delm) { }

    std::vector<std::vector<std::string>> get_data() {
        std::ifstream file(fileName);

        std::vector<std::vector<std::string>> data;
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
        return data;
    }

    std::string name() {
        return fileName;
    }
};

////////////////////////////////////////////////////////

class csv_record {
public:
    csv_record(std::vector<csv_file> files) {
        for (auto f: files) {
            auto csv_data = f.get_data();
            auto col_names = csv_data.front();

            for(auto it = csv_data.begin()+1; it < csv_data.end(); it++) {
                std::unordered_map<std::string, std::string> type_fields(csv_data.front().size() - 1);
                unsigned type_id, loc = 0;

                for (auto field: *it) {
                    if (col_names[loc].find("type_id") != std::string::npos) {
                        type_id = std::atoi(field.c_str());
                        is_edge_ = col_names[loc].find("edge") != std::string::npos ?true: false;
                    }
                    else {
                        type_fields[col_names[loc]] = field;
                    }
                    loc++;
                }
                fields_[type_id] = type_fields;
            }
        }

        for (auto type: fields_) {
            if (!is_edge_) {
                if (type.second.find("morphology") != type.second.end()) {
                    std::ifstream f(type.second["morphology"]);
                    if (!f) throw sonata_exception("Unable to open SWC file");
                    morphologies_[type.first] = arb::swc_as_morphology(arb::parse_swc_file(f));
                } else {
                    throw sonata_exception("Morphology not found in node csv description");
                }

                if (type.second.find("model_template") != type.second.end()) {
                    std::cout << type.first << std::endl << std::endl;
                    density_params_.insert(
                            {type.first, std::move(read_dynamics_params_density_base(type.second["model_template"]))});
                } else {
                    throw sonata_exception("Model_template not found in node csv description");
                }

                if (type.second.find("dynamics_params") != type.second.end()) {
                    auto dyn_params = std::move(read_dynamics_params_density_override(type.second["dynamics_params"]));
                    override_density_params(type.first, dyn_params);
                }
            } else {
                if (type.second.find("dynamics_params") != type.second.end()) {
                    point_params_.insert(
                            {type.first, std::move(read_dynamics_params_point(type.second["dynamics_params"]))});
                }
            }
        }
        std::cout << (files.front()).name() << std::endl;
        //print_point();
        print_density();
    }


    std::unordered_map<unsigned, std::unordered_map<std::string, std::string>> data() {
        return fields_;
    }

    std::unordered_map<std::string, std::string> fields(unsigned type) {
        if (fields_.find(type) != fields_.end()) {
            return fields_[type];
        }
        throw sonata_exception("Requested CSV column not found");
    }

    arb::morphology morph(unsigned type) {
        if (morphologies_.find(type) != morphologies_.end()) {
            return morphologies_[type];
        }
        throw sonata_exception("Requested morphology not found");
    }

    arb::mechanism_desc point_mech_desc(unsigned type) {
        if (point_params_.find(type) != point_params_.end()) {
            return point_params_.at(type);
        }
        throw sonata_exception("Requested CSV dynamics_params not available");
    }

    std::unordered_map<std::string, variable_map> density_vars(unsigned type) {
        std::unordered_map<std::string, variable_map> ret;
        if (density_params_.find(type) != density_params_.end()) {
            for (auto& mech: density_params_.at(type)) {
                ret[mech.first] = mech.second.variables;
            }
        }
        return ret;
    }

    std::unordered_map<std::string, std::vector<arb::mechanism_desc>>
    density_mech_desc(unsigned type, std::unordered_map<std::string, variable_map> overrides) {
        std::unordered_map<std::string, std::vector<arb::mechanism_desc>> ret;

        auto mech_type_id =  density_params_[type];

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

    void print_point() {
        for (auto p: point_params_) {
            std::cout << p.first << std::endl;
            std::cout << "\t" << p.second.name() << std::endl;
            for (auto i: p.second.values()) {
                std::cout << "\t\t" << i.first <<  " = " << i.second << std::endl;
            }
        }
    };

    void print_density() {
        for (auto p: density_params_) {
            std::cout << p.first << std::endl;
            for (auto i: p.second) {
                std::cout << "\t" << i.first << std::endl;
                (i.second).print();
            }
        }
    };

    void override_density_params(unsigned type_id, std::unordered_map<std::string, variable_map>& override) {
        auto& base = density_params_[type_id];

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

private:

    // Map from type_id to map of fields and values
    std::unordered_map<unsigned, std::unordered_map<std::string, std::string>> fields_;

    // Map from type_id to point_mechanisms_desc
    std::unordered_map<unsigned, arb::mechanism_desc> point_params_;

    // Map from type_id to mechanisms_desc
    std::unordered_map<unsigned, std::unordered_map<std::string, mech_groups>> density_params_;

    // Map from type_id to map of fields and values
    std::unordered_map<unsigned, arb::morphology> morphologies_;

    bool is_edge_;
};
