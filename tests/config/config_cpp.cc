// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "unittest.hpp"
#include <libpmemkv.hpp>
#include <string>
#include <vector>

#include <limits>

/**
 * Tests all config methods using C++ API
 */

using namespace pmem::kv;

static const int INIT_VAL = 1;
static const int DELETED_VAL = 2;
static const char *PATH = "/some/path";
static const uint64_t SIZE = 0xDEADBEEF;

struct custom_type {
	int a;
	char b;
};

static void deleter(custom_type *ct_ptr)
{
	ct_ptr->a = DELETED_VAL;
	ct_ptr->b = DELETED_VAL;
}

static void simple_test()
{
	/**
	 * TEST: add and read data from config, using all available methods.
	 */
	auto cfg = new config;
	UT_ASSERT(cfg != nullptr);

	status s = cfg->put_string("string", "abc");
	ASSERT_STATUS(s, status::OK);

	s = cfg->put_int64("int", 123);
	ASSERT_STATUS(s, status::OK);

	custom_type *ptr = new custom_type;
	ptr->a = INIT_VAL;
	ptr->b = INIT_VAL;
	s = cfg->put_object("object_ptr", ptr, nullptr);
	ASSERT_STATUS(s, status::OK);

	s = cfg->put_data("object", ptr);
	ASSERT_STATUS(s, status::OK);

	int array[3] = {1, 15, 77};
	s = cfg->put_data("array", array, 3);
	ASSERT_STATUS(s, status::OK);

	custom_type *ptr_deleter = new custom_type;
	ptr_deleter->a = INIT_VAL;
	ptr_deleter->b = INIT_VAL;
	s = cfg->put_object("object_ptr_with_deleter", ptr_deleter,
			    (void (*)(void *)) & deleter);
	ASSERT_STATUS(s, status::OK);

	s = cfg->put_path(PATH);
	ASSERT_STATUS(s, status::OK);

	s = cfg->put_size(SIZE);
	ASSERT_STATUS(s, status::OK);

	s = cfg->put_force_create(true);
	ASSERT_STATUS(s, status::OK);

	s = cfg->reserve(256);
	ASSERT_STATUS(s, status::OK);

	std::string value_string;
	s = cfg->get_string("string", value_string);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERT(value_string == "abc");

	int64_t value_int;
	s = cfg->get_int64("int", value_int);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(value_int, 123);

	custom_type *value_custom_ptr;
	s = cfg->get_object("object_ptr", value_custom_ptr);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(value_custom_ptr->a, INIT_VAL);
	UT_ASSERTeq(value_custom_ptr->b, INIT_VAL);

	custom_type *value_custom_ptr_deleter;
	s = cfg->get_object("object_ptr_with_deleter", value_custom_ptr_deleter);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(value_custom_ptr_deleter->a, INIT_VAL);
	UT_ASSERTeq(value_custom_ptr_deleter->b, INIT_VAL);

	custom_type *value_custom;
	size_t value_custom_count = 0;
	s = cfg->get_data("object", value_custom, value_custom_count);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(value_custom_count, 1U);
	UT_ASSERTeq(value_custom->a, INIT_VAL);
	UT_ASSERTeq(value_custom->b, INIT_VAL);

	int *value_array;
	size_t value_array_count = 0;
	s = cfg->get_data("array", value_array, value_array_count);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(value_array_count, 3U);
	UT_ASSERTeq(value_array[0], 1);
	UT_ASSERTeq(value_array[1], 15);
	UT_ASSERTeq(value_array[2], 77);

	int64_t none;
	ASSERT_STATUS(cfg->get_int64("non-existent", none), status::NOT_FOUND);

	s = cfg->get_string("path", value_string);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERT(value_string == PATH);

	uint64_t int_us;
	s = cfg->get_uint64("size", int_us);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(int_us, SIZE);

	s = cfg->get_uint64("force_create", int_us);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(int_us, 1);

	s = cfg->get_uint64("reserve", int_us);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(int_us, 256);

	delete cfg;
	cfg = nullptr;

	UT_ASSERTeq(value_custom_ptr_deleter->a, DELETED_VAL);
	UT_ASSERTeq(value_custom_ptr_deleter->b, DELETED_VAL);

	/* delete was not set */
	UT_ASSERTeq(ptr, value_custom_ptr);
	UT_ASSERTeq(value_custom_ptr->a, INIT_VAL);
	UT_ASSERTeq(value_custom_ptr->b, INIT_VAL);

	delete ptr;
	delete ptr_deleter;
}

