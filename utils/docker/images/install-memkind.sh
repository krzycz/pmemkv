#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2021, Intel Corporation

#
# install-memkind.sh - installs memkind from sources; depends on
#		the system it uses proper installation parameters
#

set -e

# contains better error handling in pmem_allocator
MEMKIND_VERSION=v1.12.0-rc1

WORKDIR=$(pwd)

git clone https://github.com/memkind/memkind
cd ${WORKDIR}/memkind
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
cd ${WORKDIR}
rm -r memkind
