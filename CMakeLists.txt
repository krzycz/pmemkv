# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2017-2021, Intel Corporation

cmake_minimum_required(VERSION 3.3)

project(pmemkv)
set(PMEMKV_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include(${PMEMKV_ROOT_DIR}/cmake/helpers.cmake)
set_version(VERSION)

# ----------------------------------------------------------------- #
## CMake build options
# ----------------------------------------------------------------- #
option(BUILD_DOC "build documentation" ON)
option(BUILD_EXAMPLES "build examples" ON)
option(BUILD_TESTS "build tests" ON)
option(BUILD_JSON_CONFIG "build the 'libpmemkv_json_config' library" ON)

option(TESTS_LONG "enable long running tests" OFF)
option(TESTS_USE_FORCED_PMEM "run tests with PMEM_IS_PMEM_FORCE=1 - it speeds up tests execution on emulated pmem" OFF)
option(TESTS_USE_VALGRIND "enable tests with valgrind (fail build if Valgrind not found)" ON)
option(TESTS_PMEMOBJ_DRD_HELGRIND "enable test of pmemobj engines under drd and helgrind (they should only be run on PMEM)" OFF)
option(TESTS_JSON "enable tests which require libpmemkv_json_config library (BUILD_JSON_CONFIG has to be ON)" ON)

option(COVERAGE "enable collecting of coverage data" OFF)
option(DEVELOPER_MODE "enable developer's checks" OFF)
option(CHECK_CPP_STYLE "check code style of C++ sources" OFF)
option(USE_CCACHE "use ccache if it is available in the system" ON)

# Each engine can be enabled separately.
option(ENGINE_CMAP "enable cmap engine" ON)
option(ENGINE_FMAP "enable fmap engine" ON)
option(ENGINE_VCMAP "enable vcmap engine" ON)
option(ENGINE_VSMAP "enable vsmap engine" ON)
option(ENGINE_CSMAP "enable experimental csmap engine (requires CXX_STANDARD to be set to value >= 14)" OFF)
option(ENGINE_STREE "enable experimental stree engine" ON)
option(ENGINE_TREE3 "enable experimental tree3 engine" OFF)
option(ENGINE_RADIX "enable experimental radix engine" OFF)
option(ENGINE_ROBINHOOD "enable experimental robinhood engine (requires CXX_STANDARD to be set to value >= 14)" OFF)
option(ENGINE_DRAM_VCMAP "enable testing dram_vcmap engine" OFF)

# ----------------------------------------------------------------- #
## Set required and useful variables
# ----------------------------------------------------------------- #
set(CXX_STANDARD 11 CACHE STRING "C++ language standard")

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD ${CXX_STANDARD})

# Specify and print the build type
set(DEFAULT_BUILD_TYPE "RelWithDebInfo")
set(predefined_build_types
	Debug
	Release
	RelWithDebInfo
	MinSizeRel)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE ${DEFAULT_BUILD_TYPE}
		CACHE STRING "choose the type of build (${predefined_build_types})" FORCE)
	message(STATUS "CMAKE_BUILD_TYPE not set, setting the default one: ${CMAKE_BUILD_TYPE}")
else()
	message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
	if(NOT CMAKE_BUILD_TYPE IN_LIST predefined_build_types)
		message(WARNING "Unusual build type was set, please make sure it's proper one. "
			"By default supported are only following: ${predefined_build_types}.")
	endif()
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PMEMKV_ROOT_DIR}/cmake)
set(LIBPMEMOBJ_CPP_REQUIRED_VERSION 1.12)
set(LIBPMEMOBJ_REQUIRED_VERSION 1.9.1)
set(LIBPMEM_REQUIRED_VERSION 1.7)
set(MEMKIND_REQUIRED_VERSION 1.8.0)
set(RAPIDJSON_REQUIRED_VERSION 1.0.0)

set(PKG_CONFIG_REQUIRES)
set(DEB_DEPENDS)
set(RPM_DEPENDS)

set(TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/test CACHE STRING "working directory for tests")
message(STATUS "TEST_DIR set to: \"${TEST_DIR}\"")

# Do not treat include directories from the interfaces
# of consumed Imported Targets as SYSTEM by default.
set(CMAKE_NO_SYSTEM_FROM_IMPORTED 1)

set(SOURCE_FILES
	src/libpmemkv.cc
	src/libpmemkv.h
	src/engine.cc
	src/engines/blackhole.cc
	src/engines/blackhole.h
	src/out.cc
	src/out.h
	src/iterator.h
	src/iterator.cc
)
# Add each engine source separately
if(ENGINE_CMAP)
	list(APPEND SOURCE_FILES
		src/engines/cmap.h
		src/engines/cmap.cc
	)
