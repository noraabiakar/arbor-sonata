#include <arbor/common_types.hpp>

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
                std::unordered_map<std::string, std::string> part_data(csv_data.front().size() - 1);
                std::unordered_map<std::string, std::unordered_map<std::string, double>> type_params;
                unsigned part_data_type, loc = 0;

                for (auto field: *it) {
                    if (col_names[loc].find("dynamics_params") != std::string::npos) {
                        type_params = read_dynamics_params_multiple(field);
                    }
                    if (col_names[loc].find("type_id") != std::string::npos) {
                        part_data_type = std::atoi(field.c_str());
                    }
                    else {
                        part_data[col_names[loc]] = field;
                    }
                    loc++;
                }
                data_[part_data_type] = part_data;
                dynamics_params_[part_data_type] = type_params;
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
        throw sonata_exception("Requested CSV dynamics_params not available\n");
    }

    std::unordered_map<std::string, std::string> fields(unsigned type) {
        if (data_.find(type) != data_.end()) {
            return data_[type];
        }
        throw sonata_exception("Requested CSV column not found\n");
    }

private:
    // Map from type_id to map of fields and values
    std::unordered_map<unsigned, std::unordered_map<std::string, std::string>> data_;

    // Map from type_id to map from mechanisms to their parameters
    std::unordered_map<unsigned, // type_id
     std::unordered_map<std::string, // mechanism
     std::unordered_map<std::string, // parameter
     double>>> dynamics_params_;
};
