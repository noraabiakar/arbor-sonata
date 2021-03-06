#pragma once

#include <iostream>
#include <string>

#include <hdf5.h>

/// Class for reading from hdf5 datasets
/// Datasets are opened and closed every time they are read
class h5_dataset {
public:
    // Constructor from parent (hdf5 group) id and dataset name - finds size of the dataset
    h5_dataset(hid_t parent, std::string name);

    // returns name of dataset
    std::string name();

    // returns number of elements in a dataset
    int size();

    // Read integer at index `i`; throws exception if out of bounds
    auto int_at(const int i);

    // Read double at index `i`; throws exception if out of bounds
    auto double_at(const int i);

    // Read string at index `i`; throws exception if out of bounds
    auto string_at(const int i);

    // Read range of integers between indices `i` and `j`; throws exception if out of bounds
    auto int_range(const int i, const int j);

    // Read range of doubles between indices `i` and `j`; throws exception if out of bounds
    auto double_range(const int i, const int j);

    // Read integer pair at index `i` (dataset has dimensions size() x 2)
    // Throws exception if out of bounds
    auto int_pair_at(const int i);

    // Read all 1D integer dataset
    auto int_1d();

    // Read all 2D integer dataset
    auto int_2d();

private:
    // id of parent group
    hid_t parent_id_;

    // name of dataset
    std::string name_;

    // First dimension of dataset
    size_t size_;
};


/// Class for keeping track of what's in an hdf5 group (groups and datasets with pointers to each)
class h5_group {
public:
    // Constructor from parent (hdf5 group) id and group name
    // Builds tree of groups, each with it's own sub-groups and datasets
    h5_group(hid_t parent, std::string name);

    // Returns name of group
    std::string name();

    // hdf5 groups belonging to group
    std::vector<std::shared_ptr<h5_group>> groups_;

    // hdf5 datasets belonging to group
    std::vector<std::shared_ptr<h5_dataset>> datasets_;

private:
    // RAII to handle recursive opening/closing groups
    struct group_handle {
        group_handle(hid_t parent_id, std::string name): id(H5Gopen(parent_id, name.c_str(), H5P_DEFAULT)), name(name){}
        ~group_handle() {
            H5Gclose(id);
        }
        hid_t id;
        std::string name;
    };

    // id of parent group
    hid_t parent_id_;

    // name of group
    std::string name_;

    // Handles group opening/closing
    group_handle group_h_;
};


/// Class for an hdf5 file, holding a pointer to the top level group in the file
class h5_file {
private:
    // RAII to handle opening/closing files
    struct file_handle {
        file_handle(std::string file): id(H5Fopen(file.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT)), name(file) {}
        ~file_handle() {
            H5Fclose(id);
        }
        hid_t id;
        std::string name;
    };

    // name of file
    std::string name_;

    // Handles file opening/closing
    file_handle file_h_;

public:
    // Constructor from file name
    h5_file(std::string name);

    // Returns file name
    std::string name();

    // Debugging function
    void print();

    // Pointer to top level group in file
    std::shared_ptr<h5_group> top_group_;
};

/// Class that wraps an h5_group
/// Provides direct read access to datasets in the group
/// Provides access to sub-groups of the group
class h5_wrapper {
public:
    h5_wrapper();

    h5_wrapper(const std::shared_ptr<h5_group>& g);

    // Returns number of sub-groups in the wrapped h5_group
    int size();

    // Returns index of sub-group with name `name`; returns -1 if sub-group not found
    int find_group(std::string name) const;

    // Returns index of dataset with name `name`; returns -1 if dataset not found
    int find_dataset(std::string name) const;

    // Returns size of dataset with name `name`; returns -1 if dataset not found
    int dataset_size(std::string name) const;

    // Returns int at index i of dataset with name `name`; throws exception if dataset not found
    int int_at(std::string name, unsigned i) const;

    // Returns double at index i of dataset with name `name`; throws exception if dataset not found
    double double_at(std::string name, unsigned i) const;

    // Returns string at index i of dataset with name `name`; throws exception if dataset not found
    std::string string_at(std::string name, unsigned i) const;

    // Returns integers between indices i and j of dataset with name `name`; throws exception if dataset not found
    std::vector<int> int_range(std::string name, unsigned i, unsigned j) const;

    // Returns double between indices i and j of dataset with name `name`; throws exception if dataset not found
    std::vector<double> double_range(std::string name, unsigned i, unsigned j) const;

    // Returns int pair at index i of dataset with name `name`; throws exception if dataset not found
    std::pair<int, int> int_pair_at(std::string name, unsigned i) const;

    // Returns full content of 1D dataset with name `name`; throws exception if dataset not found
    std::vector<int> int_1d(std::string name) const;

    // Returns full content of 2D dataset with name `name`; throws exception if dataset not found
    std::vector<std::pair<int, int>> int_2d(std::string name) const;

    // Returns h5_wrapper of group at index i in members_
    const h5_wrapper& operator[] (unsigned i) const;

    // Returns name of the wrapped h5_group
    std::string name() const ;

private:
    // Pointer to the h5_group wrapped in h5_wrapper
    std::shared_ptr<h5_group> ptr_;

    // Vector of h5_wrappers around sub_groups of the h5_group
    std::vector<h5_wrapper> members_;

    // Map from name of dataset to index in vector of datasets in wrapped h5_group
    std::unordered_map<std::string, unsigned> dset_map_;

    // Map from name of sub-group to index in vector of sub-groups in wrapped h5_group
    std::unordered_map<std::string, unsigned> member_map_;
};

/// Class that stores sonata specific information about a collection of hdf5 files
class h5_record {
public:
    h5_record(const std::vector<std::shared_ptr<h5_file>>& files);

    // Verifies that the hdf5 files contain sonata edge information
    bool verify_edges();

    // Verifies that the hdf5 files contain sonata node information
    bool verify_nodes();

    // Returns total number of nodes/edges
    int num_elements() const;

    // Returns the population with name `name` in populations_
    const h5_wrapper& operator [](std::string name) const;

    // Returns the population at index `i` in populations_
    const h5_wrapper& operator [](int i) const;

    // Returns all populations
    std::vector<h5_wrapper> populations() const;

    // Returns partitioned sizes of every population in the h5_record
    std::vector<unsigned> partitions() const;

    // Returns map_
    std::unordered_map<std::string, unsigned> map() const;

    // Returns names of all populations_
    std::vector<std::string> pop_names() const;

private:
    // Total number of nodes/ edges
    int num_elements_ = 0;

    // Keep the original hdf5 files, to guarantee they remain open for the lifetime of a record
    std::vector<std::shared_ptr<h5_file>> files_;

    // Population names
    std::vector<std::string> pop_names_;

    // Partitioned sizes of the populations
    std::vector<unsigned> partition_;

    // Wrapped h5_group representing the top levels of every population
    std::vector<h5_wrapper> populations_;

    // Map from population name to index in populations_
    std::unordered_map<std::string, unsigned> map_;
};