endif()
if(ENGINE_FMAP)
	list(APPEND SOURCE_FILES
		src/engines/db.hpp
		src/engines/Utils.hpp
		src/engines/AepManager.hpp
		src/engines/AepManager.cpp
		src/engines/fmap.h
		src/engines/fmap.cc
	)
endif()
if(ENGINE_CSMAP)
	list(APPEND SOURCE_FILES
		src/engines-experimental/csmap.h
		src/engines-experimental/csmap.cc
	)
endif()
if(ENGINE_VCMAP)
	list(APPEND SOURCE_FILES
		src/engines/basic_vcmap.h
		src/engines/vcmap.h
		src/engines/vcmap.cc
	)
endif()
if(ENGINE_VSMAP)
	list(APPEND SOURCE_FILES
		src/engines/vsmap.h
		src/engines/vsmap.cc
	)
endif()
if(ENGINE_STREE)
	list(APPEND SOURCE_FILES
		src/engines-experimental/stree.h
		src/engines-experimental/stree.cc
		src/engines-experimental/stree/persistent_b_tree.h
	)
endif()
if(ENGINE_TREE3)
	list(APPEND SOURCE_FILES
		src/engines-experimental/tree3.h
		src/engines-experimental/tree3.cc
	)
endif()
if(ENGINE_RADIX)
	list(APPEND SOURCE_FILES
		src/engines-experimental/radix.h
		src/engines-experimental/radix.cc
	)
endif()
if(ENGINE_ROBINHOOD)
	list(APPEND SOURCE_FILES
		src/engines-experimental/robinhood.h
		src/engines-experimental/robinhood.cc
		src/fast_hash.h
		src/fast_hash.cc
	)
endif()
if(ENGINE_DRAM_VCMAP)
	list(APPEND SOURCE_FILES
		src/engines/basic_vcmap.h
		src/engines-testing/dram_vcmap.h
		src/engines-testing/dram_vcmap.cc
	)
endif()

# ----------------------------------------------------------------- #
## Setup defines and check status of each engine
# ----------------------------------------------------------------- #
if(ENGINE_CMAP)
	add_definitions(-DENGINE_CMAP)
	message(STATUS "CMAP engine is ON")
else()
	message(STATUS "CMAP engine is OFF")
endif()
if(ENGINE_FMAP)
	add_definitions(-DENGINE_FMAP)
	message(STATUS "FMAP engine is ON")
else()
	message(STATUS "FMAP engine is OFF")
endif()
if(ENGINE_CSMAP)
	add_definitions(-DENGINE_CSMAP)
	message(STATUS "CSMAP engine is ON")

	if(CXX_STANDARD LESS 14)
		message(FATAL_ERROR "CXX_STANDARD must be >= 14 if ENGINE_CSMAP is ON")
	endif()
else()
	message(STATUS "CSMAP engine is OFF")
endif()
if(ENGINE_VCMAP)
	add_definitions(-DENGINE_VCMAP)
	message(STATUS "VCMAP engine is ON")
else()
	message(STATUS "VCMAP engine is OFF")
endif()
if(ENGINE_VSMAP)
	add_definitions(-DENGINE_VSMAP)
	message(STATUS "VSMAP engine is ON")
else()
	message(STATUS "VSMAP engine is OFF")
endif()
if(ENGINE_STREE)
	add_definitions(-DENGINE_STREE)
	message(STATUS "STREE engine is ON")
else()
	message(STATUS "STREE engine is OFF")
endif()
if(ENGINE_TREE3)
	add_definitions(-DENGINE_TREE3)
	message(STATUS "TREE3 engine is ON")
else()
	message(STATUS "TREE3 engine is OFF")
endif()
if(ENGINE_RADIX)
	add_definitions(-DENGINE_RADIX)
	message(STATUS "RADIX engine is ON")
else()
	message(STATUS "RADIX engine is OFF")
endif()
if(ENGINE_ROBINHOOD)
	add_definitions(-DENGINE_ROBINHOOD)
	message(STATUS "ROBINHOOD engine is ON")

	if(CXX_STANDARD LESS 14)
		message(FATAL_ERROR "CXX_STANDARD must be >= 14 if ENGINE_ROBINHOOD is ON")
	endif()
else()
	message(STATUS "ROBINHOOD engine is OFF")
endif()
if(ENGINE_DRAM_VCMAP)
	add_definitions(-DENGINE_DRAM_VCMAP)
	message(STATUS "DRAM_VCMAP engine is ON")
else()
	message(STATUS "DRAM_VCMAP engine is OFF")
endif()

# ----------------------------------------------------------------- #
## Set compiler's flags
# ----------------------------------------------------------------- #
if(DEVELOPER_MODE)
	add_common_flag(-Werror)
