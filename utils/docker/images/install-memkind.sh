#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2021, Intel Corporation

#
# install-memkind.sh - installs memkind from sources; depends on
#		the system it uses proper installation parameters
#

set -e

# contains new libmemkind namespace
MEMKIND_VERSION=${1:v1.11.0}
WORKDIR=${2:$(pwd)}
REPO=${3:-"https://github.com/memkind/memkind"}
MEMKIND_DIR=${WORKDIR}/memkind

git clone ${REPO} ${MEMKIND_DIR}
pushd ${MEMKIND_DIR}
git checkout ${MEMKIND_VERSION}

echo "set OS-specific configure options"
OS_SPECIFIC=""
case $(echo ${OS} | cut -d'-' -f1) in
	centos|opensuse)
		OS_SPECIFIC="--libdir=/usr/lib64"
		;;
esac

./autogen.sh
./configure --prefix=/usr ${OS_SPECIFIC}
make -j$(nproc)
make -j$(nproc) install

echo "cleanup:"
popd
rm -r ${MEMKIND_DIR}