static void put_edge_cases()
{
	/**
	 * TEST: Edge cases input data for some methods.
	 */

	auto cfg = new config;
	auto s = cfg->put_force_create(false);
	ASSERT_STATUS(s, status::OK);

	uint64_t int_us;
	s = cfg->get_uint64("force_create", int_us);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(int_us, 0);

	auto max_size = std::numeric_limits<uint64_t>::max();
	s = cfg->put_size(max_size);
	ASSERT_STATUS(s, status::OK);

	s = cfg->get_uint64("size", int_us);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(int_us, max_size);

	delete cfg;

	/* Some of those strings are not real paths, but config should not crash on them
	 * */
	std::vector<std::string> paths = {" ", "", "//",
					  ",./;'[]-=<>?:\"{}|_+!@#$%^&*()`~", "/👾"};
	for (auto path : paths) {
		auto cfg = new config();
		s = cfg->put_path(path);
		ASSERT_STATUS(s, status::OK);

		std::string value_string;
		s = cfg->get_string("path", value_string);
		ASSERT_STATUS(s, status::OK);
		UT_ASSERT(value_string == path);
		delete cfg;
	}
}

static void put_oid_simple_test()
{
	/**
	 * TEST: basic check for put_oid method.
	 */

	auto cfg = new config;
	UT_ASSERT(cfg != NULL);

	PMEMoid oid;
	status ret = cfg->put_oid(&oid);
	ASSERT_STATUS(ret, status::OK);

	PMEMoid *oid_ptr;

	ret = cfg->get_object("oid", oid_ptr);
	ASSERT_STATUS(ret, status::OK);
	UT_ASSERTeq(&oid, oid_ptr);

	delete cfg;
}

static void object_unique_ptr_default_deleter_test()
{
	auto cfg = new config;
	UT_ASSERT(cfg != nullptr);

	auto ptr_default = std::unique_ptr<custom_type>(new custom_type);
	ptr_default->a = INIT_VAL;
	ptr_default->b = INIT_VAL;
	auto s = cfg->put_object("object_ptr", std::move(ptr_default));
	ASSERT_STATUS(s, status::OK);

	delete cfg;
}

static void object_unique_ptr_nullptr_test()
{
	auto cfg = new config;
	UT_ASSERT(cfg != nullptr);

	auto ptr = std::unique_ptr<custom_type>(nullptr);
	auto s = cfg->put_object("object_ptr", std::move(ptr));
	ASSERT_STATUS(s, status::OK);

	custom_type *raw_ptr;
	s = cfg->get_object("object_ptr", raw_ptr);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(raw_ptr, nullptr);

	delete cfg;
}

static void object_unique_ptr_custom_deleter_test()
{
	auto cfg = new config;
	UT_ASSERT(cfg != nullptr);

	auto custom_deleter = [&](custom_type *ptr) {
		ptr->a = DELETED_VAL;
		ptr->b = DELETED_VAL;
	};

	auto ptr_custom = std::unique_ptr<custom_type, decltype(custom_deleter)>(
		new custom_type, custom_deleter);
	ptr_custom->a = INIT_VAL;
	ptr_custom->b = INIT_VAL;

	auto *raw_ptr = ptr_custom.get();

	auto s = cfg->put_object("object_ptr", std::move(ptr_custom));
	ASSERT_STATUS(s, status::OK);

	delete cfg;

	UT_ASSERTeq(raw_ptr->a, DELETED_VAL);
	UT_ASSERTeq(raw_ptr->b, DELETED_VAL);
	delete raw_ptr;
}

static void integral_conversion_test()
{
	/**
	 * TEST: when reading data from config it's allowed to read integers
	 * into different type (then it was originally stored), as long as
	 * the conversion is possible. CONFIG_TYPE_ERROR should be returned
	 * when e.g. reading negative integral value into signed int type.
	 */
	auto cfg = new config;
	UT_ASSERT(cfg != nullptr);

	status s = cfg->put_int64("int", 123);
	ASSERT_STATUS(s, status::OK);

	s = cfg->put_uint64("uint", 123);
	ASSERT_STATUS(s, status::OK);

	s = cfg->put_int64("negative-int", -123);
	ASSERT_STATUS(s, status::OK);

	s = cfg->put_uint64("uint-max", std::numeric_limits<size_t>::max());
	ASSERT_STATUS(s, status::OK);

	int64_t int_s;
	s = cfg->get_int64("int", int_s);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(int_s, 123);

	size_t int_us;
	s = cfg->get_uint64("int", int_us);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(int_us, 123U);

	int64_t uint_s;
	s = cfg->get_int64("uint", uint_s);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(uint_s, 123);

	size_t uint_us;
	s = cfg->get_uint64("uint", uint_us);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(uint_us, 123U);

	int64_t neg_int_s;
	s = cfg->get_int64("negative-int", neg_int_s);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(neg_int_s, -123);

	size_t neg_int_us;
	s = cfg->get_uint64("negative-int", neg_int_us);
	ASSERT_STATUS(s, status::CONFIG_TYPE_ERROR);

	int64_t uint_max_s;
	s = cfg->get_int64("uint-max", uint_max_s);
	ASSERT_STATUS(s, status::CONFIG_TYPE_ERROR);

	size_t uint_max_us;
	s = cfg->get_uint64("uint-max", uint_max_us);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(uint_max_us, std::numeric_limits<size_t>::max());

	delete cfg;
}

