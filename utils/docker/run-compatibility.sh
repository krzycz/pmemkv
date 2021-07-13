#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020-2021, Intel Corporation

#
# run-compatibility.sh - verify compatibility between versions
#

set -e

TEST_DIR=${PMEMKV_TEST_DIR:-${DEFAULT_TEST_DIR}}

INSTALL_PREFIX=/opt

source `dirname ${0}`/prepare-for-build.sh

function build_and_install_pmemkv() {
	version=${1}
	git checkout ${version}

	mkdir ${WORKDIR}/build
	cd ${WORKDIR}/build

	cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo \
		-DBUILD_TESTS=OFF \
		-DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX}/pmemkv-${version}
	make -j$(nproc)
	sudo_password -S make -j$(nproc) install

	workspace_cleanup
}

function verify_compatibility() {
	version=${1}

	echo
	echo "##################################################################"
	echo "### Verifying compatibility with ${version}"
	echo "##################################################################"

	mkdir ${WORKDIR}/build
	cd ${WORKDIR}/build

	mkdir pmemkv-HEAD
	cd pmemkv-HEAD
	PKG_CONFIG_PATH=${INSTALL_PREFIX}/pmemkv-HEAD/lib64/pkgconfig cmake ../../tests/compatibility
	make -j$(nproc)
	cd ..

	mkdir pmemkv-${version}
	cd pmemkv-${version}
	PKG_CONFIG_PATH=${INSTALL_PREFIX}/pmemkv-${version}/lib64/pkgconfig cmake ../../tests/compatibility
	make -j$(nproc)
	cd ../..

	PMEM_IS_PMEM_FORCE=1 ${WORKDIR}/tests/compatibility/cmap.sh ${WORKDIR}/build/pmemkv-HEAD/cmap_compatibility ${WORKDIR}/build/pmemkv-${version}/cmap_compatibility ${TEST_DIR}/testfile

	workspace_cleanup
}

## Main:
# Fetch git history if clone is shallow
[ -f ${WORKDIR}/.git/shallow ] && git fetch --unshallow --tags

current_version=$(git describe --all)
echo "Current pmemkv version: ${current_version}"

echo "Build and install current pmemkv's version - 'HEAD'"
build_and_install_pmemkv "HEAD"

echo "Build and install older pmemkv's versions"
build_and_install_pmemkv "1.0.2"
build_and_install_pmemkv "1.1"
build_and_install_pmemkv "1.2"
build_and_install_pmemkv "1.3"
build_and_install_pmemkv "1.4"

# checkout HEAD/current version again
git checkout ${current_version}

echo "Test compatibility of previous versions with current one"
verify_compatibility "1.0.2"
verify_compatibility "1.1"
verify_compatibility "1.2"
verify_compatibility "1.3"
verify_compatibility "1.4"
