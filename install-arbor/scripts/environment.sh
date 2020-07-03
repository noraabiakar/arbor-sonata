# Set the base paths to working directories.
# Variables defined here use the prefix _
set_working_paths() {
    if [ -z "$prefix" ]; then
        echo "error: empty prefix"
        exit 1
    fi

    # Paths to working directories
    export install_path="$prefix/install"
    export build_path="$prefix/build"
}

# Sets up the default enviroment.
# Variables defined here use the prefix _
default_environment() {
    set_working_paths

    # Detect OS
    case "$OSTYPE" in
      linux*)   system=linux ;;
      darwin*)  system=apple ;;
      *)        err "unsuported OS: $OSTYPE"; exit 1 ;;
    esac

    # Choose compiler based on OS
    if [ "$system" = "linux" ]; then
        cc=$(which gcc)
        cxx=$(which g++)
    elif [ "$system" = "apple" ]; then
        cc=$(which clang)
        cxx=$(which clang++)
    fi

    # use MPI if we can find it
    with_mpi=OFF
    command -v mpicc &> /dev/null
    [ $? = 0 ] && command -v mpic++ &> /dev/null
    if [ $? = 0 ]; then
        with_mpi=ON
        cc=$(which mpicc)
        cxx=$(which mpic++)
    fi

    # set the number of parallel build tasks
    makej=6

    # Arbor specific

    arb_git_repo=https://github.com/arbor-sim/arbor.git
    arb_branch=master

    arb_arch=native
    arb_with_gpu=OFF
    arb_vectorize=ON
}

