unset CDPATH
_base_path=$(cd "${BASH_SOURCE[0]%/*}"; pwd)
_prefix=${PREFIX:-$(pwd)}

# Load utility functions and set up default environment.

source "$_base_path/scripts/util.sh"
mkdir -p "$_prefix"
_prefix=$(full_path "$_prefix")

source "$_base_path/scripts/environment.sh"
default_environment

echo
msg "---- PATHS ----"
msg "root:            $_base_path"
msg "work dir prefix: $_prefix"
msg "install path:    $_install_path"
msg "build path:      $_build_path"
msg "input path:      $_input_path"
msg "output path:     $_output_path"
echo
msg "---- SYSTEM ----"
msg "system:          $_system"
msg "using mpi:       $_with_mpi"
msg "C compiler:      $_cc"
msg "C++ compiler:    $_cxx"
msg "python:          $_python"
echo
msg "---- ARBOR ----"
msg "repo:            $_arb_git_repo"
msg "branch:          $_arb_branch"
msg "arch:            $_arb_arch"
msg "gpu:             $_arb_with_gpu"
msg "vectorize:       $_arb_vectorize"
echo

mkdir -p "$_build_path"

# Record system configuration name, timestamp.
# (This data will also be recorded in constructed environments.)

# Note: format (and sed processing) chosen to match RFC 3339 profile
# for ISO 8601 date formatting, matching GNU date -Isec output.
_timestamp=$(date +%Y-%m-%ST%H:%M:%S%z | sed 's/[0-9][0-9]$/:&/')
echo "$_timestamp" > "$_build_path/timestamp"
echo "${_sysname:=$(hostname -s)}" > "$_build_path/sysname"

# Build simulator targets.

export CC="$_cc"
export CXX="$_cxx"

echo && source "$_base_path/scripts/build_arbor.sh"
cd "$_base_path"

echo
msg "Installation finished"
echo

