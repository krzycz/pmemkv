# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2021, Intel Corporation

#
# examples/CMakeLists.txt - CMake file for building all examples along with
#	the current pmemkv sources, for verification purposes.
#	To build an example as a standalone application (with pmemkv from the system)
#	see a CMake file in any of the subdirectories.
#
add_custom_target(examples)

# ----------------------------------------------------------------- #
## Setup examples
# ----------------------------------------------------------------- #
add_common_flag(-Wno-unused-but-set-variable)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)
add_dependencies(examples pmemkv)

# Add developer checks
add_cppstyle(examples ${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_basic_c/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_basic_cpp/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_pmemobj_cpp/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_config_c/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_open_cpp/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_comparator_cpp/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_comparator_c/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_transaction_cpp/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_transaction_c/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_iterator_c/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_iterator_cpp/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_fmap_cpp/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_benchmark_c/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_fmap_benchmark_c/*.c)

add_check_whitespace(examples ${CMAKE_CURRENT_SOURCE_DIR}/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_basic_c/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_basic_cpp/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_pmemobj_cpp/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_config_c/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_open_cpp/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_comparator_cpp/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_comparator_c/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_transaction_cpp/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_transaction_c/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_iterator_c/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_iterator_cpp/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_fmap_cpp/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_benchmark_c/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_fmap_benchmark_c/*.c)

function(add_example name)
	set(srcs ${ARGN})
	prepend(srcs ${CMAKE_CURRENT_SOURCE_DIR} ${srcs})
	add_executable(example-${name} ${srcs})
	add_dependencies(examples example-${name})
endfunction()

# ----------------------------------------------------------------- #
## Add examples (if engine used in specific example is enabled)
# ----------------------------------------------------------------- #
if(ENGINE_CMAP)
	add_example(pmemkv_basic_cpp pmemkv_basic_cpp/pmemkv_basic.cpp)
	target_link_libraries(example-pmemkv_basic_cpp pmemkv)

	add_example(pmemkv_basic_c pmemkv_basic_c/pmemkv_basic.c)
	target_link_libraries(example-pmemkv_basic_c pmemkv)

	add_example(pmemkv_benchmark_c pmemkv_benchmark_c/pmemkv_benchmark.c)
	target_link_libraries(example-pmemkv_benchmark_c pmemkv)

	add_example(pmemkv_pmemobj_cpp pmemkv_pmemobj_cpp/pmemkv_pmemobj.cpp)
	target_link_libraries(example-pmemkv_pmemobj_cpp pmemkv ${LIBPMEMOBJ++_LIBRARIES})

	add_example(pmemkv_basic_config_c pmemkv_config_c/pmemkv_basic_config.c)
	target_link_libraries(example-pmemkv_basic_config_c pmemkv ${LIBPMEMOBJ++_LIBRARIES})

	if(BUILD_JSON_CONFIG)
		add_example(pmemkv_config_c pmemkv_config_c/pmemkv_config.c)
		target_link_libraries(example-pmemkv_config_c pmemkv pmemkv_json_config)
	endif()

	add_example(pmemkv_open_cpp pmemkv_open_cpp/pmemkv_open.cpp)
	target_link_libraries(example-pmemkv_open_cpp pmemkv)
else()
	message(STATUS
		"Some examples use cmap engine, which is disabled, hence they are "
		"not built. If you want to build them use -DENGINE_CMAP=ON option.")
endif()

if(ENGINE_FMAP)
	add_example(pmemkv_fmap_cpp pmemkv_fmap_cpp/pmemkv_fmap.cpp)
	target_link_libraries(example-pmemkv_fmap_cpp pmemkv)

	add_example(pmemkv_fmap_benchmark_c pmemkv_fmap_benchmark_c/pmemkv_fmap_benchmark.c)
	target_link_libraries(example-pmemkv_fmap_benchmark_c pmemkv pthread)
else()
	message(STATUS
		"Some examples use fmap engine, which is disabled, hence they are "
		"not built. If you want to build them use -DENGINE_FMAP=ON option.")
endif()

if (ENGINE_CSMAP)
	add_example(pmemkv_comparator_cpp pmemkv_comparator_cpp/pmemkv_comparator.cpp)
	target_link_libraries(example-pmemkv_comparator_cpp pmemkv)

	add_example(pmemkv_comparator_c pmemkv_comparator_c/pmemkv_comparator.c)
	target_link_libraries(example-pmemkv_comparator_c pmemkv)

	include(FindThreads)
	if (CMAKE_USE_PTHREADS_INIT)
		add_example(pmemkv_iterator_cpp pmemkv_iterator_cpp/pmemkv_iterator.cpp)
		target_link_libraries(example-pmemkv_iterator_cpp pmemkv pthread)
	endif()
else()
	message(STATUS
		"Some examples use csmap engine, which is disabled, hence they are "
		"not built. If you want to build them use -DENGINE_CSMAP=ON option.")
endif()

if(ENGINE_RADIX)
	add_example(pmemkv_transaction_cpp pmemkv_transaction_cpp/pmemkv_transaction.cpp)
	target_link_libraries(example-pmemkv_transaction_cpp pmemkv ${LIBPMEMOBJ++_LIBRARIES})

	add_example(pmemkv_transaction_c pmemkv_transaction_c/pmemkv_transaction.c)
	target_link_libraries(example-pmemkv_transaction_c pmemkv ${LIBPMEMOBJ++_LIBRARIES})

	add_example(pmemkv_iterator_c pmemkv_iterator_c/pmemkv_iterator.c)
	target_link_libraries(example-pmemkv_iterator_c pmemkv)
else()
	message(STATUS
		"Some examples use radix engine, which is disabled, hence they are "
		"not built. If you want to build them use -DENGINE_RADIX=ON option.")
endif()

if(NOT ENGINE_CMAP AND NOT ENGINE_CSMAP AND NOT ENGINE_RADIX)
	message(WARNING "None of engines required for examples was enabled, "
		"so no examples were built. See messages above for instructions.")
endif()
