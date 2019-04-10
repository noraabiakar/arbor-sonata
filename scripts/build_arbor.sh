arb_repo_path=$build_path/arbor
arb_build_path=$arb_repo_path/build
arb_checked_flag="${arb_repo_path}/checked_out"

# clear log file from previous builds
out="$build_path/log_arbor"
rm -f "$out"

# aquire the code if it has not already been downloaded
if [ ! -f "$arb_checked_flag" ]; then
    rm -rf "$arb_repo_path"

    # clone the repository
    msg "ARBOR: cloning from $arb_git_repo"
    git clone "$arb_git_repo" "$arb_repo_path" --recursive >> "$out" 2>&1
    [ $? != 0 ] && exit_on_error "see ${out}"

    # check out the branch
    if [ "$arb_branch" != "master" ]; then
        msg "ARBOR: check out branch $arb_branch"
        cd "$arb_repo_path"
        git checkout "$arb_branch" >> "$out" 2>&1
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
cmake_args=-DCMAKE_INSTALL_PREFIX:PATH="$install_path"
cmake_args="$cmake_args -DARB_WITH_MPI=$with_mpi"
cmake_args="$cmake_args -DARB_WITH_GPU=$arb_with_gpu"
cmake_args="$cmake_args -DARB_ARCH=$arb_arch"
cmake_args="$cmake_args -DARB_VECTORIZE=$arb_vectorize"
cmake_args="$cmake_args -DARB_WITH_PROFILING=OFF"
msg "ARBOR: cmake $cmake_args"
cmake "$arb_repo_path" $cmake_args >> "$out" 2>&1
[ $? != 0 ] && exit_on_error "see ${out}"

cd "$arb_build_path"

msg "ARBOR: build"
make -j $makej >> "$out" 2>&1
[ $? != 0 ] && exit_on_error "see ${out}"

msg "ARBOR: install"
make install >> "$out" 2>&1
[ $? != 0 ] && exit_on_error "see ${out}"

src_path="$arb_build_path/bin"
dst_path="$install_path/bin"

msg "ARBOR: library build completed"
cd $base_path
