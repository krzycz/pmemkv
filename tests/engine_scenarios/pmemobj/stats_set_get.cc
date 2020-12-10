// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj.h>
#include <libpmemobj/pool_base.h>

static void GetStatDefault(std::string engine, pmem::kv::config &&config)
{
	pmem::kv::db kv;
	auto s = kv.open(engine, std::move(config));
	UT_ASSERTeq(pmem::kv::status::OK, s);

	pmem::kv::statistics stat;
	kv.stats(&stat);
	UT_ASSERTeq(POBJ_STATS_ENABLED_TRANSIENT, stat.stats_enabled);

	kv.close();
}

static void SetGetStatTranscient(std::string engine, pmem::kv::config &&config)
{
	pmem::kv::db kv;
	auto s = config.put_int64("pmemobj_stats_enabled", 0);
	UT_ASSERTeq(pmem::kv::status::OK, s);

	s = kv.open(engine, std::move(config));

	UT_ASSERTeq(pmem::kv::status::OK, s);

	pmem::kv::statistics stat;
	kv.stats(&stat);

	UT_ASSERTeq(POBJ_STATS_ENABLED_TRANSIENT, stat.stats_enabled);

	kv.close();
}

static void SetGetStatTranscientPersistent(std::string engine, pmem::kv::config &&config)
{
	pmem::kv::db kv;
	auto s = config.put_int64("pmemobj_stats_enabled", 1);
	UT_ASSERTeq(pmem::kv::status::OK, s);

	s = kv.open(engine, std::move(config));

	UT_ASSERTeq(pmem::kv::status::OK, s);

	pmem::kv::statistics stat;
	kv.stats(&stat);

	UT_ASSERTeq(POBJ_STATS_ENABLED_BOTH, stat.stats_enabled);

	kv.close();
}

static void SetGetStatPersistent(std::string engine, pmem::kv::config &&config)
{
	pmem::kv::db kv;
	auto s = config.put_int64("pmemobj_stats_enabled", 2);
	UT_ASSERTeq(pmem::kv::status::OK, s);

	s = kv.open(engine, std::move(config));

	UT_ASSERTeq(pmem::kv::status::OK, s);

	pmem::kv::statistics stat;
	kv.stats(&stat);

	UT_ASSERTeq(POBJ_STATS_ENABLED_PERSISTENT, stat.stats_enabled);

	kv.close();
}

static void SetGetStatDisable(std::string engine, pmem::kv::config &&config)
{
	pmem::kv::db kv;
	auto s = config.put_int64("pmemobj_stats_enabled", 3);
	UT_ASSERTeq(pmem::kv::status::OK, s);

	s = kv.open(engine, std::move(config));

	UT_ASSERTeq(pmem::kv::status::OK, s);

	pmem::kv::statistics stat;
	kv.stats(&stat);

	UT_ASSERTeq(POBJ_STATS_DISABLED, stat.stats_enabled);

	kv.close();
}

static void test(int argc, char *argv[])
{
	if (argc < 2)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	auto engine = argv[1];

	GetStatDefault(engine, CONFIG_FROM_JSON(argv[2]));
	SetGetStatTranscient(engine, CONFIG_FROM_JSON(argv[2]));
	SetGetStatTranscientPersistent(engine, CONFIG_FROM_JSON(argv[2]));
	SetGetStatPersistent(engine, CONFIG_FROM_JSON(argv[2]));
	SetGetStatDisable(engine, CONFIG_FROM_JSON(argv[2]));
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
