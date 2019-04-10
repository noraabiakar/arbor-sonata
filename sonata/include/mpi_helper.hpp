#pragma once

#include <type_traits>
#include <vector>
#include <numeric>

#include <mpi.h>

#define MPI_OR_THROW(fn, ...)\
while (int r_ = fn(__VA_ARGS__)) throw sonata_exception("MPI error");

int rank(MPI_Comm comm) {
    int r;
    MPI_OR_THROW(MPI_Comm_rank, comm, &r);
    return r;
}

int size(MPI_Comm comm) {
    int s;
    MPI_OR_THROW(MPI_Comm_size, comm, &s);
    return s;
}

void barrier(MPI_Comm comm) {
    MPI_OR_THROW(MPI_Barrier, comm);
}

// Type traits for automatically setting MPI_Datatype information for C++ types.
template <typename T>
struct mpi_traits {
    constexpr static size_t count() {
        return sizeof(T);
    }
    constexpr static MPI_Datatype mpi_type() {
        return MPI_CHAR;
    }
    constexpr static bool is_mpi_native_type() {
        return false;
    }
};

#define MAKE_TRAITS(T,M)     \
template <>                 \
struct mpi_traits<T> {  \
    constexpr static size_t count()            { return 1; } \
    /* constexpr */ static MPI_Datatype mpi_type()   { return M; } \
    constexpr static bool is_mpi_native_type() { return true; } \
};

MAKE_TRAITS(float,              MPI_FLOAT)
MAKE_TRAITS(double,             MPI_DOUBLE)
MAKE_TRAITS(char,               MPI_CHAR)
MAKE_TRAITS(int,                MPI_INT)
MAKE_TRAITS(unsigned,           MPI_UNSIGNED)
MAKE_TRAITS(long,               MPI_LONG)
MAKE_TRAITS(unsigned long,      MPI_UNSIGNED_LONG)
MAKE_TRAITS(long long,          MPI_LONG_LONG)
MAKE_TRAITS(unsigned long long, MPI_UNSIGNED_LONG_LONG)

static_assert(std::is_same<std::size_t, unsigned long>::value ||
              std::is_same<std::size_t, unsigned long long>::value,
              "size_t is not the same as unsigned long or unsigned long long");

template <typename C>
C make_index(C const& c)
{
    static_assert(
            std::is_integral<typename C::value_type>::value,
            "make_index only applies to integral types"
    );

    C out(c.size()+1);
    out[0] = 0;
    std::partial_sum(c.begin(), c.end(), out.begin()+1);
    return out;
}

template <typename T>
std::vector<T> gather_all(T value, MPI_Comm comm) {
    using traits = mpi_traits<T>;
    std::vector<T> buffer(size(comm));

    MPI_OR_THROW(MPI_Allgather,
                 &value,        traits::count(), traits::mpi_type(), // send buffer
                 buffer.data(), traits::count(), traits::mpi_type(), // receive buffer
                 comm);

    return buffer;
}

template <typename T>
std::vector<T> gather_all(const std::vector<T>& values, MPI_Comm comm) {

    using traits = mpi_traits<T>;
    auto counts = gather_all(int(values.size()), comm);
    for (auto& c : counts) {
        c *= traits::count();
    }
    auto displs = make_index(counts);

    std::vector<T> buffer(displs.back()/traits::count());
    MPI_OR_THROW(MPI_Allgatherv,
    // const_cast required for MPI implementations that don't use const* in their interfaces
                 const_cast<T*>(values.data()), counts[rank(comm)], traits::mpi_type(),  // send buffer
                 buffer.data(), counts.data(), displs.data(), traits::mpi_type(), // receive buffer
                 comm);

    return buffer;
}
