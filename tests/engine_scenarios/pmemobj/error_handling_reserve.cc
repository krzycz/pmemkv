// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

static void reserve_and_open(std::string engine, std::string config, size_t reserve_count)
{
	auto cfg = CONFIG_FROM_JSON(config);
	cfg.reserve(reserve_count);

	pmem::kv::db kv;
	auto s = kv.open(engine, std::move(cfg));
	ASSERT_STATUS(s, pmem::kv::status::OUT_OF_MEMORY);

	kv.close();
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	/* this elements' count should exceed db size */
	reserve_and_open(argv[1], argv[2], 5000000);

	reserve_and_open(argv[1], argv[2], std::numeric_limits<std::size_t>::max());
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
