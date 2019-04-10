### environment ###

# set up environment for building on the multicore part of daint

[ "$PE_ENV" = "CRAY" ] && module swap PrgEnv-cray PrgEnv-gnu
module load daint-mc
module load CMake
module load h5py

# PyExtensions is needed for cython, mpi4py and others.
# It loads cray-python/3.6.5.1
module load PyExtensions/3.6.5.1-CrayGNU-18.08
python=$(which python3)

# load after python tools because easybuild...
module swap gcc/7.3.0

### compilation options ###

cc=$(which cc)
cxx=$(which CC)
with_mpi=ON

arb_arch=broadwell

export CRAYPE_LINK_TYPE=dynamic

makej=20

### benchmark execution options ###

threads_per_core=2
cores_per_socket=18
sockets=2
threads_per_socket=36

run_with_mpi() {
    echo ARB_NUM_THREADS=$threads_per_socket srun -n $sockets -c $threads_per_socket $*
    ARB_NUM_THREADS=$threads_per_socket srun -n $sockets -c $threads_per_socket $*
}
