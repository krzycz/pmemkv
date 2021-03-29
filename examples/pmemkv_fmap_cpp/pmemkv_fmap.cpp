// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

/*
 * pmemkv_fmap.cpp -- example usage of fmap engine.
 */

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <libpmemkv.hpp>
#include <sstream>

#define ASSERT(expr)                                                                     \
	do {                                                                             \
		if (!(expr))                                                             \
			std::cout << pmemkv_errormsg() << std::endl;                     \
		assert(expr);                                                            \
	} while (0)
#define LOG(msg) std::cout << msg << std::endl

using namespace pmem::kv;

const uint64_t SIZE = 1024UL * 1024UL * 1024UL;

//#define LONG_VAL
int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " file\n";
		exit(1);
	}

	/* See libpmemkv_config(3) for more detailed example of config creation */
	LOG("Creating config");
	config cfg;

	status s = cfg.put_path(argv[1]);
	ASSERT(s == status::OK);
	s = cfg.put_size(SIZE);
	ASSERT(s == status::OK);
	s = cfg.put_force_create(true);
	ASSERT(s == status::OK);

	LOG("Opening pmemkv database with 'fmap' engine");
	db *kv = new db();
	ASSERT(kv != nullptr);
	s = kv->open("fmap", std::move(cfg));
	ASSERT(s == status::OK);

	LOG("Putting new key");
	s = kv->put("key0000000000001", "value1");
	ASSERT(s == status::OK);

#if 0
	size_t cnt;
	s = kv->count_all(cnt);
	ASSERT(s == status::OK && cnt == 1);
#endif

	LOG("Reading key back");
	std::string value;
	s = kv->get("key0000000000001", &value);
	ASSERT(s == status::OK && value == "value1");

	LOG("Iterating existing keys");

#ifndef LONG_VAL
	s = kv->put("key2", "value2");
	ASSERT(s == status::OK);

	s = kv->put("key3", "value3");
	ASSERT(s == status::OK);

	s = kv->get("key2", &value);
	ASSERT(s == status::OK && value == "value2");

	s = kv->get("key3", &value);
	ASSERT(s == status::OK && value == "value3");
#else
	s = kv->put("key0000000000002", "value111111111111111111111111111111111111111111111111111111111\
	2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222");
	ASSERT(s == status::OK);
	s = kv->put("key0000000000003", "value111111111111111111111111111111111111111111111111111111111\
	2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222223");
	ASSERT(s == status::OK);

	s = kv->get("key0000000000002", &value);
	ASSERT(s == status::OK && value == "value111111111111111111111111111111111111111111111111111111111\
	2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222");
	s = kv->get("key0000000000003", &value);
	ASSERT(s == status::OK && value == "value111111111111111111111111111111111111111111111111111111111\
	2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222223");

#endif

#if 0
	kv->get_all([](string_view k, string_view v) {
		LOG("  visited: " << k.data());
		return 0;
	});

	LOG("Defragmenting the database");
	s = kv->defrag(0, 100);
	ASSERT(s == status::OK);

	LOG("Removing existing key");
	s = kv->remove("key1");
	ASSERT(s == status::OK);
	s = kv->exists("key1");
	ASSERT(s == status::NOT_FOUND);
#endif
	/* Examples of using pmem:kv:status with std::ostream and operator<<,
	 * it's useful for debugging. */
	/* Print status */
	std::cout << s << std::endl;

#if 0
	/* Write status to ostringstream */
	std::ostringstream oss;
	oss << s;
	assert(oss.str() == "NOT_FOUND (2)");
#endif

	LOG("Closing database");
	delete kv;

	return 0;
}
