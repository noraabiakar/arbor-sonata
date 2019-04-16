#include <arbor/common_types.hpp>
#include <arbor/swcio.hpp>

#include <string>
#include <fstream>

#include "sonata_excpetions.hpp"

arb::mechanism_desc read_dynamics_params_point(std::string fname);
std::unordered_map<std::string, std::vector<arb::mechanism_desc>>
    read_dynamics_params_density(std::string fname);

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
            if (type.second.find("morphology") != type.second.end()) {
                std::ifstream f(type.second["morphology"]);
                if (!f) throw sonata_exception("Unable to open SWC file");
                morphologies_[type.first] = arb::swc_as_morphology(arb::parse_swc_file(f));
            }
            if (type.second.find("dynamics_params") != type.second.end()) {
                if (is_edge_) {
                    point_params_.insert({type.first, std::move(read_dynamics_params_point(type.second["dynamics_params"]))});
                }
                else {
                    density_params_.insert({type.first, std::move(read_dynamics_params_density(type.second["dynamics_params"]))});
                }
            }
        }
    }


    std::unordered_map<unsigned, std::unordered_map<std::string, std::string>> data() {
        return fields_;
    }

    arb::mechanism_desc mech_desc(unsigned type) {
        if (point_params_.find(type) != point_params_.end()) {
            return point_params_.at(type);
        }
        throw sonata_exception("Requested CSV dynamics_params not available");
    }

    arb::morphology morph(unsigned type) {
        if (morphologies_.find(type) != morphologies_.end()) {
            return morphologies_[type];
        }
        throw sonata_exception("Requested morphology not found");
    }

    std::unordered_map<std::string, std::string> fields(unsigned type) {
        if (fields_.find(type) != fields_.end()) {
            return fields_[type];
        }
        throw sonata_exception("Requested CSV column not found");
    }

private:
    // Map from type_id to map of fields and values
    std::unordered_map<unsigned, std::unordered_map<std::string, std::string>> fields_;

    // Map from type_id to point_mechanisms_desc
    std::unordered_map<unsigned, arb::mechanism_desc> point_params_;

    // Map from type_id to mechanisms_desc
    std::unordered_map<unsigned, std::unordered_map<std::string, std::vector<arb::mechanism_desc>>> density_params_;

    // Map from type_id to map of fields and values
    std::unordered_map<unsigned, arb::morphology> morphologies_;

    bool is_edge_;
};
