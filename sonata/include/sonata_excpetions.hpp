#pragma once

#include <stdexcept>
#include <sstream>
#include <string>

namespace impl {
    inline void pprintf_(std::ostringstream& o, const char* s) {
        o << s;
    }

    template <typename T, typename... Tail>
    void pprintf_(std::ostringstream& o, const char* s, T&& value, Tail&&... tail) {
        const char* t = s;
        while (*t && !(*t=='{' && t[1]=='}')) {
            ++t;
        }
        o.write(s, t-s);
        if (*t) {
            o << std::forward<T>(value);
            pprintf_(o, t+2, std::forward<Tail>(tail)...);
        }
    }
}

template <typename... Args>
std::string pprintf(const char *s, Args&&... args) {
    std::ostringstream o;
    impl::pprintf_(o, s, std::forward<Args>(args)...);
    return o.str();
}

struct sonata_exception: std::runtime_error {
    sonata_exception(const std::string& what_arg):
            std::runtime_error(what_arg) {}
};

struct sonata_dataset_exception: sonata_exception {
    sonata_dataset_exception(const std::string& name) :
        sonata_exception(pprintf("Dataset \"{}\" can not be opened/read", name)) {}

    sonata_dataset_exception(const std::string& name, unsigned index) :
        sonata_exception(pprintf("Dataset \"{}\" accessed out of bounds at: {}", name, index)) {}

    sonata_dataset_exception(const std::string& name, unsigned i, unsigned j) :
        sonata_exception(pprintf("Dataset \"{}\" accessed out of bounds between: {} and {}", name, i, j)) {}

};

