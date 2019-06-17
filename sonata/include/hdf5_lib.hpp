#pragma once

#include <iostream>
#include <string>

#include <hdf5.h>
class h5_dataset {
private:
    hid_t parent_id_;
    std::string name_;
    size_t size_;

public:
    h5_dataset(hid_t parent, std::string name);

    std::string name();

    int size();

    auto int_at(const int i);

    auto double_at(const int i);

    auto string_at(const int i);

    auto int_range(const int i, const int j);

    auto double_range(const int i, const int j);

    auto int_pair_at(const int i);

    auto int_1d();

    auto int_2d();
};

class h5_group {
private:
    struct group_handle {
        group_handle(hid_t parent_id, std::string name): id(H5Gopen(parent_id, name.c_str(), H5P_DEFAULT)), name(name){}
        ~group_handle() {
            H5Gclose(id);
        }
        hid_t id;
        std::string name;
    };

    hid_t parent_id_;
    std::string name_;
    group_handle group_h_;

public:
    std::vector<std::shared_ptr<h5_group>> groups_;
    std::vector<std::shared_ptr<h5_dataset>> datasets_;

    h5_group(hid_t parent, std::string name);

    std::string name();
};

class h5_file {
private:
    struct file_handle {
        file_handle(std::string file): id(H5Fopen(file.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT)), name(file) {}
        ~file_handle() {
            H5Fclose(id);
        }
        hid_t id;
        std::string name;
    };

    std::string file_;
    file_handle file_h_;

public:
    std::shared_ptr<h5_group> top_group_;
    h5_file(std::string name);

    std::string name();
    void print();
};


class h5_wrapper {
public:
    h5_wrapper();

    h5_wrapper(const std::shared_ptr<h5_group>& g);

    int size();

    int find_group(std::string name);

    int find_dataset(std::string name);

    int dataset_size(std::string name);

    int int_at(std::string name, unsigned i);

    double double_at(std::string name, unsigned i);

    std::string string_at(std::string name, unsigned i);

    std::vector<int> int_range(std::string name, unsigned i, unsigned j);

    std::vector<double> double_range(std::string name, unsigned i, unsigned j);

    std::pair<int, int> int_pair_at(std::string name, unsigned i);

    std::vector<int> int_1d(std::string name);

    std::vector<std::pair<int, int>> int_2d(std::string name);

    h5_wrapper operator [](unsigned i) const;

    std::string name();

private:
    std::shared_ptr<h5_group> ptr;
    std::vector<h5_wrapper> members;
    std::unordered_map<std::string, unsigned> dset_map;
    std::unordered_map<std::string, unsigned> member_map;
};


class hdf5_record {
public:
    hdf5_record(const std::vector<std::shared_ptr<h5_file>>& files);

    void verify_edges();

    void verify_nodes();

    int num_elements() const;

    h5_wrapper operator [](std::string i) const;

    h5_wrapper operator [](int i) const;

    std::vector<h5_wrapper> populations() const;

    std::vector<unsigned> partitions() const;

    std::unordered_map<std::string, unsigned> map() const;

    std::vector<std::string> pop_names() const;

private:
    int num_elements_ = 0;
    std::vector<std::string> pop_names_;
    std::vector<unsigned> partition_;
    std::vector<h5_wrapper> populations_;
    std::unordered_map<std::string, unsigned> map_;
};
