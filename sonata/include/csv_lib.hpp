#include <arbor/common_types.hpp>
#include <arbor/swcio.hpp>

#include <string>
#include <fstream>

#include "sonata_excpetions.hpp"

extern std::unordered_map<std::string, double> read_dynamics_params_single(std::string fname);
extern std::unordered_map<std::string, std::unordered_map<std::string, double>>
    read_dynamics_params_multiple(std::string fname);

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
                std::unordered_map<std::string, std::unordered_map<std::string, double>> type_params;
                arb::morphology type_morph;
                unsigned type_id, loc = 0;

                for (auto field: *it) {
                    if (col_names[loc] == "morphology") {
                        std::ifstream f(field);
                        if (!f) throw sonata_exception("Unable to open SWC file");
                        type_morph = arb::swc_as_morphology(arb::parse_swc_file(f));
                    }
                    if (col_names[loc] == "dynamics_params") {
                        type_params = read_dynamics_params_multiple(field);
                    }
                    if (col_names[loc].find("type_id") != std::string::npos) {
                        type_id = std::atoi(field.c_str());
                    }
                    else {
                        type_fields[col_names[loc]] = field;
                    }
                    loc++;
                }
                data_[type_id] = type_fields;
                dynamics_params_[type_id] = type_params;
                morphologies_[type_id] = type_morph;
            }
        }
    }


    std::unordered_map<unsigned, std::unordered_map<std::string, std::string>> data() {
        return data_;
    }

    std::unordered_map<std::string, std::unordered_map<std::string, double>>
    dyn_params(unsigned type) {
        if (dynamics_params_.find(type) != dynamics_params_.end()) {
            return dynamics_params_[type];
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
        if (data_.find(type) != data_.end()) {
            return data_[type];
        }
        throw sonata_exception("Requested CSV column not found");
    }

private:
    // Map from type_id to map of fields and values
    std::unordered_map<unsigned, std::unordered_map<std::string, std::string>> data_;

    // Map from type_id to map from mechanisms to their parameters
    std::unordered_map<unsigned, // type_id
     std::unordered_map<std::string, // mechanism
     std::unordered_map<std::string, // parameter
     double>>> dynamics_params_;

    // Map from type_id to map of fields and values
    std::unordered_map<unsigned, arb::morphology> morphologies_;
};
