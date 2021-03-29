#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2021, Intel Corporation

#
# run-test-building.sh - is called inside a Docker container,
#                        starts testing of pmemkv building
#                        and automatic update of the documentation.
#

set -e

source `dirname $0`/prepare-for-build.sh

# params set for the file (if not previously set, the right-hand param is used)
TEST_DIR=${PMEMKV_TEST_DIR:-${DEFAULT_TEST_DIR}}
TEST_PACKAGES=${TEST_PACKAGES:-ON}
BUILD_JSON_CONFIG=${BUILD_JSON_CONFIG:-ON}
TESTS_USE_FORCED_PMEM=${TESTS_USE_FORCED_PMEM:-ON}
TEST_TIMEOUT=${TEST_TIMEOUT:-600}

###############################################################################
# BUILD test_gcc_cpp20
###############################################################################
function test_gcc_cpp20() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"
	CWD=$(pwd)
	mkdir $WORKDIR/build
	cd $WORKDIR/build

	CC=gcc CXX=g++ cmake .. -DCMAKE_BUILD_TYPE=Release \
		-DTEST_DIR=$TEST_DIR \
		-DCMAKE_INSTALL_PREFIX=$PREFIX \
		-DBUILD_JSON_CONFIG=${BUILD_JSON_CONFIG} \
		-DCOVERAGE=$COVERAGE \
		-DDEVELOPER_MODE=1 \
		-DTESTS_USE_FORCED_PMEM=${TESTS_USE_FORCED_PMEM} \
		-DCXX_STANDARD=20

	make -j$(nproc)
	# Run basic tests
	ctest -R "SimpleTest" --output-on-failure --timeout ${TEST_TIMEOUT}

	if [ "$COVERAGE" == "1" ]; then
		upload_codecov test_gcc_cpp20
	fi

	workspace_cleanup

	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

###############################################################################
# BUILD test_building_of_packages
###############################################################################
function test_building_of_packages() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"

	# Fetch git history for `git describe` to work,
	# so that package has proper 'version' field
	[ -f .git/shallow ] && git fetch --unshallow --tags

	mkdir $WORKDIR/build
	cd $WORKDIR/build

	cmake .. -DCMAKE_BUILD_TYPE=Debug \
		-DTEST_DIR=$TEST_DIR \
		-DCMAKE_INSTALL_PREFIX=$PREFIX \
		-DDEVELOPER_MODE=1 \
		-DBUILD_JSON_CONFIG=${BUILD_JSON_CONFIG} \
		-DCPACK_GENERATOR=$PACKAGE_MANAGER

	echo
	echo "### Making sure there is no libpmemkv currently installed"
	echo "---------------------------- Error expected! ------------------------------"
	compile_example_standalone pmemkv_basic_cpp && exit 1
	echo "---------------------------------------------------------------------------"

	make -j$(nproc) package

	if [ $PACKAGE_MANAGER = "deb" ]; then
		sudo_password dpkg -i libpmemkv*.deb
	elif [ $PACKAGE_MANAGER = "rpm" ]; then
		sudo_password rpm -i libpmemkv*.rpm
	fi

	# Verify installed packages
	compile_example_standalone pmemkv_basic_c
	run_example_standalone pmemkv_basic_c pool
	compile_example_standalone pmemkv_basic_cpp
	run_example_standalone pmemkv_basic_cpp pool

	# Clean after installation
	if [ $PACKAGE_MANAGER = "deb" ]; then
		sudo_password dpkg -r libpmemkv-dev
	elif [ $PACKAGE_MANAGER = "rpm" ]; then
		sudo_password rpm -e --nodeps libpmemkv-devel
	fi

	workspace_cleanup

	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

# helper function to check building with specified CMake flag
function build_with_flags() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"

	CMAKE_FLAGS_AND_SETTINGS=$@
	echo
	echo "##############################################################"
	echo "### Verifying building with flag: ${CMAKE_FLAGS_AND_SETTINGS}"
	echo "##############################################################"

	mkdir $WORKDIR/build
	cd $WORKDIR/build

	cmake .. ${CMAKE_FLAGS_AND_SETTINGS}
	make -j$(nproc)
	# list all tests in this build
	ctest -N

	workspace_cleanup

	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

# Main:

workspace_cleanup
cd $WORKDIR

# CXX_STANDARD==20 is supported since CMake 3.12
if [ $CMAKE_VERSION_NUMBER -ge 312 ]; then
	test_gcc_cpp20
fi

echo
echo "##############################################################"
echo "### Verifying if each engine is building properly"
echo "##############################################################"
engines_flags=(
	ENGINE_VSMAP
	ENGINE_VCMAP
	ENGINE_CMAP
	ENGINE_CSMAP
	ENGINE_STREE
	ENGINE_TREE3
	ENGINE_RADIX
	ENGINE_ROBINHOOD
	ENGINE_DRAM_VCMAP
	ENGINE_DRAM_VCSMAP
	# the last item is to test all engines disabled
	BLACKHOLE_TEST
)

for engine_flag in "${engines_flags[@]}"
do
	mkdir $WORKDIR/build
	cd $WORKDIR/build
	# testing each engine separately; disabling default engines
	echo
	echo "##############################################################"
	echo "### Verifying building of the '$engine_flag' engine"
	echo "##############################################################"
	cmake .. -DCXX_STANDARD=17 \
		-DENGINE_VSMAP=OFF \
		-DENGINE_VCMAP=OFF \
		-DENGINE_CMAP=OFF \
		-DENGINE_CSMAP=OFF \
		-DBUILD_JSON_CONFIG=${BUILD_JSON_CONFIG} \
		-D$engine_flag=ON
	make -j$(nproc)
	# list all tests in this build
	ctest -N
	ctest -R wrong_engine_name_test --output-on-failure --timeout ${TEST_TIMEOUT}

	workspace_cleanup
done

echo
echo "##############################################################"
echo "### Verifying building of all engines"
echo "##############################################################"
mkdir $WORKDIR/build
cd $WORKDIR/build

cmake .. -DCXX_STANDARD=17 \
	-DENGINE_VSMAP=ON \
	-DENGINE_VCMAP=ON \
	-DENGINE_CMAP=ON \
	-DENGINE_CSMAP=ON \
	-DENGINE_STREE=ON \
	-DENGINE_TREE3=ON \
	-DENGINE_RADIX=ON \
	-DENGINE_ROBINHOOD=ON \
	-DENGINE_DRAM_VCMAP=ON \
	-DENGINE_DRAM_VCSMAP=ON \
	-DBUILD_JSON_CONFIG=${BUILD_JSON_CONFIG}
make -j$(nproc)
# list all tests in this build
ctest -N

workspace_cleanup

echo
echo "##############################################################"
echo "### Verifying build with specific CMake flags"
echo "##############################################################"
build_with_flags -DBUILD_JSON_CONFIG=OFF -DTESTS_JSON=OFF

# building of packages should be verified only if PACKAGE_MANAGER equals 'rpm' or 'deb'
case $PACKAGE_MANAGER in
	rpm|deb)
		[ "$TEST_PACKAGES" == "ON" ] && test_building_of_packages
		;;
	*)
		echo "Notice: skipping building of packages because PACKAGE_MANAGER is not equal 'rpm' nor 'deb' ..."
		;;
esac
