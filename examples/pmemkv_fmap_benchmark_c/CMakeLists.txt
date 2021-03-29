# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019, Intel Corporation

cmake_minimum_required(VERSION 3.5)
project(pmemkv_fmap_benchmark C)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	pkg_check_modules(LIBPMEMKV REQUIRED libpmemkv)
else()
	message(FATAL_ERROR "pkg-config not found")
endif()

include_directories(${LIBPMEMKV_INCLUDE_DIRS})
link_directories(${LIBPMEMKV_LIBRARY_DIRS})
add_executable(pmemkv_fmap_benchmark_c pmemkv_fmap_benchmark.c)
target_link_libraries(pmemkv_fmap_benchmark_c ${LIBPMEMKV_LIBRARIES})
