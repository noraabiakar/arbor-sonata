module swap PrgEnv-cray PrgEnv-gnu
module load daint-gpu
module load CMake
module load h5py
module load cudatoolkit/9.2.148_3.19-6.0.7.1_2.1__g3d9acc8
module swap gcc/6.2.0

export CC=$(which cc)
export CXX=$(which CC)
export CRAYPE_LINK_TYPE=dynamic
export HDF5_USE_FILE_LOCKING=FALSE
