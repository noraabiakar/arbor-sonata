unset CDPATH
base_path=$(cd "${BASH_SOURCE[0]%/*}"; pwd)
prefix=${PREFIX:-$(pwd)}
environment=

while [ "$1" != "" ]
do
    case $1 in
        --env=* )
            environment=${1#--env=}
            ;;
        --env )
            shift
            environment=$1
            ;;
        * )
            echo "unknown option '$1'"
            exit 1
    esac
    shift
done

# Load utility functions and set up default environment.

source "$base_path/scripts/util.sh"
mkdir -p "$prefix"
prefix=$(full_path "$prefix")

source "$base_path/scripts/environment.sh"
default_environment

# Run a user supplied configuration script if it was provided with the -e flag.
# This will make changes to the configuration variables set in environment()

if [ "$environment" != "" ]; then
    msg "using additional configuration: $environment"
    if [ ! -f "$environment" ]; then
        err "file '$environment' not found"
        exit 1
    fi
    source "$environment"
    echo
fi

echo
msg "---- PATHS ----"
msg "root:            $base_path"
msg "work dir prefix: $prefix"
msg "install path:    $install_path"
msg "build path:      $build_path"
echo
msg "---- SYSTEM ----"
msg "system:          $system"
msg "using mpi:       $with_mpi"
msg "C compiler:      $cc"
msg "C++ compiler:    $cxx"
msg "python:          $python"
echo
msg "---- ARBOR ----"
msg "repo:            $arb_git_repo"
msg "branch:          $arb_branch"
msg "arch:            $arb_arch"
msg "gpu:             $arb_with_gpu"
msg "vectorize:       $arb_vectorize"
echo

mkdir -p "$build_path"

# Record system configuration name, timestamp.
# (This data will also be recorded in constructed environments.)

# Note: format (and sed processing) chosen to match RFC 3339 profile
# for ISO 8601 date formatting, matching GNU date -Isec output.
timestamp=$(date +%Y-%m-%ST%H:%M:%S%z | sed 's/[0-9][0-9]$/:&/')
echo "$timestamp" > "$build_path/timestamp"
echo "${sysname:=$(hostname -s)}" > "$build_path/sysname"

# Build simulator targets.

export CC="$cc"
export CXX="$cxx"

echo && source "$base_path/scripts/build_arbor.sh"
cd "$base_path"

echo
msg "Installation finished"
echo

