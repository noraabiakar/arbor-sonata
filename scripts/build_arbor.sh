arb_repo_path=$_build_path/arbor
arb_build_path=$arb_repo_path/build
arb_checked_flag="${arb_repo_path}/checked_out"

# clear log file from previous builds
out="$_build_path/log_arbor"
rm -f "$out"

# aquire the code if it has not already been downloaded
if [ ! -f "$arb_checked_flag" ]; then
    rm -rf "$arb_repo_path"

    # clone the repository
    msg "ARBOR: cloning from $_arb_git_repo"
    git clone "$_arb_git_repo" "$arb_repo_path" --recursive >> "$out" 2>&1
    [ $? != 0 ] && exit_on_error "see ${out}"

    # check out the branch
    if [ "$_arb_branch" != "master" ]; then
        msg "ARBOR: check out branch $_arb_branch"
        cd "$arb_repo_path"
        git checkout "$_arb_branch" >> "$out" 2>&1
        [ $? != 0 ] && exit_on_error "see ${out}"
    fi
    touch "${arb_checked_flag}"
else
    msg "ARBOR: repository has already downloaded"
fi

# remove old build files
mkdir -p "$arb_build_path"

# configure the build with cmake
cd "$arb_build_path"
cmake_args=-DCMAKE_INSTALL_PREFIX:PATH="$_install_path"
cmake_args="$cmake_args -DARB_WITH_MPI=$_with_mpi"
cmake_args="$cmake_args -DARB_WITH_GPU=$_arb_with_gpu"
cmake_args="$cmake_args -DARB_ARCH=$_arb_arch"
cmake_args="$cmake_args -DARB_VECTORIZE=$_arb_vectorize"
msg "ARBOR: cmake $cmake_args"
cmake "$arb_repo_path" $cmake_args >> "$out" 2>&1
[ $? != 0 ] && exit_on_error "see ${out}"

cd "$arb_build_path"

msg "ARBOR: build"
make -j $_makej >> "$out" 2>&1
[ $? != 0 ] && exit_on_error "see ${out}"

msg "ARBOR: install"
make install >> "$out" 2>&1
[ $? != 0 ] && exit_on_error "see ${out}"

src_path="$arb_build_path/bin"
dst_path="$_install_path/bin"

msg "ARBOR: library build completed"
cd $_base_path
