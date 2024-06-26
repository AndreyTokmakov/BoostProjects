
# ==============================================================================================================================

git clone git@github.com:boostorg/boost.git

cd boost
git submodule update --init


git checkout tags/boost-1.85.0

# ==============================================================================================================================
															GCC  | CMAKE
# ==============================================================================================================================

export GCC_VERSION=13.2
export GCC_PATH=/home/andtokm/DiskS/Utils/bin/gcc-$GCC_VERSION

export PATH=${GCC_PATH}/bin:${PATH}
export PATH=/home/andtokm/DiskS/Utils/cmake/cmake-3.25.1/bin/:${PATH}

export LD_LIBRARY_PATH=${GCC_PATH}/lib64
export CC=gcc-$GCC_VERSION CXX=g++-$GCC_VERSION

# ==============================================================================================================================
															Configure | Gcc-13.2
# ==============================================================================================================================

# ./bootstrap.sh

./bootstrap.sh --with-toolset=gcc --with-libraries=all
./b2 --toolset=gcc-13.2

# ==============================================================================================================================
															Validate
# ==============================================================================================================================

> ldd b2


	linux-vdso.so.1 (0x00007ffca0dc2000)
		libstdc++.so.6 => /home/andtokm/DiskS/Utils/bin/gcc-13.2/lib64/libstdc++.so.6 (0x00007f334dd45000)
		libgcc_s.so.1 => /home/andtokm/DiskS/Utils/bin/gcc-13.2/lib64/libgcc_s.so.1 (0x00007f334dd1f000)
		libpthread.so.0 => /lib/x86_64-linux-gnu/libpthread.so.0 (0x00007f334dcd2000)
		libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f334dae0000)
		libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007f334d991000)
		/lib64/ld-linux-x86-64.so.2 (0x00007f334e238000)


# =======================================================================================================================================
															Build
# =======================================================================================================================================

./b2 --toolset=gcc stage threading=multi link=shared

# Build
./b2

./b2 --show-libraries

# ------------------------------------------------ install to dir -------------------------------------------


./b2 install --libdir=/home/andtokm/DiskS/ProjectsUbuntu/third_party/boost/output/libs --includedir=/home/andtokm/DiskS/ProjectsUbuntu/third_party/boost/output/include




=======================================================================================================================================
														Cmake
=======================================================================================================================================

# Set to ON to disable searching in locations not specified by these hint variables. Default is OFF.
set (Boost_NO_SYSTEM_PATHS ON)

# Для отладки

set(Boost_DEBUG ON)
set(Boost_VERBOSE ON)
