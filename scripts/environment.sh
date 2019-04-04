# Set the base paths to working directories.
# Variables defined here use the prefix _
set_working_paths() {
    if [ -z "$_prefix" ]; then
        echo "error: empty _prefix"
        exit 1
    fi

    # Paths to working directories
    export _install_path="$_prefix/install"
    export _build_path="$_prefix/build"
}

# Sets up the default enviroment.
# Variables defined here use the prefix _
default_environment() {
    set_working_paths

    # Detect OS
    case "$OSTYPE" in
      linux*)   _system=linux ;;
      darwin*)  _system=apple ;;
      *)        err "unsuported OS: $OSTYPE"; exit 1 ;;
    esac

    # Choose compiler based on OS
    if [ "$_system" = "linux" ]; then
        _cc=$(which gcc)
        _cxx=$(which g++)
    elif [ "$_system" = "apple" ]; then
        _cc=$(which clang)
        _cxx=$(which clang++)
    fi

    # use MPI if we can find it
    _with_mpi=OFF
    command -v mpicc &> /dev/null
    [ $? = 0 ] && command -v mpic++ &> /dev/null
    if [ $? = 0 ]; then
        _with_mpi=ON
        _cc=$(which mpicc)
        _cxx=$(which mpic++)
    fi

    # set the number of parallel build tasks
    _makej=6

    # Arbor specific

    _arb_git_repo=https://github.com/noraabiakar/arbor.git
    _arb_branch=sonata

    _arb_arch=native
    _arb_with_gpu=OFF
    _arb_vectorize=ON
}

