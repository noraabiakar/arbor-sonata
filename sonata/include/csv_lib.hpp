#include <arbor/common_types.hpp>

#include <string>
#include <fstream>

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
            auto vec = csv_data.front();

            std::unordered_map<std::string, std::string> part_data(csv_data.front().size() - 1);
            unsigned part_data_type;

            for(auto it = csv_data.begin()+1; it < csv_data.end(); it++) {
                unsigned loc = 0;
                for (auto field: *it) {
                    if (vec[loc].find("type_id") != std::string::npos) {
                        part_data_type = std::atoi(field.c_str());
                    }
                    else {
                        part_data[vec[loc]] = field;
                    }
                    loc++;
                }
                data_[part_data_type] = part_data;
            }
        }
    }


    std::unordered_map<unsigned, std::unordered_map<std::string, std::string>> data() {
        return data_;
    }

    std::unordered_map<std::string, std::string> fields(unsigned type) {
        return data_[type];
    }

private:
    std::unordered_map<unsigned, std::unordered_map<std::string, std::string>> data_;
};
