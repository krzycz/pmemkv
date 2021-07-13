// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "error_handling_tx.hpp"

#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

struct Root {
	PMEMoid oid;
};

static void test(int argc, char *argv[])
{
	using namespace std::placeholders;

	if (argc < 3)
		UT_FATAL("usage: %s engine path", argv[0]);

	auto pmemobj_pool_path = std::string(argv[2]);
	pmem::obj::pool<Root> pmemobj_pool;

	auto layout = std::string("pmemkv");
	auto engine = std::string(argv[1]);
	if (engine != "cmap")
		layout = "pmemkv_" + engine;

	try {
		pmemobj_pool = pmem::obj::pool<Root>::open(pmemobj_pool_path, layout);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pmem::kv::config cfg;
	auto s = cfg.put_oid(&(pmemobj_pool.root()->oid));

	ASSERT_STATUS(s, status::OK);

	{
		auto kv = INITIALIZE_KV(engine, std::move(cfg));

		TransactionTest(pmemobj_pool, kv);
	}

	pmemobj_pool.close();
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
