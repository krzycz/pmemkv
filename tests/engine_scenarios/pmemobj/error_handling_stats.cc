// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj/pool_base.h>

static void FailsToCreateInstanceWithInvalidStatParam(std::string engine,
						     pmem::kv::config &&config)
{
	auto s = config.put_int64("pmemobj_stats_enabled", 4);
	UT_ASSERTeq(pmem::kv::status::OK, s);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(config));

	/* wrong pmemobj_stats_enabled parameter */
	UT_ASSERTeq(pmem::kv::status::INVALID_ARGUMENT, s);

	kv.close();
}

static void test(int argc, char *argv[])
{
	if (argc < 2)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	auto engine = argv[1];

	FailsToCreateInstanceWithInvalidStatParam(engine, CONFIG_FROM_JSON(argv[2]));
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