static void constructors_test()
{
	/**
	 * TEST: in C++ API there is more than one way to create config's object
	 */
	auto cfg = new config;
	UT_ASSERT(cfg != nullptr);

	/* assign released C++ config to C config;
	 * it's null because config is lazy initialized */
	pmemkv_config *c_cfg = cfg->release();
	UT_ASSERTeq(c_cfg, nullptr);

	/* put value to C++ config */
	auto s = cfg->put_int64("int", 65535);
	ASSERT_STATUS(s, status::OK);

	/* use move constructor and test if data is still accessible */
	config *move_config = new config(std::move(*cfg));
	int64_t int_s;
	s = move_config->get_int64("int", int_s);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(int_s, 65535);

	/* release new C++ config and test if data is accessible in C config */
	c_cfg = move_config->release();
	auto ret = pmemkv_config_get_int64(c_cfg, "int", &int_s);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERTeq(int_s, 65535);

	/* check if moved config is empty */
	s = move_config->get_int64("int", int_s);
	ASSERT_STATUS(s, status::NOT_FOUND);

	/* create new config... */
	delete cfg;
	cfg = new config();
	s = cfg->put_string("string", "config");
	ASSERT_STATUS(s, status::OK);

	config *move_assign = new config();
	s = move_assign->put_string("move_string", "value");
	ASSERT_STATUS(s, status::OK);

	/* ... and check move assignment operator
		from different and the same config */
	*move_assign = std::move(*cfg);
	auto &move_assign2 = *move_assign;
	*move_assign = std::move(move_assign2);

	std::string string_s;
	s = move_assign->get_string("move_string", string_s);
	ASSERT_STATUS(s, status::NOT_FOUND);
	s = move_assign->get_string("string", string_s);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERT(string_s == "config");

	/* cleanup */
	pmemkv_config_delete(c_cfg);
	delete move_config;
	delete move_assign;
	delete cfg;
}

static void not_found_test()
{
	/**
	 * TEST: all config get_* methods should return status NOT_FOUND if item
	 * does not exist
	 */
	auto cfg = new config;
	UT_ASSERT(cfg != nullptr);

	/* config is nullptr; all gets should return NotFound */
	std::string my_string;
	int64_t my_int;
	uint64_t my_uint;
	custom_type *my_object;
	size_t my_object_count = 0;

	ASSERT_STATUS(cfg->get_string("string", my_string), status::NOT_FOUND);
	ASSERT_STATUS(cfg->get_int64("int", my_int), status::NOT_FOUND);
	ASSERT_STATUS(cfg->get_uint64("uint", my_uint), status::NOT_FOUND);
	ASSERT_STATUS(cfg->get_object("object", my_object), status::NOT_FOUND);
	ASSERT_STATUS(cfg->get_data("data", my_object, my_object_count),
		      status::NOT_FOUND);
	UT_ASSERTeq(my_object_count, 0U);

	/* initialize config with any put */
	cfg->put_int64("init", 0);

	/* all gets should return NOT_FOUND when looking for non-existing key */
	ASSERT_STATUS(cfg->get_string("non-existent-string", my_string),
		      status::NOT_FOUND);
	ASSERT_STATUS(cfg->get_int64("non-existent-int", my_int), status::NOT_FOUND);
	ASSERT_STATUS(cfg->get_uint64("non-existent-uint", my_uint), status::NOT_FOUND);
	ASSERT_STATUS(cfg->get_object("non-existent-object_ptr", my_object),
		      status::NOT_FOUND);
	ASSERT_STATUS(cfg->get_data("non-existent-data", my_object, my_object_count),
		      status::NOT_FOUND);
	UT_ASSERTeq(my_object_count, 0U);

	delete cfg;
}

/* XXX: add tests for putting binary (and perhaps) random data into config */

static void test(int argc, char *argv[])
{
	simple_test();
	put_oid_simple_test();
	put_edge_cases();
	object_unique_ptr_nullptr_test();
	object_unique_ptr_default_deleter_test();
	object_unique_ptr_custom_deleter_test();
	integral_conversion_test();
	not_found_test();
	constructors_test();
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
