// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#include "unittest.hpp"

#include "pmem_allocator.h"

#ifdef USE_LIBMEMKIND_NAMESPACE
namespace memkind_ns = libmemkind::pmem;
#else
namespace memkind_ns = pmem;
#endif

static bool older_memkind;

static void FailsToOpenInstanceWithInvalidPath(std::string engine,
					       std::string non_existent_path)
{
	pmem::kv::config cfg;
	auto s = cfg.put_path(non_existent_path);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = cfg.put_size(MEMKIND_PMEM_MIN_SIZE);
	ASSERT_STATUS(s, pmem::kv::status::OK);

	pmem::kv::db kv;
	/* Non-existent path supplied */
	s = kv.open(engine, std::move(cfg));

	if (older_memkind) {
		ASSERT_STATUS(s, pmem::kv::status::UNKNOWN_ERROR);
	} else {
		ASSERT_STATUS(s, pmem::kv::status::INVALID_ARGUMENT);
	}
}

static void FailsToCreateInstanceWithTooSmallSize(std::string engine, std::string path)
{
	pmem::kv::config cfg;
	auto s = cfg.put_path(path);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = cfg.put_size(MEMKIND_PMEM_MIN_SIZE - 1);
	ASSERT_STATUS(s, pmem::kv::status::OK);

	pmem::kv::db kv;
	/* Too small size supplied */
	s = kv.open(engine, std::move(cfg));

	if (older_memkind) {
		ASSERT_STATUS(s, pmem::kv::status::UNKNOWN_ERROR);
	} else {
		ASSERT_STATUS(s, pmem::kv::status::INVALID_ARGUMENT);
	}
}

static void NoSizeInConfig(std::string engine, std::string path)
{
	pmem::kv::config cfg;
	auto s = cfg.put_path(path);
	ASSERT_STATUS(s, pmem::kv::status::OK);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(cfg));

	ASSERT_STATUS(s, pmem::kv::status::INVALID_ARGUMENT);
}

static void NoPathInConfig(std::string engine)
{
	pmem::kv::config cfg;
	auto s = cfg.put_size(MEMKIND_PMEM_MIN_SIZE);
	ASSERT_STATUS(s, pmem::kv::status::OK);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(cfg));

	ASSERT_STATUS(s, pmem::kv::status::INVALID_ARGUMENT);
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine non_existent_path correct_path", argv[0]);

	/* Check if memkind has extended error handling
	XXX: Remove that when we won't support memkind < 1.12 */
	try {
		auto alloc = memkind_ns::allocator<int>(argv[2], 100000000);
	} catch (std::invalid_argument &e) {
		older_memkind = false;
	} catch (std::exception &e) {
		older_memkind = true;
	}

	FailsToOpenInstanceWithInvalidPath(argv[1], argv[2]);
	FailsToCreateInstanceWithTooSmallSize(argv[1], argv[3]);
	NoSizeInConfig(argv[1], argv[3]);
	NoPathInConfig(argv[1]);
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
