# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2021, Intel Corporation

#
# tests/CMakeLists.txt - prepares all the tests; it specifies which tests are enabled based on
#	options and available programs. Build tests with 'make tests' command, execute them using 'ctest'
#
include(ctest_helpers.cmake)

# ----------------------------------------------------------------- #
## Setup
# ----------------------------------------------------------------- #
add_custom_target(tests)

# Find valgrind
if(PKG_CONFIG_FOUND)
	pkg_check_modules(VALGRIND QUIET valgrind)
else()
	find_package(VALGRIND QUIET)
endif()

if(NOT VALGRIND_FOUND AND TESTS_USE_VALGRIND)
	message(FATAL_ERROR "Valgrind not found, but flag TESTS_USE_VALGRIND was set.")
elseif(NOT VALGRIND_FOUND)
	message(WARNING "Valgrind not found. Valgrind tests will not be performed.")
elseif(VALGRIND_FOUND)
	message(STATUS "Found Valgrind in '${VALGRIND_LIBRARY_DIRS}' (version: ${VALGRIND_VERSION})")
endif()

# Find libpmem & libpmemobj
if(PKG_CONFIG_FOUND)
	pkg_check_modules(LIBPMEMOBJ REQUIRED libpmemobj>=${LIBPMEMOBJ_REQUIRED_VERSION})
	pkg_check_modules(LIBPMEM REQUIRED libpmem>=${LIBPMEM_REQUIRED_VERSION})
else()
	find_package(LIBPMEMOBJ REQUIRED ${LIBPMEMOBJ_REQUIRED_VERSION})
	find_package(LIBPMEM REQUIRED ${LIBPMEM_REQUIRED_VERSION})
endif()

find_pmemcheck()
find_libunwind()
find_pmreorder()
find_pmempool()
find_gdb()

