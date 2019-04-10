module swap PrgEnv-cray PrgEnv-gnu
module load daint-mc
module load CMake
module load h5py
module swap gcc/7.3.0

export CC=$(which cc)
export CXX=$(which CC)
export CRAYPE_LINK_TYPE=dynamic
export HDF5_USE_FILE_LOCKING=FALSE

