### environment ###

# set up environment for building on the multicore part of daint

[ "$PE_ENV" = "CRAY" ] && module swap PrgEnv-cray PrgEnv-gnu
module load daint-gpu
module load CMake
module load h5py

module load cudatoolkit/9.2.148_3.19-6.0.7.1_2.1__g3d9acc8

# PyExtensions is needed for cython, mpi4py and others.
# It loads cray-python/3.6.5.1 which points python at version 3.6.1.1
module load PyExtensions/3.6.5.1-CrayGNU-18.08
_python=$(which python3)

# load after python tools because easybuild...
module swap gcc/6.2.0

### compilation options ###

_cc=$(which cc)
_cxx=$(which CC)
_with_mpi=ON

_arb_with_gpu=ON
_arb_arch=haswell

export CRAYPE_LINK_TYPE=dynamic

_makej=20

### benchmark execution options ###

_threads_per_core=2
_cores_per_socket=12
_sockets=1
_threads_per_socket=24

run_with_mpi() {
    echo ARB_NUM_THREADS=$ns_threads_per_socket srun -n $ns_sockets -N $ns_sockets -c $ns_threads_per_socket $*
    ARB_NUM_THREADS=$ns_threads_per_socket srun -n $ns_sockets -N $ns_sockets -c $ns_threads_per_socket $*
}
