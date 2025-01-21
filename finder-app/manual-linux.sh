#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
TOOLCHAIN_BIN_PATH=$(dirname "$(which ${CROSS_COMPILE}gcc)")
TOOLCHAIN_PATH=$(dirname "${TOOLCHAIN_BIN_PATH}")

# Verify the toolchain path
if [ -z "${TOOLCHAIN_PATH}" ]; then
    echo "Error: Cross-compiler toolchain not found in PATH."
    exit 1
fi

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd ${OUTDIR}
if [ ! -d "linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs

    cd ..
fi

echo "Adding the Image in outdir"
cp linux-stable/arch/${ARCH}/boot/Image .

echo "Creating the staging directory for the root filesystem"
if [ -d "rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf rootfs
fi

mkdir -p rootfs/{bin,dev,etc,home,lib,lib64,proc,sbin,sys,tmp,usr/{bin,lib,sbin},var/log}

if [ ! -d "busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
else
    cd busybox
fi

make distclean
make defconfig
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=../rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

cd ../rootfs
echo "Library dependencies"
# Extract required libraries and program interpreter
PROGRAM_INTERPRETER=$(${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter" |  awk -F': ' '{print $2}' | tr -d '[]')
SHARED_LIBRARIES=$(${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library" | awk -F'[][]' '{print $2}')

# Copy program interpreter
echo "Program interpreter: ${PROGRAM_INTERPRETER}"
INTERPRETER_PATH=$(find ${TOOLCHAIN_PATH} -type f -name $(basename ${PROGRAM_INTERPRETER}))
if [ -n ${INTERPRETER_PATH} ]; then
  echo "Copying ${INTERPRETER_PATH} to rootfs/lib"
  cp ${INTERPRETER_PATH} lib
else
  echo "Error: Program interpreter ${PROGRAM_INTERPRETER} not found in ${TOOLCHAIN_PATH}"
fi

# Copy shared libraries
for LIB in ${SHARED_LIBRARIES}; do
  echo "Required shared library: ${LIB}"
  LIB_PATH=$(find ${TOOLCHAIN_PATH} -type f -name $(basename ${LIB}))
  if [ -n ${LIB_PATH} ]; then
    echo "Copying ${LIB_PATH} to rootfs/lib64"
    cp ${LIB_PATH} lib64
  else
    echo "Error: Shared library ${LIB} not found in ${TOOLCHAIN_PATH}"
  fi
done

echo "Dependency setup completed"

sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1

echo "Cross-compiling the writer utility"
ROOT_FS_PATH=$(pwd)
cd ${FINDER_APP_DIR}
make clean
CROSS_COMPILE=${CROSS_COMPILE} make

echo "Copying the finder related scripts and executables to the /home directory"
cp -R finder.sh finder-test.sh writer autorun-qemu.sh conf/ ${ROOT_FS_PATH}/home/
cd ${ROOT_FS_PATH}
sudo chown -R root:root *

find . | cpio -H newc -ov --owner root:root > ../initramfs.cpio
gzip -f ../initramfs.cpio
