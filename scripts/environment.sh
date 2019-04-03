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
    export _cache_path="$_prefix/cache"
    export _input_path="$_prefix/input"
    export _config_path="$_prefix/config"
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

    _arb_git_repo=https://github.com/arbor-sim/arbor.git
    _arb_branch=v0.2

    _arb_arch=native
    _arb_with_gpu=OFF
    _arb_vectorize=ON
}

find_installed_paths() {
    find "$_install_path" -type d -name "$1" | awk -v ORS=: '{print}'
}

# Save the environment used to build a simulation engine
# to a shell script that can be used to reproduce that
# environment for running the simulation engine.
#
# Record prefix to writable data (_prefix) and other
# installation-time information, viz. _timestamp and
# _sysname.
# 
# Take name of simulation engine (one of: {arb, nrn, corenrn}) as
# first argument; any additional arguments are appended to the
# generated config script verbatim.
save_environment() {
    set_working_paths
    sim="$1"
    shift

    # Find and record python, bin, and lib paths.
    python_path=$(find_installed_paths site-packages)"$_base_path/common/python:"
    bin_path=$(find_installed_paths bin)"$_base_path/common/bin:"
    lib_path=$(find_installed_paths lib)$(find_installed_paths lib64)

    config_file="${_config_path}/env_${sim}.sh"
    mkdir -p "$_config_path"

    source_env_script=
    if [ -n "$_environment" ]; then
        source_env_script='source '$(full_path "$_environment")
    fi

    pyvenv_activate=$_pyvenv_path/bin/activate
    source_pyvenv_script=
    if [ -r "$pyvenv_activate" ]; then
	source_pyvenv_script="source '$pyvenv_activate'"
    fi

    cat <<_end_ > "$_config_path/env_$sim.sh"
export _prefix="$_prefix"
export _timestamp="$_timestamp"
export _sysname="$_sysname"
export PATH="$bin_path\${PATH}"
export PYTHONPATH="$python_path\$PYTHONPATH"
export PATH="$bin_path\$PATH"
export LD_LIBRARY_PATH="${lib_path}:\$LD_LIBRARY_PATH"
source "$_base_path/scripts/environment.sh"
default_environment
$source_env_script
$source_pyvenv_script
_end_

    for appendix in "${@}"; do
        echo "$appendix" >> "$_config_path/env_$sim.sh"
    done
}