endif()
if(COVERAGE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -coverage")
endif()

if(ENGINE_FMAP)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4 -mavx -mavx2 -momit-leaf-frame-pointer")
endif()

add_common_flag(-Wall)
add_common_flag(-Wpointer-arith)
add_common_flag(-Wsign-compare)
add_common_flag(-Wunreachable-code-return)
add_common_flag(-Wmissing-variable-declarations)
add_common_flag(-fno-common)
add_common_flag(-Wunused-macros)
add_common_flag(-Wsign-conversion)

add_common_flag(-ggdb DEBUG)
add_common_flag(-DDEBUG DEBUG)

add_common_flag("-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2" RELEASE)

# ----------------------------------------------------------------- #
## Setup environment, find packages,
## configure list of package dependencies
# ----------------------------------------------------------------- #
find_package(PkgConfig QUIET)
include(FindPerl)
include(ExternalProject)
include(FindThreads)
include(CheckCXXSourceCompiles)
include(GNUInstallDirs)

include(libpmemobj++)
list(APPEND PKG_CONFIG_REQUIRES "libpmemobj++ >= ${LIBPMEMOBJ_CPP_REQUIRED_VERSION}")
list(APPEND RPM_DEPENDS "libpmemobj >= ${LIBPMEMOBJ_REQUIRED_VERSION}")
list(APPEND DEB_DEPENDS "libpmemobj1 (>= ${LIBPMEMOBJ_REQUIRED_VERSION}) | libpmemobj (>= ${LIBPMEMOBJ_REQUIRED_VERSION})")

# Configure the ccache as compiler launcher
find_program(CCACHE_FOUND ccache)
if(USE_CCACHE AND CCACHE_FOUND)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif()

if(ENGINE_VSMAP OR ENGINE_VCMAP)
	include(memkind)
	list(APPEND PKG_CONFIG_REQUIRES "memkind >= ${MEMKIND_REQUIRED_VERSION}")
	list(APPEND RPM_DEPENDS "memkind >= ${MEMKIND_REQUIRED_VERSION}")
	list(APPEND DEB_DEPENDS "libmemkind0 (>= ${MEMKIND_REQUIRED_VERSION})")
endif()

if(ENGINE_VCMAP OR ENGINE_DRAM_VCMAP)
	include(tbb)
	add_definitions(-DTBB_DEFINE_STD_HASH_SPECIALIZATIONS)
	list(APPEND PKG_CONFIG_REQUIRES tbb)
	list(APPEND RPM_DEPENDS tbb)
	list(APPEND DEB_DEPENDS libtbb2)
endif()

# ----------------------------------------------------------------- #
## Link libraries, setup targets
# ----------------------------------------------------------------- #
add_library(pmemkv SHARED ${SOURCE_FILES})
set_target_properties(pmemkv PROPERTIES SOVERSION 1)
target_link_libraries(pmemkv PRIVATE
	-Wl,--version-script=${PMEMKV_ROOT_DIR}/src/libpmemkv.map)

target_include_directories(pmemkv PRIVATE src/valgrind)
# Enable libpmemobj-cpp valgrind annotations
target_compile_options(pmemkv PRIVATE -DLIBPMEMOBJ_CPP_VG_ENABLED=1)

target_link_libraries(pmemkv PRIVATE ${LIBPMEMOBJ++_LIBRARIES})
if(ENGINE_VSMAP OR ENGINE_VCMAP)
	target_link_libraries(pmemkv PRIVATE ${MEMKIND_LIBRARIES})
endif()
if(ENGINE_VCMAP OR ENGINE_DRAM_VCMAP)
	target_link_libraries(pmemkv PRIVATE ${TBB_LIBRARIES})
endif()

# ----------------------------------------------------------------- #
## Setup additional targets
# ----------------------------------------------------------------- #
add_custom_target(checkers ALL)
add_custom_target(cppstyle)
add_custom_target(cppformat)
add_custom_target(check-whitespace)
add_custom_target(check-license
	COMMAND ${PMEMKV_ROOT_DIR}/utils/check_license/check-headers.sh
		${PMEMKV_ROOT_DIR}
		BSD-3-Clause)
add_custom_target(copyright-format
	COMMAND ${PMEMKV_ROOT_DIR}/utils/check_license/check-headers.sh
		${PMEMKV_ROOT_DIR}
		BSD-3-Clause -d)

if(CHECK_CPP_STYLE)
	# check for required programs for code style check and add dependency to ALL
	find_program(CLANG_FORMAT NAMES clang-format clang-format-9 clang-format-9.0)
	set(CLANG_FORMAT_REQUIRED "9.0")
	if(CLANG_FORMAT)
		get_program_version_major_minor(${CLANG_FORMAT} CLANG_FORMAT_VERSION)
		message(STATUS "Found clang-format: ${CLANG_FORMAT} (version: ${CLANG_FORMAT_VERSION})")
		if(NOT (CLANG_FORMAT_VERSION VERSION_EQUAL CLANG_FORMAT_REQUIRED))
			message(FATAL_ERROR "required clang-format version is ${CLANG_FORMAT_REQUIRED}")
		endif()
	else()
		message(FATAL_ERROR "CHECK_CPP_STYLE=ON, but clang-format not found (required version: ${CLANG_FORMAT_REQUIRED})")
	endif()

	add_dependencies(checkers cppstyle)
endif()

if(DEVELOPER_MODE)
	# check for required programs for whitespace and license checks and add dependencies to ALL
	if(NOT PERL_FOUND)
		message(FATAL_ERROR "Perl not found")
	endif()
	if(PERL_VERSION_STRING VERSION_LESS 5.16)
		message(FATAL_ERROR "Minimum required version of Perl is 5.16)")
	endif()

	execute_process(COMMAND ${PERL_EXECUTABLE} -MText::Diff -e ""
			ERROR_QUIET
			RESULT_VARIABLE PERL_TEXT_DIFF_STATUS)
	if(PERL_TEXT_DIFF_STATUS)
		message(FATAL_ERROR "Text::Diff Perl module not found (install libtext-diff-perl or perl-Text-Diff)")
	endif()

	add_dependencies(checkers check-whitespace)
	add_dependencies(checkers check-license)
endif()

add_cppstyle(src ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/src/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/src/comparator/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines-experimental/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines-experimental/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines-experimental/stree/*.h*)

add_check_whitespace(main ${PMEMKV_ROOT_DIR}/utils/check_whitespace
			${CMAKE_CURRENT_SOURCE_DIR}/utils/check_license/*.sh
			${CMAKE_CURRENT_SOURCE_DIR}/*.md)

add_check_whitespace(src ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/src/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/src/comparator/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines-experimental/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines-experimental/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines-experimental/stree/*.h*)

# ----------------------------------------------------------------- #
## Add sub-directories if builds enabled
# ----------------------------------------------------------------- #
if(BUILD_JSON_CONFIG)
	include(rapidjson)
	list(APPEND PKG_CONFIG_REQUIRES "RapidJSON >= ${RAPIDJSON_REQUIRED_VERSION}")
	list(APPEND RPM_DEPENDS "rapidjson-devel >= ${RAPIDJSON_REQUIRED_VERSION}")
	list(APPEND DEB_DEPENDS "rapidjson-dev (>= ${RAPIDJSON_REQUIRED_VERSION})")
	add_definitions(-DBUILD_JSON_CONFIG)

	set(SOURCE_FILES_JSON_CONFIG
		src/libpmemkv_json_config.cc
		src/libpmemkv_json_config.h
		src/out.cc
		src/out.h
	)

	add_library(pmemkv_json_config SHARED ${SOURCE_FILES_JSON_CONFIG})
	set_target_properties(pmemkv_json_config PROPERTIES SOVERSION 1)
	target_link_libraries(pmemkv_json_config PRIVATE pmemkv ${RapidJSON_LIBRARIES}
		-Wl,--version-script=${PMEMKV_ROOT_DIR}/src/libpmemkv_json_config.map)
	set_target_properties(pmemkv_json_config
		PROPERTIES PUBLIC_HEADER "src/libpmemkv_json_config.h")

	configure_file(libpmemkv_json_config.pc.in libpmemkv_json_config.pc @ONLY)

	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libpmemkv_json_config.pc
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

	install(TARGETS pmemkv_json_config
		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

if(BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()

if(BUILD_DOC)
	add_subdirectory(doc)
endif()

# ----------------------------------------------------------------- #
## Configure make install/uninstall and packages
# ----------------------------------------------------------------- #
string(REPLACE ";" " " PKG_CONFIG_REQUIRES "${PKG_CONFIG_REQUIRES}")
string(REPLACE ";" ", " RPM_PACKAGE_REQUIRES "${RPM_DEPENDS}")
string(REPLACE ";" ", " DEB_PACKAGE_REQUIRES "${DEB_DEPENDS}")

configure_file(libpmemkv.pc.in libpmemkv.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libpmemkv.pc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

set_target_properties(pmemkv PROPERTIES PUBLIC_HEADER "src/libpmemkv.h;src/libpmemkv.hpp")

install(TARGETS pmemkv
		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

configure_file(
	"${PMEMKV_ROOT_DIR}/cmake/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY)

add_custom_target(uninstall
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

if(NOT "${CPACK_GENERATOR}" STREQUAL "")
	include(${PMEMKV_ROOT_DIR}/cmake/packages.cmake)
endif()