# Add checks when DEVELOPER_MODE is ON
add_cppstyle(tests ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
		${CMAKE_CURRENT_SOURCE_DIR}/c_api/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/common/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/common/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/compatibility/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/comparator/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/config/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/all/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/concurrent/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/memkind/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/persistent/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/pmemobj/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/pmemobj/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/pmreorder/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/sorted/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/sorted/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/transaction/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/result/*.c*)

add_check_whitespace(tests ${CMAKE_CURRENT_SOURCE_DIR}/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/c_api/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/cmake/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/common/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/compatibility/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/comparator/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/config/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/all/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/concurrent/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/memkind/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/persistent/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/pmemobj/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/pmreorder/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/sorted/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/transaction/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/result/*.*)

if(TESTS_JSON AND NOT BUILD_JSON_CONFIG)
	message(FATAL_ERROR "Tests requiring 'libpmemkv_json_config' library "
		"are enabled (with TESTS_JSON option), but the library is not built. "
		"If you want to run them use BUILD_JSON_CONFIG=ON option.")
elseif(NOT TESTS_JSON)
	message(WARNING
		"Most of pmemkv's tests require the 'libpmemkv_json_config' library. "
		"To enable these tests use TESTS_JSON option (and turn BUILD_JSON_CONFIG on).")
endif()

add_library(test_backtrace STATIC common/test_backtrace.c)
if(LIBUNWIND_FOUND)
	target_compile_definitions(test_backtrace PUBLIC USE_LIBUNWIND=1)
endif()

add_executable(check_is_pmem common/check_is_pmem.cpp)
target_link_libraries(check_is_pmem ${LIBPMEM_LIBRARIES})

if(COVERAGE AND VALGRIND_FOUND)
	message(STATUS "This is the Coverage build, skipping Valgrind tests")
endif()

# ----------------------------------------------------------------- #
## Common tests
# ----------------------------------------------------------------- #
build_test(wrong_engine_name_test wrong_engine_name_test.cc)
add_test_generic(NAME wrong_engine_name_test TRACERS none)

build_test(error_msg_test error_msg_test.cc)
add_test_generic(NAME error_msg_test TRACERS none)

if(BUILD_EXAMPLES AND ENGINE_CMAP)
	add_dependencies(tests example-pmemkv_basic_c
		example-pmemkv_basic_cpp
		example-pmemkv_pmemobj_cpp)
	add_test_generic(NAME example-pmemkv_basic_c TRACERS none)
	add_test_generic(NAME example-pmemkv_basic_cpp TRACERS none)
	add_test_generic(NAME example-pmemkv_pmemobj_cpp TRACERS none)

	if(BUILD_JSON_CONFIG)
		add_dependencies(tests example-pmemkv_config_c)
		add_test_generic(NAME example-pmemkv_config_c TRACERS none)
	endif()
elseif(BUILD_EXAMPLES AND NOT ENGINE_CMAP)
	message(WARNING
		"Examples use cmap engine, which is disabled, hence their execution "
		"is also disabled. If you want to run them use -DENGINE_CMAP=ON option.")
endif()

build_test(config_c config/config_c.c)
add_test_generic(NAME config_c TRACERS none memcheck)

build_test(config_cpp config/config_cpp.cc)
add_test_generic(NAME config_cpp TRACERS none memcheck)

build_test_ext(NAME json_to_config SRC_FILES config/json_to_config.cc LIBS json)
add_test_generic(NAME json_to_config TRACERS none memcheck)

# Separate binaries for config tests using deprecated functions
build_test_ext(NAME deprecated_config_c SRC_FILES config/deprecated_config.c LIBS json OPTS -Wno-deprecated-declarations)
add_test_generic(NAME deprecated_config_c TRACERS none memcheck)

build_test_ext(NAME deprecated_config_cpp SRC_FILES config/deprecated_config.cc LIBS json OPTS -Wno-deprecated-declarations)
add_test_generic(NAME deprecated_config_cpp TRACERS none memcheck)

build_test(result result/result.cpp)
add_test_generic(NAME result TRACERS none memcheck)

# ----------------------------------------------------------------- #
## Test scenarios (parametrized at least with engine name)
# ----------------------------------------------------------------- #
# Tests for all engines
build_test(open engine_scenarios/all/open.cc)
build_test_ext(NAME put_get_remove SRC_FILES engine_scenarios/all/put_get_remove.cc LIBS json)
build_test_ext(NAME put_get_remove_not_aligned SRC_FILES engine_scenarios/all/put_get_remove_not_aligned.cc LIBS json)
build_test_ext(NAME put_get_remove_charset_params SRC_FILES engine_scenarios/all/put_get_remove_charset_params.cc LIBS json)
build_test_ext(NAME put_get_remove_long_key SRC_FILES engine_scenarios/all/put_get_remove_long_key.cc LIBS json)
build_test_ext(NAME put_get_remove_params SRC_FILES engine_scenarios/all/put_get_remove_params.cc LIBS json)
build_test_ext(NAME put_get_std_map SRC_FILES engine_scenarios/all/put_get_std_map.cc LIBS json)
build_test_ext(NAME iterate SRC_FILES engine_scenarios/all/iterate.cc LIBS json)
build_test_ext(NAME error_handling_oom SRC_FILES engine_scenarios/all/error_handling_oom.cc LIBS json)

# Tests for concurrent engines
build_test_ext(NAME concurrent_iterate_params SRC_FILES engine_scenarios/concurrent/iterate_params.cc LIBS json)
build_test_ext(NAME concurrent_put_get_remove_params SRC_FILES engine_scenarios/concurrent/put_get_remove_params.cc LIBS json)
build_test_ext(NAME concurrent_put_get_remove_gen_params SRC_FILES engine_scenarios/concurrent/put_get_remove_gen_params.cc LIBS json)
build_test_ext(NAME concurrent_put_get_remove_single_op_params SRC_FILES engine_scenarios/concurrent/put_get_remove_single_op_params.cc LIBS json)
build_test_ext(NAME iterator_concurrent SRC_FILES engine_scenarios/concurrent/iterator_concurrent.cc LIBS json)

# Tests for persistent engines
build_test_ext(NAME persistent_not_found_verify SRC_FILES engine_scenarios/persistent/not_found_verify.cc LIBS json)
build_test_ext(NAME persistent_overwrite_verify SRC_FILES engine_scenarios/persistent/overwrite_verify.cc LIBS json)
build_test_ext(NAME persistent_put_remove_verify SRC_FILES engine_scenarios/persistent/put_remove_verify.cc LIBS json)
build_test_ext(NAME persistent_put_verify_asc_params SRC_FILES engine_scenarios/persistent/put_verify_asc_params.cc LIBS json)
build_test_ext(NAME persistent_put_verify_desc_params SRC_FILES engine_scenarios/persistent/put_verify_desc_params.cc LIBS json)
build_test_ext(NAME persistent_put_verify SRC_FILES engine_scenarios/persistent/put_verify.cc LIBS json)
build_test_ext(NAME persistent_put_get_std_map_multiple_reopen SRC_FILES engine_scenarios/persistent/put_get_std_map_multiple_reopen.cc LIBS json)
build_test_ext(NAME pmreorder_insert SRC_FILES engine_scenarios/pmreorder/insert.cc LIBS json)
build_test_ext(NAME pmreorder_erase SRC_FILES engine_scenarios/pmreorder/erase.cc LIBS json)
build_test_ext(NAME pmreorder_iterator SRC_FILES engine_scenarios/pmreorder/iterator.cc LIBS json)
build_test_ext(NAME pmreorder_recover SRC_FILES engine_scenarios/pmreorder/recover.cc LIBS json)

# Tests for sorted engines
build_test_ext(NAME sorted_iterate SRC_FILES engine_scenarios/sorted/iterate.cc LIBS json)
build_test_ext(NAME sorted_get_all_gen_params SRC_FILES engine_scenarios/sorted/get_all_gen_params.cc LIBS json)
build_test_ext(NAME sorted_get_above_gen_params SRC_FILES engine_scenarios/sorted/get_above_gen_params.cc LIBS json)
build_test_ext(NAME sorted_get_equal_above_gen_params SRC_FILES engine_scenarios/sorted/get_equal_above_gen_params.cc LIBS json)
build_test_ext(NAME sorted_get_below_gen_params SRC_FILES engine_scenarios/sorted/get_below_gen_params.cc LIBS json)
build_test_ext(NAME sorted_get_equal_below_gen_params SRC_FILES engine_scenarios/sorted/get_equal_below_gen_params.cc LIBS json)
build_test_ext(NAME sorted_get_between_gen_params SRC_FILES engine_scenarios/sorted/get_between_gen_params.cc LIBS json)

# Tests for pmemobj engines
build_test_ext(NAME pmemobj_error_handling_create SRC_FILES engine_scenarios/pmemobj/error_handling_create.cc LIBS json)
build_test_ext(NAME pmemobj_error_handling_defrag SRC_FILES engine_scenarios/pmemobj/error_handling_defrag.cc LIBS json)
build_test_ext(NAME pmemobj_error_handling_tx_path SRC_FILES engine_scenarios/pmemobj/error_handling_tx_path.cc LIBS json)
build_test_ext(NAME pmemobj_put_get_std_map_defrag SRC_FILES engine_scenarios/pmemobj/put_get_std_map_defrag.cc LIBS json)
build_test_ext(NAME pmemobj_error_handling_tx_oom SRC_FILES engine_scenarios/pmemobj/error_handling_tx_oom.cc engine_scenarios/pmemobj/mock_tx_alloc.cc LIBS json dl_libs)
build_test_ext(NAME pmemobj_error_handling_tx_oid SRC_FILES engine_scenarios/pmemobj/error_handling_tx_oid.cc LIBS json libpmemobj_cpp)
build_test_ext(NAME pmemobj_put_get_std_map_oid SRC_FILES engine_scenarios/pmemobj/put_get_std_map_oid.cc LIBS json libpmemobj_cpp)
build_test(pmemobj_create_or_error_if_exists engine_scenarios/pmemobj/create_or_error_if_exists.cc)

# Tests for memkind engines
build_test_ext(NAME memkind_error_handling SRC_FILES engine_scenarios/memkind/error_handling.cc LIBS json)

# Tests for C API
build_test(c_api_null_db_config c_api/null_db_config.c)

# Tests for comparator
build_test_ext(NAME comparator_basic_c SRC_FILES comparator/basic.c LIBS json)
build_test_ext(NAME comparator_basic_persistent_c SRC_FILES comparator/basic_persistent.c LIBS json)
build_test_ext(NAME comparator_custom_reopen_c SRC_FILES comparator/custom_reopen.c LIBS json)
build_test_ext(NAME comparator_default_reopen_c SRC_FILES comparator/default_reopen.c LIBS json)
build_test_ext(NAME comparator_custom_reopen_cpp SRC_FILES comparator/custom_reopen.cc LIBS json)
build_test_ext(NAME comparator_default_reopen_cpp SRC_FILES comparator/default_reopen.cc LIBS json)

# Tests for transaction
build_test_ext(NAME transaction_put SRC_FILES engine_scenarios/transaction/put.cc LIBS json)
build_test_ext(NAME transaction_remove SRC_FILES engine_scenarios/transaction/remove.cc LIBS json)
build_test_ext(NAME transaction_put_pmreorder SRC_FILES engine_scenarios/transaction/put_pmreorder.cc LIBS json)
build_test_ext(NAME transaction_not_supported SRC_FILES engine_scenarios/transaction/not_supported.cc LIBS json)

# Tests for iterator
build_test_ext(NAME iterator_basic SRC_FILES engine_scenarios/all/iterator_basic.cc LIBS json)
build_test_ext(NAME iterator_sorted SRC_FILES engine_scenarios/sorted/iterator_sorted.cc LIBS json)
build_test_ext(NAME iterator_not_supported SRC_FILES engine_scenarios/all/iterator_not_supported.cc LIBS json)

###################################### BLACKHOLE ##############################
build_test(blackhole_test engines/blackhole/blackhole_test.cc)
add_test_generic(NAME blackhole_test TRACERS none memcheck)
#
add_engine_test(ENGINE blackhole
		BINARY transaction_not_supported
		TRACERS none memcheck
		SCRIPT blackhole/default.cmake)
################################################################################
##################################### CMAP ####################################
if(ENGINE_CMAP)
	add_engine_test(ENGINE cmap
			BINARY c_api_null_db_config
			TRACERS none memcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE cmap
			BINARY open
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default_no_config.cmake)

	add_engine_test(ENGINE cmap
			BINARY put_get_remove
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE cmap
			BINARY put_get_remove_not_aligned
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)


	add_engine_test(ENGINE cmap
			BINARY put_get_remove_charset_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 16 8)

	add_engine_test(ENGINE cmap
			BINARY put_get_remove_long_key
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE cmap
			BINARY put_get_remove_params
			TRACERS none
			SCRIPT pmemobj_based/default.cmake
			DB_SIZE 4G PARAMS 400000)

	add_engine_test(ENGINE cmap
			BINARY put_get_remove_params
			TRACERS memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			DB_SIZE 1G PARAMS 4000)

	add_engine_test(ENGINE cmap
			BINARY put_get_std_map
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 100 200)

	# XXX: https://github.com/pmem/libpmemobj-cpp/issues/516
	# add_engine_test(ENGINE cmap
	# BINARY error_handling_oom
	# TRACERS none
	# SCRIPT pmemobj_based/default.cmake
	# DB_SIZE 50M)

	add_engine_test(ENGINE cmap
			BINARY iterate
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE cmap
			BINARY concurrent_put_get_remove_params
			TRACERS none memcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 8 50)

	if(TESTS_PMEMOBJ_DRD_HELGRIND)
		add_engine_test(ENGINE cmap
				BINARY concurrent_put_get_remove_params
				TRACERS drd helgrind
				SCRIPT pmemobj_based/default.cmake
				PARAMS 8 50)
	endif()

	add_engine_test(ENGINE cmap
			BINARY concurrent_put_get_remove_gen_params
			TRACERS none memcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 8 50 100)

	if(TESTS_PMEMOBJ_DRD_HELGRIND)
		add_engine_test(ENGINE cmap
				BINARY concurrent_put_get_remove_gen_params
				TRACERS drd helgrind
				SCRIPT pmemobj_based/default.cmake
				PARAMS 8 50 100)
	endif()

	add_engine_test(ENGINE cmap
			BINARY concurrent_put_get_remove_single_op_params
			TRACERS none
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000)

	add_engine_test(ENGINE cmap
			BINARY concurrent_put_get_remove_single_op_params
			TRACERS memcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 400)

	if(TESTS_PMEMOBJ_DRD_HELGRIND)
		add_engine_test(ENGINE cmap
				BINARY concurrent_put_get_remove_single_op_params
				TRACERS drd helgrind
				SCRIPT pmemobj_based/default.cmake
				PARAMS 250)
	endif()

	add_engine_test(ENGINE cmap
			BINARY persistent_put_get_std_map_multiple_reopen
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE cmap
			BINARY persistent_not_found_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE cmap
			BINARY persistent_overwrite_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE cmap
			BINARY persistent_put_remove_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE cmap
			BINARY persistent_put_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE cmap
			BINARY persistent_put_verify_asc_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 4000)

	add_engine_test(ENGINE cmap
			BINARY persistent_put_verify_desc_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 4000)

	add_engine_test(ENGINE cmap
			BINARY pmemobj_error_handling_tx_oom
			TRACERS none
			SCRIPT pmemobj_based/default.cmake
			DB_SIZE 200M)

	if(TESTS_LONG)
		add_engine_test(ENGINE cmap
				BINARY pmemobj_error_handling_tx_oom
				TRACERS memcheck
				SCRIPT pmemobj_based/default.cmake
				DB_SIZE 200M)
	endif()

	add_engine_test(ENGINE cmap
			BINARY pmemobj_error_handling_create
			TRACERS none memcheck
			SCRIPT pmemobj_based/pmemobj/error_handling_create.cmake)

	add_engine_test(ENGINE cmap
			BINARY pmemobj_error_handling_tx_oid
			TRACERS none memcheck
			SCRIPT pmemobj_based/pmemobj/error_handling_tx_oid.cmake)

	add_engine_test(ENGINE cmap
			BINARY pmemobj_error_handling_tx_path
			TRACERS none memcheck
			SCRIPT pmemobj_based/pmemobj/error_handling_tx_path.cmake)

	add_engine_test(ENGINE cmap
			BINARY pmemobj_error_handling_defrag
			TRACERS none memcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE cmap
			BINARY pmemobj_create_or_error_if_exists
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default_no_config.cmake)

	add_engine_test(ENGINE cmap
			BINARY pmemobj_put_get_std_map_defrag
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE cmap
			BINARY pmemobj_put_get_std_map_oid
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/put_get_std_map_oid.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE cmap
			BINARY put_get_std_map
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/put_get_std_map_create_or_error_if_exists.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE cmap
			BINARY put_get_std_map
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/create_if_missing.cmake
			PARAMS 128 32 16)

	add_engine_test(ENGINE cmap
			BINARY iterator_basic
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE cmap
			BINARY iterator_concurrent
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 8)

	add_engine_test(ENGINE cmap
			BINARY transaction_not_supported
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	if(TESTS_PMEMOBJ_DRD_HELGRIND)
		add_engine_test(ENGINE cmap
				BINARY iterator_concurrent
				TRACERS helgrind drd
				SCRIPT pmemobj_based/default.cmake
				PARAMS 8)
	endif()
endif(ENGINE_CMAP)
################################################################################
###################################### CSMAP ###################################
if(ENGINE_CSMAP)
	add_engine_test(ENGINE csmap
			BINARY c_api_null_db_config
			TRACERS none memcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE csmap
			BINARY open
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default_no_config.cmake)

	add_engine_test(ENGINE csmap
			BINARY comparator_basic_c
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE csmap
			BINARY comparator_basic_persistent_c
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE csmap
			BINARY comparator_custom_reopen_c
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE csmap
			BINARY comparator_default_reopen_c
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE csmap
			BINARY comparator_custom_reopen_cpp
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE csmap
			BINARY comparator_default_reopen_cpp
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE csmap
			BINARY put_get_remove
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE csmap
			BINARY put_get_remove_not_aligned
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE csmap
			BINARY put_get_remove_charset_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 16 8)

	add_engine_test(ENGINE csmap
			BINARY put_get_remove_long_key
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE csmap
			BINARY put_get_remove_params
			TRACERS none
			SCRIPT pmemobj_based/default.cmake
			DB_SIZE 4G PARAMS 400000)

	add_engine_test(ENGINE csmap
			BINARY put_get_remove_params
			TRACERS memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			DB_SIZE 1G PARAMS 4000)

	add_engine_test(ENGINE csmap
			BINARY put_get_std_map
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 100 200)

	# XXX: investigate failure (possibly https://github.com/pmem/libpmemobj-cpp/issues/516)
	# add_engine_test(ENGINE csmap
	# BINARY error_handling_oom
	# TRACERS none
	# SCRIPT pmemobj_based/default.cmake
	# DB_SIZE 50M)

	add_engine_test(ENGINE csmap
			BINARY iterate
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE csmap
			BINARY sorted_iterate
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE csmap
			BINARY sorted_get_all_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 32 8)

	add_engine_test(ENGINE csmap
			BINARY sorted_get_above_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS default 32 8)

	add_engine_test(ENGINE csmap
			BINARY sorted_get_above_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS reverse 32 8)

	add_engine_test(ENGINE csmap
			BINARY sorted_get_equal_above_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 32 8)

	add_engine_test(ENGINE csmap
			BINARY sorted_get_below_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 32 8)

	add_engine_test(ENGINE csmap
			BINARY sorted_get_equal_below_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 32 8)

	add_engine_test(ENGINE csmap
			BINARY sorted_get_between_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 32 8)

	add_engine_test(ENGINE csmap
			BINARY concurrent_iterate_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 24 200)

	if(TESTS_PMEMOBJ_DRD_HELGRIND)
		add_engine_test(ENGINE csmap
				BINARY concurrent_iterate_params
				TRACERS drd # helgrind XXX: skip list lock order error
				SCRIPT pmemobj_based/default.cmake
				PARAMS 4 50)
	endif()

	add_engine_test(ENGINE csmap
			BINARY concurrent_put_get_remove_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 8 50)

	if(TESTS_PMEMOBJ_DRD_HELGRIND AND TESTS_LONG)
		add_engine_test(ENGINE csmap
				BINARY concurrent_put_get_remove_params
				TRACERS drd # helgrind XXX: skip list lock order error
				SCRIPT pmemobj_based/default.cmake
				PARAMS 8 10)
		if(TESTS_LONG)
			add_engine_test(ENGINE csmap
					BINARY concurrent_put_get_remove_params
					TRACERS drd # helgrind XXX: skip list lock order error
					SCRIPT pmemobj_based/default.cmake
					PARAMS 8 50)
		endif()
	endif()

	add_engine_test(ENGINE csmap
			BINARY concurrent_put_get_remove_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 8 50 100)

	if(TESTS_PMEMOBJ_DRD_HELGRIND)
		add_engine_test(ENGINE csmap
				BINARY concurrent_put_get_remove_gen_params
				TRACERS drd # helgrind XXX: skip list lock order error
				SCRIPT pmemobj_based/default.cmake
				PARAMS 8 50 100)
	endif()

	add_engine_test(ENGINE csmap
			BINARY concurrent_put_get_remove_single_op_params
			TRACERS none
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000)

	add_engine_test(ENGINE csmap
			BINARY concurrent_put_get_remove_single_op_params
			TRACERS memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 400)

	if(TESTS_PMEMOBJ_DRD_HELGRIND)
		add_engine_test(ENGINE csmap
				BINARY concurrent_put_get_remove_single_op_params
				TRACERS drd # helgrind XXX: skip list lock order error
				SCRIPT pmemobj_based/default.cmake
				PARAMS 100)

		add_engine_test(ENGINE csmap
				BINARY iterator_concurrent
				TRACERS drd # helgrind XXX: https://github.com/pmem/libpmemobj-cpp/issues/797
				SCRIPT pmemobj_based/default.cmake
				PARAMS 8)

		if(TESTS_LONG)
			add_engine_test(ENGINE csmap
					BINARY concurrent_put_get_remove_single_op_params
					TRACERS drd # helgrind XXX: skip list lock order error
					SCRIPT pmemobj_based/default.cmake
					PARAMS 400)
		endif()
	endif()

	add_engine_test(ENGINE csmap
			BINARY persistent_put_get_std_map_multiple_reopen
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE csmap
			BINARY persistent_not_found_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE csmap
			BINARY persistent_overwrite_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE csmap
			BINARY persistent_put_remove_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE csmap
			BINARY persistent_put_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE csmap
			BINARY persistent_put_verify_asc_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 4000)

	add_engine_test(ENGINE csmap
			BINARY persistent_put_verify_desc_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 4000)

	add_engine_test(ENGINE csmap
			BINARY pmemobj_error_handling_tx_oom
			TRACERS none
			SCRIPT pmemobj_based/default.cmake
			DB_SIZE 200M)

	if(TESTS_LONG)
		add_engine_test(ENGINE csmap
				BINARY pmemobj_error_handling_tx_oom
				TRACERS memcheck
				SCRIPT pmemobj_based/default.cmake
				DB_SIZE 200M)
	endif()

	add_engine_test(ENGINE csmap
			BINARY pmemobj_error_handling_create
			TRACERS none memcheck
			SCRIPT pmemobj_based/pmemobj/error_handling_create.cmake)

	add_engine_test(ENGINE csmap
			BINARY pmemobj_error_handling_tx_oid
			TRACERS none memcheck
			SCRIPT pmemobj_based/pmemobj/error_handling_tx_oid.cmake)

	add_engine_test(ENGINE csmap
			BINARY pmemobj_error_handling_tx_path
			TRACERS none memcheck
			SCRIPT pmemobj_based/pmemobj/error_handling_tx_path.cmake)

	add_engine_test(ENGINE csmap
			BINARY pmemobj_create_or_error_if_exists
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default_no_config.cmake)

	# XXX - CSMAP does not support defrag yet
	# add_engine_test(ENGINE csmap
	# BINARY pmemobj_error_handling_defrag
	# TRACERS none memcheck
	# SCRIPT pmemobj_based/default.cmake)

	# XXX - CSMAP does not support defrag yet
	# add_engine_test(ENGINE csmap
	# BINARY pmemobj_put_get_std_map_defrag
	# TRACERS none memcheck pmemcheck
	# SCRIPT pmemobj_based/default.cmake
	# PARAMS 1000 100 200)

	add_engine_test(ENGINE csmap
			BINARY pmemobj_put_get_std_map_oid
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/put_get_std_map_oid.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE csmap
			BINARY put_get_std_map
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/put_get_std_map_create_or_error_if_exists.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE csmap
			BINARY put_get_std_map
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/create_if_missing.cmake
			PARAMS 128 32 16)

	add_engine_test(ENGINE csmap
			BINARY iterator_basic
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE csmap
			BINARY iterator_sorted
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS false)

	add_engine_test(ENGINE csmap
			BINARY iterator_concurrent
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 8 true)

	add_engine_test(ENGINE csmap
			BINARY transaction_not_supported
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)
endif(ENGINE_CSMAP)
################################################################################
###################################### VCMAP ###################################
if(ENGINE_VCMAP)
	add_engine_test(ENGINE vcmap
			BINARY c_api_null_db_config
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vcmap
			BINARY open
			TRACERS none memcheck
			SCRIPT memkind_based/default_no_config.cmake)

	add_engine_test(ENGINE vcmap
			BINARY put_get_remove
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vcmap
			BINARY put_get_remove_not_aligned
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vcmap
			BINARY put_get_remove_charset_params
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake
			PARAMS 16 8)

	add_engine_test(ENGINE vcmap
			BINARY put_get_remove_long_key
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vcmap
			BINARY put_get_remove_params
			TRACERS none
			SCRIPT memkind_based/default.cmake
			DB_SIZE 4294967296 PARAMS 400000)

	add_engine_test(ENGINE vcmap
			BINARY put_get_remove_params
			TRACERS memcheck
			SCRIPT memkind_based/default.cmake
			DB_SIZE 4294967296 PARAMS 4000)

	add_engine_test(ENGINE vcmap
			BINARY put_get_std_map
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake
			PARAMS 1000 100 200)

	# XXX: https://github.com/pmem/pmemkv/issues/623
	# add_engine_test(ENGINE vcmap
	# BINARY error_handling_oom
	# TRACERS none memcheck
	# SCRIPT memkind_based/default.cmake
	# DB_SIZE 50485760)

	add_engine_test(ENGINE vcmap
			BINARY iterate
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vcmap
			BINARY concurrent_put_get_remove_params
			TRACERS none memcheck # XXX - tbb lock does not work well with drd or helgrind
			SCRIPT memkind_based/default.cmake
			PARAMS 8 50)

	add_engine_test(ENGINE vcmap
			BINARY concurrent_put_get_remove_gen_params
			TRACERS none memcheck # XXX - tbb lock does not work well with drd or helgrind
			SCRIPT memkind_based/default.cmake
			PARAMS 8 50 100)

	add_engine_test(ENGINE vcmap
			BINARY concurrent_put_get_remove_single_op_params
			TRACERS none # XXX - tbb lock does not work well with drd or helgrind
			SCRIPT memkind_based/default.cmake
			DB_SIZE "MIN_JEMALLOC_ARENA_SIZE" PARAMS 1000)

	add_engine_test(ENGINE vcmap
			BINARY concurrent_put_get_remove_single_op_params
			TRACERS memcheck # XXX - tbb lock does not work well with drd or helgrind
			SCRIPT memkind_based/default.cmake
			DB_SIZE "MIN_JEMALLOC_ARENA_SIZE" PARAMS 400)

	add_engine_test(ENGINE vcmap
			BINARY memkind_error_handling
			TRACERS none memcheck
			SCRIPT memkind_based/memkind/error_handling.cmake)

	add_engine_test(ENGINE vcmap
			BINARY iterator_basic
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vcmap
			BINARY iterator_concurrent
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake
			PARAMS 8)

	add_engine_test(ENGINE vcmap
			BINARY transaction_not_supported
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)
endif(ENGINE_VCMAP)
################################################################################
###################################### VSMAP ###################################
if(ENGINE_VSMAP)
	add_engine_test(ENGINE vsmap
			BINARY c_api_null_db_config
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vsmap
			BINARY open
			TRACERS none memcheck
			SCRIPT memkind_based/default_no_config.cmake)

	add_engine_test(ENGINE vsmap
			BINARY comparator_basic_c
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vsmap
			BINARY put_get_remove
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vsmap
			BINARY put_get_remove_not_aligned
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vsmap
			BINARY put_get_remove_charset_params
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake
			PARAMS 16 8)

	add_engine_test(ENGINE vsmap
			BINARY put_get_remove_long_key
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vsmap
			BINARY put_get_remove_params
			TRACERS none
			SCRIPT memkind_based/default.cmake
			DB_SIZE 4294967296 PARAMS 400000)

	add_engine_test(ENGINE vsmap
			BINARY put_get_remove_params
			TRACERS memcheck
			SCRIPT memkind_based/default.cmake
			DB_SIZE 4294967296 PARAMS 4000)

	add_engine_test(ENGINE vsmap
			BINARY put_get_std_map
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE vsmap
			BINARY error_handling_oom
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake
			DB_SIZE 50485760)

	add_engine_test(ENGINE vsmap
			BINARY iterate
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vsmap
			BINARY sorted_iterate
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vsmap
			BINARY sorted_get_all_gen_params
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake
			PARAMS 32 8)

	add_engine_test(ENGINE vsmap
			BINARY sorted_get_above_gen_params
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake
			PARAMS default 32 8)

	add_engine_test(ENGINE vsmap
			BINARY sorted_get_above_gen_params
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake
			PARAMS reverse 32 8)

	add_engine_test(ENGINE vsmap
			BINARY sorted_get_equal_above_gen_params
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake
			PARAMS 32 8)

	add_engine_test(ENGINE vsmap
			BINARY sorted_get_below_gen_params
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake
			PARAMS 32 8)

	add_engine_test(ENGINE vsmap
			BINARY sorted_get_equal_below_gen_params
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake
			PARAMS 32 8)

	add_engine_test(ENGINE vsmap
			BINARY sorted_get_between_gen_params
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake
			PARAMS 32 8)

	add_engine_test(ENGINE vsmap
			BINARY memkind_error_handling
			TRACERS none memcheck
			SCRIPT memkind_based/memkind/error_handling.cmake)

	add_engine_test(ENGINE vsmap
			BINARY iterator_basic
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vsmap
			BINARY iterator_sorted
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vsmap
			BINARY transaction_not_supported
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)
endif(ENGINE_VSMAP)
################################################################################
###################################### TREE3 ###################################
if(ENGINE_TREE3)
	# XXX - all memcheck and pmemcheck tests are disabled due to failures
	# Need to investigate

	add_engine_test(ENGINE tree3
			BINARY c_api_null_db_config
			TRACERS none #memcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE tree3
			BINARY open
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/default_no_config.cmake)

	# XXX - add comparator support to tree3
	# add_engine_test(ENGINE tree3
	# BINARY c_api_comparator
	# TRACERS none memcheck
	# SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE tree3
			BINARY put_get_remove
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE tree3
			BINARY put_get_remove_not_aligned
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE tree3
			BINARY put_get_remove_charset_params
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 16 8)

	add_engine_test(ENGINE tree3
			BINARY put_get_remove_long_key
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE tree3
			BINARY put_get_remove_params
			TRACERS none #memcheck
			SCRIPT pmemobj_based/default.cmake
			DB_SIZE 4G PARAMS 400000)

	add_engine_test(ENGINE tree3
			BINARY put_get_std_map
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE tree3
			BINARY error_handling_oom
			TRACERS none #memcheck
			SCRIPT pmemobj_based/default.cmake
			DB_SIZE 20M)

	# XXX - inverstigate failure
	# add_engine_test(ENGINE tree3
	# BINARY iterate
	# TRACERS none memcheck pmemcheck
	# SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE tree3
			BINARY persistent_put_get_std_map_multiple_reopen
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE tree3
			BINARY persistent_not_found_verify
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE tree3
			BINARY persistent_overwrite_verify
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE tree3
			BINARY persistent_put_remove_verify
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE tree3
			BINARY persistent_put_verify
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE tree3
			BINARY persistent_put_verify_asc_params
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 6000)

	add_engine_test(ENGINE tree3
			BINARY persistent_put_verify_asc_params
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 144) # 144 is limit for one inner node

	add_engine_test(ENGINE tree3
			BINARY persistent_put_verify_desc_params
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 6000)

	add_engine_test(ENGINE tree3
			BINARY persistent_put_verify_desc_params
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 144) # 144 is limit for one inner node

	add_engine_test(ENGINE tree3
			BINARY pmemobj_error_handling_tx_oom
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE tree3
			BINARY pmemobj_error_handling_create
			TRACERS none #memcheck
			SCRIPT pmemobj_based/pmemobj/error_handling_create.cmake)

	add_engine_test(ENGINE tree3
			BINARY pmemobj_create_or_error_if_exists
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default_no_config.cmake)

	# XXX - defrag not supported
	# add_engine_test(ENGINE tree3
	# BINARY pmemobj_error_handling_tx_oid
	# TRACERS none memcheck
	# SCRIPT pmemobj_based/pmemobj/error_handling_tx_oid.cmake)

	# XXX - defrag not supported
	# add_engine_test(ENGINE tree3
	# BINARY pmemobj_error_handling_tx_path
	# TRACERS none memcheck
	# SCRIPT pmemobj_based/pmemobj/error_handling_tx_path.cmake)

	# XXX - defrag not supported
	# add_engine_test(ENGINE tree3
	# BINARY pmemobj_error_handling_defrag
	# TRACERS none memcheck
	# SCRIPT pmemobj_based/default.cmake)

	# XXX - defrag not supported
	# add_engine_test(ENGINE tree3
	# BINARY pmemobj_put_get_std_map_defrag
	# TRACERS none memcheck pmemcheck
	# SCRIPT pmemobj_based/default.cmake
	# PARAMS 1000 100 200)

	add_engine_test(ENGINE tree3
			BINARY pmemobj_put_get_std_map_oid
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/put_get_std_map_oid.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE tree3
			BINARY put_get_std_map
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/put_get_std_map_create_or_error_if_exists.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE tree3
			BINARY put_get_std_map
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/create_if_missing.cmake
			PARAMS 128 32 16)

	# XXX fail to investigate
	# add_engine_test(ENGINE tree3
	# BINARY sorted_iterate
	# TRACERS none memcheck pmemcheck
	# SCRIPT pmemobj_based/default.cmake)

	# XXX to be tested
	# add_engine_test(ENGINE tree3
	# BINARY sorted_get_all_gen_params
	# TRACERS none memcheck pmemcheck
	# SCRIPT pmemobj_based/default.cmake
	# PARAMS 32 8)

	# XXX to be tested
	# add_engine_test(ENGINE tree3
	# BINARY sorted_get_above_gen_params
	# TRACERS none memcheck pmemcheck
	# SCRIPT pmemobj_based/default.cmake
	# PARAMS 32 8)

	# XXX to be tested
	# add_engine_test(ENGINE tree3
	# BINARY sorted_get_equal_above_gen_params
	# TRACERS none memcheck pmemcheck
	# SCRIPT pmemobj_based/default.cmake
	# PARAMS 32 8)

	# XXX to be tested
	# add_engine_test(ENGINE tree3
	# BINARY sorted_get_below_gen_params
	# TRACERS none memcheck pmemcheck
	# SCRIPT pmemobj_based/default.cmake
	# PARAMS 32 8)

	# XXX to be tested
	# add_engine_test(ENGINE tree3
	# BINARY sorted_get_equal_below_gen_params
	# TRACERS none memcheck pmemcheck
	# SCRIPT pmemobj_based/default.cmake
	# PARAMS 32 8)

	# XXX to be tested
	# add_engine_test(ENGINE tree3
	# BINARY sorted_get_between_gen_params
	# TRACERS none memcheck pmemcheck
	# SCRIPT pmemobj_based/default.cmake
	# PARAMS 32 8)

	add_engine_test(ENGINE tree3
			BINARY transaction_not_supported
			TRACERS none memcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE tree3
			BINARY iterator_not_supported
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)
endif(ENGINE_TREE3)
################################################################################
###################################### STREE ###################################
if(ENGINE_STREE)
	add_engine_test(ENGINE stree
			BINARY c_api_null_db_config
			TRACERS none memcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE stree
			BINARY open
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default_no_config.cmake)

	add_engine_test(ENGINE stree
			BINARY comparator_basic_c
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE stree
			BINARY comparator_basic_persistent_c
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE stree
			BINARY comparator_custom_reopen_c
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE stree
			BINARY comparator_default_reopen_c
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE stree
			BINARY comparator_custom_reopen_cpp
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE stree
			BINARY comparator_default_reopen_cpp
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE stree
			BINARY put_get_remove
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE stree
			BINARY put_get_remove_not_aligned
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE stree
			BINARY put_get_remove_charset_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 16 8)

	add_engine_test(ENGINE stree
			BINARY put_get_remove_long_key
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE stree
				BINARY put_get_remove_params
				TRACERS none memcheck pmemcheck
				SCRIPT pmemobj_based/default.cmake
				DB_SIZE 1G PARAMS 10000)

	if (TESTS_LONG)
		add_engine_test(ENGINE stree
				BINARY put_get_remove_params
				TRACERS none memcheck pmemcheck
				SCRIPT pmemobj_based/default.cmake
				DB_SIZE 4G PARAMS 400000)
	endif()

	if(PMREORDER_SUPPORTED)
		add_engine_test(ENGINE stree
				BINARY pmreorder_insert
				TRACERS none
				SCRIPT pmemobj_based/pmreorder/insert.cmake)

		add_engine_test(ENGINE stree
				BINARY pmreorder_erase
				TRACERS none
				SCRIPT pmemobj_based/pmreorder/erase.cmake)

		add_engine_test(ENGINE stree
				BINARY pmreorder_iterator
				TRACERS none
				SCRIPT pmemobj_based/pmreorder/iterator.cmake)

		add_engine_test(ENGINE stree
				BINARY pmreorder_recover
				TRACERS none
				SCRIPT pmemobj_based/pmreorder/recover.cmake)
	endif()

	add_engine_test(ENGINE stree
			BINARY put_get_std_map
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 20 200)

	# XXX: investigate failure (possibly https://github.com/pmem/libpmemobj-cpp/issues/516)
	# add_engine_test(ENGINE stree
	# BINARY error_handling_oom
	# TRACERS none memcheck
	# SCRIPT pmemobj_based/default.cmake
	# DB_SIZE 20M)

	add_engine_test(ENGINE stree
			BINARY iterate
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE stree
			BINARY persistent_put_get_std_map_multiple_reopen
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 20 200)

	add_engine_test(ENGINE stree
			BINARY persistent_not_found_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE stree
			BINARY persistent_overwrite_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE stree
			BINARY persistent_put_remove_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE stree
			BINARY persistent_put_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE stree
			BINARY persistent_put_verify_asc_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 4000)

	add_engine_test(ENGINE stree
			BINARY persistent_put_verify_desc_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 4000)

	add_engine_test(ENGINE stree
			BINARY pmemobj_error_handling_create
			TRACERS none memcheck
			SCRIPT pmemobj_based/pmemobj/error_handling_create.cmake)

	add_engine_test(ENGINE stree
			BINARY pmemobj_create_or_error_if_exists
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default_no_config.cmake)

	# XXX - defrag not supported
	# add_engine_test(ENGINE stree
	# BINARY pmemobj_error_handling_tx_oid
	# TRACERS none memcheck
	# SCRIPT pmemobj_based/pmemobj/error_handling_tx_oid.cmake)

	# XXX - defrag not supported
	# add_engine_test(ENGINE stree
	# BINARY pmemobj_error_handling_tx_path
	# TRACERS none memcheck
	# SCRIPT pmemobj_based/pmemobj/error_handling_tx_path.cmake)

	# XXX - defrag not supported
	# add_engine_test(ENGINE stree
	# BINARY pmemobj_error_handling_defrag
	# TRACERS none memcheck
	# SCRIPT pmemobj_based/default.cmake)

	# XXX - defrag not supported
	# add_engine_test(ENGINE stree
	# BINARY pmemobj_put_get_std_map_defrag
	# TRACERS none memcheck pmemcheck
	# SCRIPT pmemobj_based/default.cmake
	# PARAMS 1000 100 200)

	# XXX: investigate failure (possibly https://github.com/pmem/libpmemobj-cpp/issues/516)
	# add_engine_test(ENGINE stree
	# BINARY pmemobj_error_handling_tx_oom
	# TRACERS none
	# SCRIPT pmemobj_based/default.cmake
	# DB_SIZE 200M)
	# if(TESTS_LONG)
	# add_engine_test(ENGINE stree
	# BINARY pmemobj_error_handling_tx_oom
	# TRACERS memcheck
	# SCRIPT pmemobj_based/default.cmake
	# DB_SIZE 200M)
	# endif()

	add_engine_test(ENGINE stree
			BINARY pmemobj_put_get_std_map_oid
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/put_get_std_map_oid.cmake
			PARAMS 1000 20 200)

	add_engine_test(ENGINE stree
			BINARY put_get_std_map
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/put_get_std_map_create_or_error_if_exists.cmake
			PARAMS 1000 20 200)

	add_engine_test(ENGINE stree
			BINARY put_get_std_map
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/create_if_missing.cmake
			PARAMS 128 32 16)

	add_engine_test(ENGINE stree
			BINARY sorted_iterate
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE stree
			BINARY sorted_get_all_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 32 8)

	add_engine_test(ENGINE stree
			BINARY sorted_get_above_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS default 32 8)

	add_engine_test(ENGINE stree
			BINARY sorted_get_above_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS reverse 32 8)

	add_engine_test(ENGINE stree
			BINARY sorted_get_equal_above_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 32 8)

	add_engine_test(ENGINE stree
			BINARY sorted_get_below_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 32 8)

	add_engine_test(ENGINE stree
			BINARY sorted_get_equal_below_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 32 8)

	add_engine_test(ENGINE stree
			BINARY sorted_get_between_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 32 8)

	add_engine_test(ENGINE stree
			BINARY iterator_basic
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE stree
			BINARY iterator_sorted
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE stree
		BINARY transaction_not_supported
		TRACERS none memcheck pmemcheck
		SCRIPT pmemobj_based/default.cmake)
endif(ENGINE_STREE)
################################################################################
###################################### RADIX ###################################
if(ENGINE_RADIX)
	add_engine_test(ENGINE radix
			BINARY c_api_null_db_config
			TRACERS none memcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE radix
			BINARY open
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default_no_config.cmake)

	add_engine_test(ENGINE radix
			BINARY put_get_remove
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE radix
			BINARY put_get_remove_not_aligned
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE radix
			BINARY put_get_std_map
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 8 200)

	add_engine_test(ENGINE radix
			BINARY error_handling_oom
			TRACERS none memcheck
			SCRIPT pmemobj_based/default.cmake
			DB_SIZE 20M)

	add_engine_test(ENGINE radix
			BINARY iterate
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE radix
			BINARY persistent_put_get_std_map_multiple_reopen
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 8 200)

	add_engine_test(ENGINE radix
			BINARY persistent_not_found_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE radix
			BINARY persistent_overwrite_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE radix
			BINARY persistent_put_remove_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE radix
			BINARY persistent_put_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE radix
			BINARY persistent_put_verify_asc_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 6000)

	add_engine_test(ENGINE radix
			BINARY persistent_put_verify_desc_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 6000)

	add_engine_test(ENGINE radix
			BINARY pmemobj_error_handling_tx_oom
			TRACERS none
			SCRIPT pmemobj_based/default.cmake)

	if(TESTS_LONG)
		add_engine_test(ENGINE radix
				BINARY pmemobj_error_handling_tx_oom
				TRACERS memcheck
				SCRIPT pmemobj_based/default.cmake
				DB_SIZE 200M)
	endif()

	add_engine_test(ENGINE radix
			BINARY pmemobj_error_handling_create
			TRACERS none memcheck
			SCRIPT pmemobj_based/pmemobj/error_handling_create.cmake)

	add_engine_test(ENGINE radix
			BINARY pmemobj_error_handling_tx_oid
			TRACERS none memcheck
			SCRIPT pmemobj_based/pmemobj/error_handling_tx_oid.cmake)

	add_engine_test(ENGINE radix
			BINARY pmemobj_error_handling_tx_path
			TRACERS none memcheck
			SCRIPT pmemobj_based/pmemobj/error_handling_tx_path.cmake)

	add_engine_test(ENGINE radix
			BINARY pmemobj_create_or_error_if_exists
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default_no_config.cmake)

	# XXX - defrag not supported
	# add_engine_test(ENGINE radix
	# BINARY pmemobj_error_handling_defrag
	# TRACERS none memcheck
	# SCRIPT pmemobj_based/default.cmake)

	# XXX - defrag not supported
	# add_engine_test(ENGINE radix
	# BINARY pmemobj_put_get_std_map_defrag
	# TRACERS none memcheck pmemcheck
	# SCRIPT pmemobj_based/default.cmake
	# PARAMS 1000 100 200)

	add_engine_test(ENGINE radix
			BINARY pmemobj_put_get_std_map_oid
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/put_get_std_map_oid.cmake
			PARAMS 1000 8 200)

	add_engine_test(ENGINE radix
			BINARY put_get_std_map
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/put_get_std_map_create_or_error_if_exists.cmake
			PARAMS 1000 8 200)

	add_engine_test(ENGINE radix
			BINARY put_get_std_map
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/create_if_missing.cmake
			PARAMS 128 32 16)

	add_engine_test(ENGINE radix
			BINARY put_get_remove_charset_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 16)

	add_engine_test(ENGINE radix
			BINARY put_get_remove_charset_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 1024)

	add_engine_test(ENGINE radix
			BINARY put_get_remove_long_key
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE radix
			BINARY put_get_remove_params
			TRACERS none
			SCRIPT pmemobj_based/default.cmake
			DB_SIZE 1G PARAMS 400000)

	add_engine_test(ENGINE radix
			BINARY put_get_remove_params
			TRACERS memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			DB_SIZE 1G PARAMS 4000)

	add_engine_test(ENGINE radix
			BINARY put_get_std_map
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE radix
			BINARY sorted_iterate
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE radix
			BINARY sorted_get_all_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 32 8)

	add_engine_test(ENGINE radix
			BINARY sorted_get_above_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS default 32 8)

	add_engine_test(ENGINE radix
			BINARY sorted_get_equal_above_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 32 8)

	add_engine_test(ENGINE radix
			BINARY sorted_get_below_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 32 8)

	add_engine_test(ENGINE radix
			BINARY sorted_get_equal_below_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 32 8)

	add_engine_test(ENGINE radix
			BINARY sorted_get_between_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 32 8)

	add_engine_test(ENGINE radix
			BINARY transaction_put
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE radix
			BINARY transaction_remove
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE radix
			BINARY iterator_basic
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE radix
			BINARY iterator_sorted
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	if(PMREORDER_SUPPORTED)
		add_engine_test(ENGINE radix
				BINARY transaction_put_pmreorder
				TRACERS none
				SCRIPT pmemobj_based/pmreorder/insert.cmake)

		add_engine_test(ENGINE radix
				BINARY pmreorder_insert
				TRACERS none
				SCRIPT pmemobj_based/pmreorder/insert.cmake)

		add_engine_test(ENGINE radix
				BINARY pmreorder_erase
				TRACERS none
				SCRIPT pmemobj_based/pmreorder/erase.cmake)

		add_engine_test(ENGINE radix
				BINARY pmreorder_iterator
				TRACERS none
				SCRIPT pmemobj_based/pmreorder/iterator.cmake)

		add_engine_test(ENGINE radix
				BINARY pmreorder_recover
				TRACERS none
				SCRIPT pmemobj_based/pmreorder/recover.cmake)
	endif()
endif(ENGINE_RADIX)
################################################################################
#################################### ROBINHOOD #################################
if (ENGINE_ROBINHOOD)
	# XXX: https://github.com/pmem/pmemkv/issues/916
	# add_engine_test(ENGINE robinhood
	#	BINARY error_handling_oom
	#	TRACERS none memcheck
	#	SCRIPT pmemobj_based/default.cmake
	#	DB_SIZE 20M)

	add_engine_test(ENGINE robinhood
			BINARY iterate
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE robinhood
			BINARY iterator_not_supported
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE robinhood
			BINARY open
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default_no_config.cmake)

	add_engine_test(ENGINE robinhood
			BINARY put_get_remove_charset_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 16 8)

	add_engine_test(ENGINE robinhood
			BINARY put_get_remove_params
			TRACERS none
			SCRIPT pmemobj_based/default.cmake
			DB_SIZE 1G PARAMS 40000)

	add_engine_test(ENGINE robinhood
			BINARY put_get_remove_params
			TRACERS memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			DB_SIZE 1G PARAMS 4000)

	add_engine_test(ENGINE robinhood
			BINARY put_get_remove
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE robinhood
			BINARY put_get_std_map
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 8 8)

	add_engine_test(ENGINE robinhood
			BINARY concurrent_put_get_remove_single_op_params
			TRACERS none
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000)

	add_engine_test(ENGINE robinhood
			BINARY concurrent_put_get_remove_single_op_params
			TRACERS memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 400)

	add_engine_test(ENGINE robinhood
			BINARY concurrent_put_get_remove_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 8 50)

	add_engine_test(ENGINE robinhood
			BINARY concurrent_put_get_remove_gen_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 8 50 8)

	add_engine_test(ENGINE robinhood
			BINARY concurrent_iterate_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 24 200 0)

	add_engine_test(ENGINE robinhood
			BINARY persistent_not_found_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE robinhood
			BINARY persistent_overwrite_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE robinhood
			BINARY persistent_put_get_std_map_multiple_reopen
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 8 8)

	add_engine_test(ENGINE robinhood
			BINARY persistent_put_remove_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE robinhood
			BINARY persistent_put_verify_asc_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 4000)

	add_engine_test(ENGINE robinhood
			BINARY persistent_put_verify_desc_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 4000)

	add_engine_test(ENGINE robinhood
			BINARY persistent_put_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	if(TESTS_PMEMOBJ_DRD_HELGRIND)
		add_engine_test(ENGINE robinhood
			BINARY concurrent_put_get_remove_single_op_params
			TRACERS drd helgrind
			SCRIPT pmemobj_based/default.cmake
			PARAMS 250)

		add_engine_test(ENGINE robinhood
			BINARY concurrent_put_get_remove_params
			TRACERS drd helgrind
			SCRIPT pmemobj_based/default.cmake
			PARAMS 8 50)

		add_engine_test(ENGINE robinhood
			BINARY concurrent_put_get_remove_gen_params
			TRACERS drd helgrind
			SCRIPT pmemobj_based/default.cmake
			PARAMS 8 50 8)

		add_engine_test(ENGINE robinhood
			BINARY concurrent_iterate_params
			TRACERS drd helgrind
			SCRIPT pmemobj_based/default.cmake
			PARAMS 4 50 0)
	endif()

	add_engine_test(ENGINE robinhood
			BINARY pmemobj_error_handling_create
			TRACERS none memcheck
			SCRIPT pmemobj_based/pmemobj/error_handling_create.cmake)

	add_engine_test(ENGINE robinhood
			BINARY pmemobj_error_handling_tx_oid
			TRACERS none memcheck
			SCRIPT pmemobj_based/pmemobj/error_handling_tx_oid.cmake)

	add_engine_test(ENGINE robinhood
			BINARY pmemobj_error_handling_tx_path
			TRACERS none memcheck
			SCRIPT pmemobj_based/pmemobj/error_handling_tx_path.cmake)

	add_engine_test(ENGINE robinhood
			BINARY pmemobj_create_or_error_if_exists
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default_no_config.cmake)

	add_engine_test(ENGINE robinhood
			BINARY put_get_std_map
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/put_get_std_map_create_or_error_if_exists.cmake
			PARAMS 1000 8 8)

	add_engine_test(ENGINE robinhood
			BINARY put_get_std_map
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/create_if_missing.cmake
			PARAMS 128 8 8)

	add_engine_test(ENGINE robinhood
			BINARY pmemobj_put_get_std_map_oid
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/put_get_std_map_oid.cmake
			PARAMS 1000 8 8)

	if(PMREORDER_SUPPORTED)
		add_engine_test(ENGINE robinhood
				BINARY pmreorder_insert
				TRACERS none
				SCRIPT pmemobj_based/pmreorder/insert.cmake)

		add_engine_test(ENGINE robinhood
				BINARY pmreorder_erase
				TRACERS none
				SCRIPT pmemobj_based/pmreorder/erase.cmake)

		if (TESTS_LONG)
			add_engine_test(ENGINE robinhood
					BINARY pmreorder_recover
					TRACERS none
					SCRIPT pmemobj_based/pmreorder/recover.cmake)
		endif()
	endif()
endif()
################################################################################
###################################### DRAM_VCMAP ###################################
if(ENGINE_DRAM_VCMAP)
	add_engine_test(ENGINE dram_vcmap
			BINARY c_api_null_db_config
			TRACERS none memcheck
			SCRIPT dram/default.cmake)

	add_engine_test(ENGINE dram_vcmap
			BINARY put_get_remove
			TRACERS none memcheck
			SCRIPT dram/default.cmake)

	add_engine_test(ENGINE dram_vcmap
			BINARY put_get_remove_charset_params
			TRACERS none memcheck
			SCRIPT dram/default.cmake
			PARAMS 16 8)

	add_engine_test(ENGINE dram_vcmap
			BINARY put_get_remove_long_key
			TRACERS none memcheck
			SCRIPT dram/default.cmake)

	add_engine_test(ENGINE dram_vcmap
			BINARY put_get_remove_params
			TRACERS none
			SCRIPT dram/default.cmake
			PARAMS 400000)

	add_engine_test(ENGINE dram_vcmap
			BINARY put_get_remove_params
			TRACERS memcheck
			SCRIPT dram/default.cmake
			PARAMS 4000)

	add_engine_test(ENGINE dram_vcmap
			BINARY put_get_std_map
			TRACERS none memcheck
			SCRIPT dram/default.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE dram_vcmap
			BINARY iterate
			TRACERS none memcheck
			SCRIPT dram/default.cmake)

	add_engine_test(ENGINE dram_vcmap
			BINARY concurrent_put_get_remove_params
			TRACERS none memcheck # XXX - tbb lock does not work well with drd or helgrind
			SCRIPT dram/default.cmake
			PARAMS 8 50)

	add_engine_test(ENGINE dram_vcmap
			BINARY concurrent_put_get_remove_gen_params
			TRACERS none memcheck # XXX - tbb lock does not work well with drd or helgrind
			SCRIPT dram/default.cmake
			PARAMS 8 50 100)

	add_engine_test(ENGINE dram_vcmap
			BINARY concurrent_put_get_remove_single_op_params
			TRACERS none # XXX - tbb lock does not work well with drd or helgrind
			SCRIPT dram/default.cmake
			PARAMS 1000)

	add_engine_test(ENGINE dram_vcmap
			BINARY concurrent_put_get_remove_single_op_params
			TRACERS memcheck # XXX - tbb lock does not work well with drd or helgrind
			SCRIPT dram/default.cmake
			PARAMS 400)

	add_engine_test(ENGINE dram_vcmap
			BINARY iterator_basic
			TRACERS none memcheck
			SCRIPT dram/default.cmake)

	add_engine_test(ENGINE dram_vcmap
			BINARY iterator_concurrent
			TRACERS none memcheck
			SCRIPT dram/default.cmake
			PARAMS 8)

	add_engine_test(ENGINE dram_vcmap
			BINARY transaction_not_supported
			TRACERS none memcheck
			SCRIPT dram/default.cmake)
endif(ENGINE_DRAM_VCMAP)
################################################################################
###################################### DRAM_VCSMAP #############################
if(ENGINE_DRAM_VCSMAP)
	add_engine_test(ENGINE dram_vcsmap
			BINARY c_api_null_db_config
			TRACERS none memcheck
			SCRIPT dram/default.cmake)

	add_engine_test(ENGINE dram_vcsmap
			BINARY put_get_remove
			TRACERS none memcheck
			SCRIPT dram/default.cmake)

	add_engine_test(ENGINE dram_vcsmap
			BINARY put_get_remove_not_aligned
			TRACERS none memcheck
			SCRIPT dram/default.cmake)

	add_engine_test(ENGINE dram_vcsmap
			BINARY put_get_remove_charset_params
			TRACERS none memcheck
			SCRIPT dram/default.cmake
			PARAMS 16 8)

	add_engine_test(ENGINE dram_vcsmap
			BINARY put_get_remove_long_key
			TRACERS none memcheck
			SCRIPT dram/default.cmake)

	add_engine_test(ENGINE dram_vcsmap
			BINARY put_get_remove_params
			TRACERS none
			SCRIPT dram/default.cmake
			PARAMS 400000)

	add_engine_test(ENGINE dram_vcsmap
			BINARY put_get_remove_params
			TRACERS memcheck
			SCRIPT dram/default.cmake
			PARAMS 4000)

	add_engine_test(ENGINE dram_vcsmap
			BINARY put_get_std_map
			TRACERS none memcheck
			SCRIPT dram/default.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE dram_vcsmap
			BINARY iterate
			TRACERS none memcheck
			SCRIPT dram/default.cmake)

	add_engine_test(ENGINE dram_vcsmap
			BINARY concurrent_iterate_params
			TRACERS none memcheck
			SCRIPT dram/default.cmake
			PARAMS 24 200)

	add_engine_test(ENGINE dram_vcsmap
			BINARY concurrent_put_get_remove_params
			TRACERS none memcheck
			SCRIPT dram/default.cmake
			PARAMS 8 50)

	add_engine_test(ENGINE dram_vcsmap
			BINARY concurrent_put_get_remove_gen_params
			TRACERS none memcheck
			SCRIPT dram/default.cmake
			PARAMS 8 50 100)

	add_engine_test(ENGINE dram_vcsmap
			BINARY concurrent_put_get_remove_single_op_params
			TRACERS none
			SCRIPT dram/default.cmake
			PARAMS 1000)

	add_engine_test(ENGINE dram_vcsmap
			BINARY concurrent_put_get_remove_single_op_params
			TRACERS memcheck
			SCRIPT dram/default.cmake
			PARAMS 400)

	add_engine_test(ENGINE dram_vcsmap
			BINARY transaction_not_supported
			TRACERS none memcheck
			SCRIPT dram/default.cmake)
endif(ENGINE_DRAM_VCSMAP)
################################################################################

